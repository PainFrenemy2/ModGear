// Бэкенд open.mp: компонент сервера

#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/Vehicles/vehicles.hpp>
#include <Server/Components/Objects/objects.hpp>
#include <Server/Components/Timers/timers.hpp>
#include <bitstream.hpp>

#include <cstdarg>
#include <cstdio>

#include <raknet/bitstream.h>
#include "../../Platform.h"
#include "../../Afk.h"
#include "../../Plugin.h"
#include "../../Events.h"
#include "../../Version.h"
#include "../../LogQueue.h"

namespace
{
	ICore* core = nullptr;
	IPlayerPool* players = nullptr;
	IVehiclesComponent* vehicles = nullptr;
	IObjectsComponent* objects = nullptr;

	void WriteLog(int level, const char* text)
	{
		if (!core) return;
		const ::LogLevel omp = level == (int)Platform::LogLevel::Error ? ::LogLevel::Error
			: level == (int)Platform::LogLevel::Warning ? ::LogLevel::Warning : ::LogLevel::Message;
		core->logLn(omp, "%s", text);
	}

	// очередь лога фоновых потоков -> лог сервера (только основной поток)
	void FlushLog() { LogQueue::Flush(WriteLog); }

	// BitStream поверх данных NetworkBitStream (без копии) — правки пишутся прямо в пакет
	struct Wrap final : BitStream
	{
		explicit Wrap(NetworkBitStream& bs) : BitStream(bs.GetData(), bs.GetNumberOfBytesUsed(), false)
		{
			SetWriteOffset(bs.GetNumberOfBitsUsed());
			SetReadOffset(bs.GetReadOffset());
		}
	};

	struct FastTimer final : TimerTimeOutHandler
	{
		void timeout(ITimer&) override { Events::OnFastTimer(); }
		void free(ITimer&) override { delete this; }
	};
	struct TickTimer final : TimerTimeOutHandler
	{
		void timeout(ITimer&) override { FlushLog(); Events::OnTick(); }
		void free(ITimer&) override { delete this; }
	};

	struct SecondTimer final : TimerTimeOutHandler
	{
		void timeout(ITimer&) override { Events::OnSecondTimer(); }
		void free(ITimer&) override { delete this; }
	};

	struct NetEvents final : PlayerConnectEventHandler, PlayerStreamEventHandler, PlayerUpdateEventHandler, VehicleEventHandler, NetworkInEventHandler, NetworkOutEventHandler
	{
		void onPlayerConnect(IPlayer& player) override
		{
			const StringView version = player.getClientVersionName();
			Events::OnPlayerConnect(player.getID(), version.find("arizona-mobile") != StringView::npos);
		}
		void onPlayerDisconnect(IPlayer& player, PeerDisconnectReason) override { Events::OnPlayerDisconnect(player.getID()); }

		void onPlayerStreamIn(IPlayer& player, IPlayer& forPlayer) override { Events::OnPlayerStreamIn(forPlayer.getID(), player.getID()); }
		void onPlayerStreamOut(IPlayer& player, IPlayer& forPlayer) override { Events::OnPlayerStreamOut(forPlayer.getID(), player.getID()); }
		void onVehicleStreamIn(IVehicle& vehicle, IPlayer& player) override { Events::OnVehicleStreamIn(vehicle.getID(), player.getID()); }
		void onVehicleStreamOut(IVehicle& vehicle, IPlayer& player) override { Events::OnVehicleStreamOut(vehicle.getID(), player.getID()); }

		// синхронизацию (пешком, машина, прицел...) open.mp разбирает сам и в onReceivePacket не отдаёт
		// активность игрока берём отсюда. Без этого игрок считался свёрнутым с входа, и пакеты плагина
		// (радио, подсказки и т.д.) копились в очереди AFK
		bool onPlayerUpdate(IPlayer& player, TimePoint) override { Afk::Activity(player.getID()); return true; }

		bool onReceivePacket(IPlayer& peer, int id, NetworkBitStream& bs) override
		{
			if (Afk::IsActivityPacket(id)) Afk::Activity(peer.getID()); // синхронизацию свёрнутая игра не шлёт
			if (id == 220)
			{
				Wrap w(bs);
				Events::OnIncomingCustomPacket(peer.getID(), w);
			}
			else if (id == 221)
			{
				Wrap w(bs);
				Events::OnPedPacket(peer.getID(), w);
			}
			else if (id == 252 && bs.GetNumberOfBytesUsed() > 4)
				Events::OnVoicePacket(peer.getID(), bs.GetData(), bs.GetNumberOfBytesUsed());
			return true;
		}
		bool onReceiveRPC(IPlayer& peer, int id, NetworkBitStream& bs) override
		{
			if (Afk::IsActivityRPC(id)) Afk::Activity(peer.getID());
			Wrap w(bs);
			Events::OnIncomingRPC(peer.getID(), id, w);
			return true;
		}
		// синхронизация другого игрока игроку с лаунчером: шлём свою копию с байтом полного ID оружия в конце
		// (open.mp отправляет исходные данные, а не bs - дописать в bs нельзя), исходную отменяем
		bool onSendPacket(IPlayer* peer, int id, NetworkBitStream& bs) override
		{
			if (!peer || !Events::IsSyncPacket(id)) return true;
			const int weapon = Events::OutgoingSyncWeapon(peer->getID(), bs.GetData(), (unsigned int)bs.GetNumberOfBitsUsed());
			if (weapon < 0) return true;
			std::vector<unsigned char> data;
			Events::AppendByte(bs.GetData(), (unsigned int)bs.GetNumberOfBitsUsed(), (unsigned char)weapon, data);
			peer->sendPacket(Span<uint8_t>(data.data(), bs.GetNumberOfBitsUsed() + 8), OrderingChannel_SyncPacket, false);
			return false;
		}
		bool onSendRPC(IPlayer* peer, int id, NetworkBitStream& bs) override
		{
			if (!peer) return true;
			Wrap w(bs);
			Events::OnOutgoingRPC(peer->getID(), id, w);
			Events::OnOutgoingRPCSent(peer->getID(), id, w);
			return true;
		}
	} netEvents;

	// синхронизацию от игрока open.mp в общий onReceivePacket не отдаёт - ловим по ID пакета
	// клиент Arizona дописывает в конец байт с полным ID оружия (кастомное 71+)
	struct SyncIn final : SingleNetworkInEventHandler
	{
		bool onReceive(IPlayer& peer, NetworkBitStream& bs) override
		{
			Events::OnIncomingSync(peer.getID(), bs.GetData(), (unsigned int)bs.GetNumberOfBytesUsed());
			return true;
		}
	} syncIn;
	constexpr int SYNC_PACKETS[] = { 200, 207, 211 };
}

class ModGearComponent final : public IComponent, public PawnEventHandler
{
public:
	PROVIDE_UID(0x3a6df347f12ba19);

	StringView componentName() const override { return MODGEAR_NAME; }
	SemanticVersion componentVersion() const override { return SemanticVersion(MODGEAR_VER_MAJOR, MODGEAR_VER_MINOR, MODGEAR_VER_PATCH, 0); }

	void onLoad(ICore* c) override
	{
		LogQueue::SetMainThread();
		core = c;
		players = &c->getPlayers();
	}

	void onInit(IComponentList* components) override
	{
		pawn = components->queryComponent<IPawnComponent>();
		vehicles = components->queryComponent<IVehiclesComponent>();
		objects = components->queryComponent<IObjectsComponent>();
		timers = components->queryComponent<ITimersComponent>();
		if (!pawn || !vehicles || !timers)
		{
			core->logLn(LogLevel::Error, "[%s]: Pawn / Vehicles / Timers component not found", MODGEAR_NAME);
			return;
		}

		Plugin::PrintBanner("open.mp");

		Plugin::Init((void*)&pawn->getAmxFunctions());
		pawn->getEventDispatcher().addEventHandler(this);
		vehicles->getEventDispatcher().addEventHandler(&netEvents);
		players->getPlayerConnectDispatcher().addEventHandler(&netEvents);
		players->getPlayerStreamDispatcher().addEventHandler(&netEvents);
		players->getPlayerUpdateDispatcher().addEventHandler(&netEvents);
		for (INetwork* network : core->getNetworks())
		{
			network->getInEventDispatcher().addEventHandler(&netEvents);
			network->getOutEventDispatcher().addEventHandler(&netEvents);
			for (int id : SYNC_PACKETS) network->getPerPacketInEventDispatcher().addEventHandler(&syncIn, id, EventPriority_FairlyHigh);
		}
		timers->create(new SecondTimer(), Seconds(1), true);
		timers->create(new FastTimer(), Milliseconds(200), true);
		timers->create(new TickTimer(), Milliseconds(20), true);
		active = true;
	}

	void onAmxLoad(IPawnScript& script) override { Plugin::OnAmxLoad((AMX*)script.GetAMX()); }
	void onAmxUnload(IPawnScript& script) override { Plugin::OnAmxUnload((AMX*)script.GetAMX()); }

	void onFree(IComponent* component) override
	{
		// open.mp освобождает компоненты по очереди: уже освобождённые забываем
		// сервера, если Vehicles освобождался
		if (component == vehicles) vehicles = nullptr;
		if (component == objects) objects = nullptr;
		if (component == timers) timers = nullptr;
		if (!active || (component != pawn && component != this)) return;
		active = false;
		pawn->getEventDispatcher().removeEventHandler(this);
		if (vehicles) vehicles->getEventDispatcher().removeEventHandler(&netEvents);
		players->getPlayerConnectDispatcher().removeEventHandler(&netEvents);
		players->getPlayerStreamDispatcher().removeEventHandler(&netEvents);
		players->getPlayerUpdateDispatcher().removeEventHandler(&netEvents);
		for (INetwork* network : core->getNetworks())
		{
			network->getInEventDispatcher().removeEventHandler(&netEvents);
			network->getOutEventDispatcher().removeEventHandler(&netEvents);
			for (int id : SYNC_PACKETS) network->getPerPacketInEventDispatcher().removeEventHandler(&syncIn, id);
		}
		Plugin::Shutdown();
		FlushLog();
		pawn = nullptr;
	}

	void reset() override {}
	void free() override { delete this; }

private:
	IPawnComponent* pawn = nullptr;
	ITimersComponent* timers = nullptr;
	bool active = false;
};

COMPONENT_ENTRY_POINT() { return new ModGearComponent(); }

// ---------------- Platform

void Platform::Log(LogLevel level, const char* format, ...)
{
	if (!core) return;
	char buffer[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	if (LogQueue::OnMainThread()) WriteLog((int)level, buffer);
	else LogQueue::Push((int)level, buffer);
}

bool Platform::SendPacket(int playerid, BitStream* bs)
{
	if (!players) return false;
	const Span<uint8_t> data(bs->GetData(), bs->GetNumberOfBitsUsed());
	if (playerid == -1)
	{
		players->broadcastPacket(data, 0, nullptr, false);
		return true;
	}
	IPlayer* player = players->get(playerid);
	return player && player->sendPacket(data, 0, false);
}

bool Platform::SendRPC(int playerid, int rpcid, BitStream* bs)
{
	if (!players) return false;
	const Span<uint8_t> data(bs->GetData(), bs->GetNumberOfBitsUsed());
	if (playerid == -1)
	{
		players->broadcastRPC(rpcid, data, 0, nullptr, false);
		return true;
	}
	IPlayer* player = players->get(playerid);
	return player && player->sendRPC(rpcid, data, 0, false);
}

bool Platform::IsPlayerConnected(int playerid)
{
	return players && players->get(playerid) != nullptr;
}

std::string Platform::GetPlayerName(int playerid)
{
	IPlayer* player = players ? players->get(playerid) : nullptr;
	if (!player) return "";
	const StringView name = player->getName();
	return std::string(name.data(), name.length());
}

std::string Platform::GetPlayerIp(int playerid)
{
	IPlayer* player = players ? players->get(playerid) : nullptr;
	if (!player) return "";
	PeerAddress::AddressString ip;
	if (!PeerAddress::ToString(player->getNetworkData().networkID.address, ip)) return "";
	return std::string(ip.data(), ip.length());
}

bool Platform::IsVehicleValid(int vehicleid)
{
	return vehicles && vehicles->get(vehicleid) != nullptr;
}

bool Platform::IsPlayerStreamedIn(int playerid, int forplayerid)
{
	if (!players) return false;
	IPlayer* player = players->get(playerid);
	IPlayer* forPlayer = players->get(forplayerid);
	return player && forPlayer && player->isStreamedInForPlayer(*forPlayer);
}

bool Platform::IsVehicleStreamedIn(int vehicleid, int forplayerid)
{
	if (!players || !vehicles) return false;
	IVehicle* vehicle = vehicles ? vehicles->get(vehicleid) : nullptr;
	IPlayer* forPlayer = players->get(forplayerid);
	return vehicle && forPlayer && vehicle->isStreamedInForPlayer(*forPlayer);
}

bool Platform::GetPlayerPos(int playerid, float& x, float& y, float& z)
{
	IPlayer* player = players ? players->get(playerid) : nullptr;
	if (!player) return false;
	Vector3 pos = player->getPosition();
	x = pos.x; y = pos.y; z = pos.z;
	return true;
}

bool Platform::GetVehiclePos(int vehicleid, float& x, float& y, float& z)
{
	IVehicle* vehicle = vehicles ? vehicles->get(vehicleid) : nullptr;
	if (!vehicle) return false;
	Vector3 pos = vehicle->getPosition();
	x = pos.x; y = pos.y; z = pos.z;
	return true;
}

int Platform::GetPlayerVehicle(int playerid, int* seat)
{
	if (seat) *seat = -1;
	IPlayer* player = players ? players->get(playerid) : nullptr;
	if (!player) return 0;
	IPlayerVehicleData* data = queryExtension<IPlayerVehicleData>(*player);
	IVehicle* vehicle = data ? data->getVehicle() : nullptr;
	if (!vehicle) return 0;
	if (seat) *seat = data->getSeat();
	return vehicle->getID();
}

int Platform::GetPlayerVirtualWorld(int playerid)
{
	IPlayer* player = players ? players->get(playerid) : nullptr;
	return player ? player->getVirtualWorld() : 0;
}

int Platform::GetPlayerInterior(int playerid)
{
	IPlayer* player = players ? players->get(playerid) : nullptr;
	return player ? (int)player->getInterior() : 0;
}

bool Platform::GetObjectPos(int objectid, float& x, float& y, float& z)
{
	IObject* object = objects ? objects->get(objectid) : nullptr;
	if (!object) return false;
	Vector3 pos = object->getPosition();
	x = pos.x; y = pos.y; z = pos.z;
	return true;
}

std::string Platform::GetBindAddress()
{
	if (!core) return "";
	return std::string(core->getConfig().getString("network.bind"));
}

int Platform::GetPort()
{
	if (!core) return 0;
	const int* port = core->getConfig().getInt("network.port");
	return port ? *port : 0;
}
