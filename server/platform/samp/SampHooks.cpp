#include "SampHooks.h"

#include <array>
#include <vector>
#include <memory>
#include <cstdint>

#include <raknet/bitstream.h>
#include <raknet/networktypes.h>
#include <raknet/urmem.hpp>

#include "../../Events.h"
#include "../../Afk.h"
#include "../../DebugLog.h"
#include "../../Platform.h"
#include <chrono>

#ifdef _WIN32
#define THISCALL __thiscall
#else
#define THISCALL
#endif

namespace
{
	// Сигнатура функции, которая создаёт RakServer (samp-server 0.3.7)
#ifdef _WIN32
	const char* pattern =
		"\x6A\xFF\x68\x5B\xA4\x4A\x00\x64\xA1\x00\x00"
		"\x00\x00\x50\x64\x89\x25\x00\x00\x00\x00\x51"
		"\x68\x18\x0E\x00\x00\xE8\xFF\xFF\xFF\xFF\x83"
		"\xC4\x04\x89\x04\x24\x85\xC0\xC7\x44\x24\xFF"
		"\x00\x00\x00\x00\x74\x16";
	const char* mask = "???????xxxxxxxxxxxxxxxx????x????xxxxxxxxxxx?xxxxxx";

	enum RakServerOffset
	{
		SEND = 7,
		RPC = 32,
		RECEIVE = 10,
		REGISTER_CALL = 29,
		GET_PLAYER_ID_INDEX_FROM = 57,
		GET_PLAYER_ID_FROM_INDEX = 58
	};
#else
	const char* pattern =
		"\x55\x89\xE5\x83\xEC\x18\xC7\x04\x24\xFF\xFF"
		"\xFF\xFF\x89\x75\xFF\x89\x5D\xFF\xE8\xFF\xFF"
		"\xFF\xFF\x89\x04\x24\x89\xC6\xE8\xFF\xFF\xFF"
		"\xFF\x89\xF0\x8B\x5D\xFF\x8B\x75\xFF\x89\xEC"
		"\x5D\xC3";
	const char* mask = "?????xxxx????xx?xx?x????xxxxxx????xxxx?xx?xxxx";

	enum RakServerOffset
	{
		SEND = 9,
		RECEIVE = 11,
		RPC = 35,
		REGISTER_CALL = 30,
		GET_PLAYER_ID_INDEX_FROM = 58,
		GET_PLAYER_ID_FROM_INDEX = 59
	};
#endif

	urmem::address_t rakServer{},
		funcSend{},
		funcRPC{},
		funcReceive{},
		funcRegisterRPC{},
		funcGetIndexFromPlayerID{},
		funcGetPlayerIDFromIndex{};

	std::shared_ptr<urmem::hook> hookGetRakServer, hookRPC, hookReceive, hookSend, hookRegisterRPC;
	std::array<RPCFunction, MAX_RPC_MAP_SIZE> originalRPC{}, customRPC{};

	int GetIndexFromPlayerID(const PlayerID& id)
	{
		return urmem::call_function<urmem::calling_convention::thiscall, int>(funcGetIndexFromPlayerID, rakServer, id);
	}

	PlayerID GetPlayerIDFromIndex(int index)
	{
		return urmem::call_function<urmem::calling_convention::thiscall, PlayerID>(funcGetPlayerIDFromIndex, rakServer, index);
	}

	bool CallRPC(void* _this, RPCIndex* id, BitStream* bs, int priority, int reliability, char channel, PlayerID player, bool broadcast, bool shiftTimestamp)
	{
		return urmem::call_function<urmem::calling_convention::thiscall, bool>(funcRPC, _this, id, bs, priority, reliability, channel, player, broadcast, shiftTimestamp);
	}

	// ---- входящие RPC: подменяем обработчики сервера своими
	void ReceiveRPC(int rpcid, RPCParameters* p)
	{
		if (!p) return;
		const int playerid = GetIndexFromPlayerID(p->sender);
		if (Afk::IsActivityRPC(rpcid)) Afk::Activity(playerid);
		if (p->input)
		{
			BitStream bs(p->input, BITS_TO_BYTES(p->numberOfBitsOfData), false);
			Events::OnIncomingRPC(playerid, rpcid, bs);
		}
		if (originalRPC[rpcid]) originalRPC[rpcid](p);
	}

	template <size_t ID>
	struct RPCHandler
	{
		static void Handle(RPCParameters* p) { ReceiveRPC(ID, p); }
		static void Create()
		{
			customRPC[ID] = &Handle;
			RPCHandler<ID + 1>::Create();
		}
	};
	template <>
	struct RPCHandler<MAX_RPC_MAP_SIZE>
	{
		static void Create() {}
	};

	constexpr int SAMP_MAX_PLAYERS = 1000, SAMP_MAX_VEHICLES = 2000; // лимиты SA-MP 0.3.7

	// отброшенный пакет: всё - в modgear_debug.log, в лог сервера - не чаще раза в минуту на игрока
	// __thiscall в MSVC допустим только у методов — поэтому статические методы структуры
	struct Hook
	{
		static void* THISCALL RegisterRPC(void* _this, RPCIndex* id, RPCFunction function);
		static Packet* THISCALL Receive(void* _this);
		static bool THISCALL Send(void* _this, BitStream* bs, int priority, int reliability, char channel, PlayerID player, bool broadcast);
		static bool THISCALL RPC(void* _this, RPCIndex* id, BitStream* bs, int priority, int reliability, char channel, PlayerID player, bool broadcast, bool shiftTimestamp);
	};

	void* THISCALL Hook::RegisterRPC(void* _this, RPCIndex* id, RPCFunction function)
	{
		urmem::hook::raii scope(*hookRegisterRPC);
		if (id && function && *id < MAX_RPC_MAP_SIZE)
		{
			originalRPC[*id] = function;
			function = customRPC[*id];
		}
		static bool voiceRegistered = false; // RPC 252 (голос Arizona) сервер не знает - регистрируем сами
		if (!voiceRegistered)
		{
			voiceRegistered = true;
			RPCIndex voice = 252;
			urmem::call_function<urmem::calling_convention::thiscall, void*>(funcRegisterRPC, _this, &voice, customRPC[252]);
		}
		return urmem::call_function<urmem::calling_convention::thiscall, void*>(funcRegisterRPC, _this, id, function);
	}

	// ---- входящие пакеты
	Packet* THISCALL Hook::Receive(void* _this)
	{
		urmem::hook::raii scope(*hookReceive);
		Packet* packet = urmem::call_function<urmem::calling_convention::thiscall, Packet*>(funcReceive, _this);
		if (packet && packet->data && packet->length && packet->playerIndex < SAMP_MAX_PLAYERS && Afk::IsActivityPacket(packet->data[0]))
			Afk::Activity(packet->playerIndex); // синхронизацию свёрнутая игра не шлёт
		if (packet && packet->data && packet->length && packet->playerIndex < SAMP_MAX_PLAYERS && Events::IsSyncPacket(packet->data[0]))
			Events::OnIncomingSync(packet->playerIndex, packet->data, packet->length); // полный ID оружия (кастомное Arizona)
		if (packet && packet->data && packet->length > 1 && packet->data[0] == 220 && packet->playerIndex != 0xFFFF)
		{
			BitStream bs(packet->data, packet->length, false);
			bs.IgnoreBits(8);
			Events::OnIncomingCustomPacket(packet->playerIndex, bs);
		}
		else if (packet && packet->data && packet->length > 2 && packet->data[0] == 221 && packet->playerIndex != 0xFFFF)
		{
			BitStream bs(packet->data, packet->length, false);
			bs.IgnoreBits(8);
			Events::OnPedPacket(packet->playerIndex, bs);
		}
		else if (packet && packet->data && packet->length > 4 && packet->data[0] == 252 && packet->playerIndex != 0xFFFF)
			Events::OnVoicePacket(packet->playerIndex, packet->data, packet->length);
		return packet;
	}

	// ---- исходящие пакеты
	// синхронизация другого игрока игроку с лаунчером: дописываем байт с полным ID оружия (так делает сервер
	// Arizona - клиент читает его из конца пакета, без него показывает оружие из мусора)
	bool THISCALL Hook::Send(void* _this, BitStream* bs, int priority, int reliability, char channel, PlayerID player, bool broadcast)
	{
		urmem::hook::raii scope(*hookSend);
		if (bs && !broadcast && bs->GetNumberOfBitsUsed() >= 24 && Events::IsSyncPacket(bs->GetData()[0]))
		{
			const int viewerid = GetIndexFromPlayerID(player);
			const int weapon = Events::OutgoingSyncWeapon(viewerid, bs->GetData(), bs->GetNumberOfBitsUsed());
			if (weapon >= 0)
			{
				std::vector<unsigned char> data;
				Events::AppendByte(bs->GetData(), bs->GetNumberOfBitsUsed(), (unsigned char)weapon, data);
				BitStream copy(data.data(), (unsigned int)data.size(), false);
				copy.SetWriteOffset(bs->GetNumberOfBitsUsed() + 8);
				return urmem::call_function<urmem::calling_convention::thiscall, bool>(funcSend, _this, &copy, priority, reliability, channel, player, broadcast);
			}
		}
		return urmem::call_function<urmem::calling_convention::thiscall, bool>(funcSend, _this, bs, priority, reliability, channel, player, broadcast);
	}

	// ---- исходящие RPC
	bool THISCALL Hook::RPC(void* _this, RPCIndex* id, BitStream* bs, int priority, int reliability, char channel, PlayerID player, bool broadcast, bool shiftTimestamp)
	{
		urmem::hook::raii scope(*hookRPC);
		if (!id || !bs || broadcast) return CallRPC(_this, id, bs, priority, reliability, channel, player, broadcast, shiftTimestamp);

		const int playerid = GetIndexFromPlayerID(player);
		if (playerid == -1) return CallRPC(_this, id, bs, priority, reliability, channel, player, broadcast, shiftTimestamp);

		const int rpcid = *id;
		const auto readOffset = bs->GetReadOffset();

		// стрим: 163 — игрок пропал, 165 — машина пропала
		if (rpcid == 163 || rpcid == 165)
		{
			uint16_t target = 0;
			bs->Read(target);
			bs->SetReadOffset(readOffset);
			if (rpcid == 163) Events::OnPlayerStreamOut(playerid, target);
			else Events::OnVehicleStreamOut(target, playerid);
		}

		Events::OnOutgoingRPC(playerid, rpcid, *bs);
		const bool result = CallRPC(_this, id, bs, priority, reliability, channel, player, broadcast, shiftTimestamp);
		Events::OnOutgoingRPCSent(playerid, rpcid, *bs);

		// стрим: 32 — игрок появился, 164 — машина появилась
		if (rpcid == 32 || rpcid == 164)
		{
			uint16_t target = 0;
			bs->SetReadOffset(0);
			bs->Read(target);
			bs->SetReadOffset(readOffset);
			if (rpcid == 32) Events::OnPlayerStreamIn(playerid, target);
			else Events::OnVehicleStreamIn(target, playerid);
		}
		return result;
	}

	urmem::address_t GetRakServerInterface()
	{
		const auto server = hookGetRakServer->call<urmem::calling_convention::cdeclcall, urmem::address_t>();
		const auto vmt = urmem::pointer(server).field<urmem::address_t*>(0);

		rakServer = server;
		funcSend = vmt[SEND];
		funcRPC = vmt[RPC];
		funcReceive = vmt[RECEIVE];
		funcRegisterRPC = vmt[REGISTER_CALL];
		funcGetIndexFromPlayerID = vmt[GET_PLAYER_ID_INDEX_FROM];
		funcGetPlayerIDFromIndex = vmt[GET_PLAYER_ID_FROM_INDEX];

		originalRPC.fill(nullptr);
		customRPC.fill(nullptr);
		RPCHandler<0>::Create();

		hookRPC = std::make_shared<urmem::hook>(funcRPC, urmem::get_func_addr(&Hook::RPC));
		hookReceive = std::make_shared<urmem::hook>(funcReceive, urmem::get_func_addr(&Hook::Receive));
		hookSend = std::make_shared<urmem::hook>(funcSend, urmem::get_func_addr(&Hook::Send));
		hookRegisterRPC = std::make_shared<urmem::hook>(funcRegisterRPC, urmem::get_func_addr(&Hook::RegisterRPC));
		return server;
	}
}

bool SampHooks::Init(void* logprintf)
{
	urmem::sig_scanner scanner;
	urmem::address_t address{};
	if (!scanner.init(reinterpret_cast<urmem::address_t>(logprintf)) || !scanner.find(pattern, mask, address)) return false;
	hookGetRakServer = std::make_shared<urmem::hook>(address, urmem::get_func_addr(&GetRakServerInterface));
	return true;
}

bool SampHooks::SendPacket(int playerid, BitStream* bs)
{
	if (!rakServer) return false;
	const bool broadcast = playerid == -1;
	const PlayerID target = broadcast ? UNASSIGNED_PLAYER_ID : GetPlayerIDFromIndex(playerid);
	return urmem::call_function<urmem::calling_convention::thiscall, bool>(funcSend, rakServer, bs, (int)HIGH_PRIORITY, (int)RELIABLE_ORDERED, '\0', target, broadcast);
}

bool SampHooks::SendRPC(int playerid, int rpcid, BitStream* bs)
{
	if (!rakServer) return false;
	RPCIndex id = static_cast<RPCIndex>(rpcid);
	const bool broadcast = playerid == -1;
	const PlayerID target = broadcast ? UNASSIGNED_PLAYER_ID : GetPlayerIDFromIndex(playerid);
	return urmem::call_function<urmem::calling_convention::thiscall, bool>(funcRPC, rakServer, &id, bs, (int)HIGH_PRIORITY, (int)RELIABLE_ORDERED, '\0', target, broadcast, false);
}
