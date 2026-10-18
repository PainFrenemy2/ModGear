#include "Afk.h"
#include "Platform.h"
#include "DebugLog.h"

#include <chrono>
#include <cstdint>
#include <deque>
#include <vector>
#include <raknet/bitstream.h>
#include <lib/InfoServer.h>

namespace
{
	// активный игрок шлёт синхронизацию почти каждую секунду, даже стоя на месте; свёрнутый - ничего
	constexpr int64_t AWAY_MS = 3000;
	constexpr size_t MAX_QUEUE_BYTES = 8 * 1024 * 1024; // больше - выбрасываем самые старые

	struct Entry { bool rpc; int id; int bits; std::vector<unsigned char> data; int64_t at; };

	bool joined[MAX_PLAYERS];
	int64_t lastActive[MAX_PLAYERS]; // 0 - после входа активности ещё не было
	std::deque<Entry> queue[MAX_PLAYERS];
	size_t queueBytes[MAX_PLAYERS];
	bool overflowLogged[MAX_PLAYERS];

	int64_t NowMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	bool Valid(int playerid) { return playerid >= 0 && playerid < MAX_PLAYERS; }

	bool SendNow(int playerid, bool rpc, int rpcid, BitStream* bs)
	{
		return rpc ? Platform::SendRPC(playerid, rpcid, bs) : Platform::SendPacket(playerid, bs);
	}

	void Enqueue(int playerid, bool rpc, int rpcid, BitStream* bs)
	{
		Entry e{ rpc, rpcid, bs->GetNumberOfBitsUsed(), {}, NowMs() };
		e.data.assign(bs->GetData(), bs->GetData() + bs->GetNumberOfBytesUsed());
		queueBytes[playerid] += e.data.size();
		queue[playerid].push_back(std::move(e));
		while (queueBytes[playerid] > MAX_QUEUE_BYTES && !queue[playerid].empty())
		{
			queueBytes[playerid] -= queue[playerid].front().data.size();
			queue[playerid].pop_front();
			if (!overflowLogged[playerid])
			{
				overflowLogged[playerid] = true;
				DebugLog::Write("[ModGear]: AFK: у игрока %d переполнена очередь пакетов, старые выброшены", playerid);
			}
		}
	}

	bool AnyAway()
	{
		for (int i : connectedPlayers) if (Afk::Away(i)) return true;
		return false;
	}
}

void Afk::Joined(int playerid)
{
	if (!Valid(playerid)) return;
	Left(playerid);
	joined[playerid] = true;
}

void Afk::Left(int playerid)
{
	if (!Valid(playerid)) return;
	joined[playerid] = false;
	lastActive[playerid] = 0;
	queue[playerid].clear();
	queueBytes[playerid] = 0;
	overflowLogged[playerid] = false;
}

void Afk::Activity(int playerid)
{
	if (Valid(playerid)) lastActive[playerid] = NowMs();
}

bool Afk::IsActivityPacket(int packetid)
{
	// 200 машина, 203 прицел, 207 пешком, 209 пустая машина, 210 прицеп, 211 пассажир, 212 наблюдение.
	// 204 (оружие) и 205 (статы) клиент шлёт и свёрнутым
	return packetid == 200 || packetid == 203 || (packetid >= 207 && packetid <= 212 && packetid != 208);
}

bool Afk::IsActivityRPC(int rpcid)
{
	// 25 вход, 128 / 129 класс, 52 спавн - сами при входе; 252 - войс Arizona
	return rpcid != 25 && rpcid != 128 && rpcid != 129 && rpcid != 52 && rpcid != 252;
}

bool Afk::Away(int playerid)
{
	if (!Valid(playerid) || !joined[playerid]) return false;
	return !lastActive[playerid] || NowMs() - lastActive[playerid] > AWAY_MS;
}

bool Afk::Send(int playerid, bool rpc, int rpcid, BitStream* bs)
{
	if (!bs) return false;
	if (playerid == -1)
	{
		if (!AnyAway()) return SendNow(-1, rpc, rpcid, bs);
		bool ok = false;
		for (int i : connectedPlayers) ok |= Send(i, rpc, rpcid, bs);
		return ok;
	}
	if (!Valid(playerid)) return SendNow(playerid, rpc, rpcid, bs);
	// очередь не пуста - досылка ещё идёт, новый пакет только после неё (порядок важен)
	if (!Away(playerid) && queue[playerid].empty()) return SendNow(playerid, rpc, rpcid, bs);
	Enqueue(playerid, rpc, rpcid, bs);
	return true;
}

void Afk::Flush()
{
	for (int i : connectedPlayers)
	{
		if (queue[i].empty() || Away(i)) continue;
		const size_t count = queue[i].size();
		while (!queue[i].empty())
		{
			Entry e = std::move(queue[i].front());
			queue[i].pop_front();
			BitStream bs;
			bs.WriteBits(e.data.data(), e.bits, false);
			// 252:17 - включить трек: в конце u32 с какой секунды. Пока пакет лежал, трек у остальных шёл -
			// досылаем с поправкой, иначе у вернувшегося трек отстаёт на время AFK
			if (e.rpc && e.id == 252 && !e.data.empty() && e.data[0] == 17 && e.bits >= 8 + 32)
			{
				bs.SetReadOffset(e.bits - 32);
				uint32_t sec = 0;
				bs.Read(sec);
				bs.SetWriteOffset(e.bits - 32);
				bs.Write((uint32_t)(sec + (NowMs() - e.at) / 1000));
			}
			SendNow(i, e.rpc, e.id, &bs);
		}
		queueBytes[i] = 0;
		overflowLogged[i] = false;
		if (count >= 300) DebugLog::Write("[ModGear]: AFK: игрок %d вернулся, дослано много пакетов (%d)", i, (int)count);
	}
}
