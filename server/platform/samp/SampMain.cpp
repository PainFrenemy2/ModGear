// Бэкенд SA-MP: обычный плагин samp-server (через sampgdk)

// Platform.h — до sampgdk.h, иначе его макросы переименуют объявления
#include "../../Platform.h"
#include <sampgdk.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <chrono>

#include <raknet/bitstream.h>
#include "SampHooks.h"
#include "../../Plugin.h"
#include "../../Events.h"
#include "../../Version.h"
#include "../../LogQueue.h"
#include "../../Paths.h"

namespace
{
	bool active = false;
	std::chrono::steady_clock::time_point lastSecond, lastFast;
}

PLUGIN_EXPORT unsigned int PLUGIN_CALL Supports()
{
	return SUPPORTS_VERSION | SUPPORTS_AMX_NATIVES | SUPPORTS_PROCESS_TICK;
}

// очередь лога фоновых потоков -> logprintf (только основной поток)
static void FlushLog()
{
	LogQueue::Flush([](int, const char* text) { sampgdk::logprintf("%s", text); });
}

PLUGIN_EXPORT bool PLUGIN_CALL Load(void** ppData)
{
	if (!sampgdk::Load(ppData)) return false;
	LogQueue::SetMainThread();

	Plugin::PrintBanner("SA-MP 0.3.7");

	if (!SampHooks::Init(ppData[PLUGIN_DATA_LOGPRINTF]))
	{
		Platform::Log(Platform::LogLevel::Error, "[%s]: Неподдерживаемая версия сервера (нужен SA-MP 0.3.7)", MODGEAR_NAME);
		return false;
	}
	if (!Plugin::Init(ppData[PLUGIN_DATA_AMX_EXPORTS])) return false;

	lastSecond = std::chrono::steady_clock::now();
	active = true;
	return true;
}

PLUGIN_EXPORT void PLUGIN_CALL Unload()
{
	if (active) Plugin::Shutdown();
	active = false;
	FlushLog();
	sampgdk::Unload();
}

PLUGIN_EXPORT int PLUGIN_CALL AmxLoad(AMX* amx)
{
	if (active) Plugin::OnAmxLoad(amx);
	return AMX_ERR_NONE;
}

PLUGIN_EXPORT int PLUGIN_CALL AmxUnload(AMX* amx)
{
	Plugin::OnAmxUnload(amx);
	return AMX_ERR_NONE;
}

PLUGIN_EXPORT void PLUGIN_CALL ProcessTick()
{
	sampgdk::ProcessTick();
	FlushLog();
	if (!active) return;
	Events::OnTick();
	const auto now = std::chrono::steady_clock::now();
	if (now - lastSecond >= std::chrono::seconds(1))
	{
		lastSecond = now;
		Events::OnSecondTimer();
	}
	if (now - lastFast >= std::chrono::milliseconds(200))
	{
		lastFast = now;
		Events::OnFastTimer();
	}
}

// колбэки сервера (sampgdk вызывает их по имени)
PLUGIN_EXPORT bool PLUGIN_CALL OnPlayerConnect(int playerid)
{
	if (!active) return true;
	char version[32] = { 0 };
	sampgdk_GetPlayerVersion(playerid, version, sizeof(version));
	Events::OnPlayerConnect(playerid, std::strstr(version, "arizona-mobile") != nullptr);
	return true;
}

PLUGIN_EXPORT bool PLUGIN_CALL OnPlayerDisconnect(int playerid, int reason)
{
	if (active) Events::OnPlayerDisconnect(playerid);
	return true;
}

// ---------------- Platform

void Platform::Log(LogLevel level, const char* format, ...)
{
	char buffer[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);
	if (LogQueue::OnMainThread()) sampgdk::logprintf("%s", buffer);
	else LogQueue::Push((int)level, buffer);
}

bool Platform::SendPacket(int playerid, BitStream* bs) { return SampHooks::SendPacket(playerid, bs); }
bool Platform::SendRPC(int playerid, int rpcid, BitStream* bs) { return SampHooks::SendRPC(playerid, rpcid, bs); }

// sampgdk объявляет нативы и как макросы — снимаем, чтобы не переименовать наши функции
#undef IsPlayerConnected
#undef IsPlayerStreamedIn
#undef IsVehicleStreamedIn
#undef GetPlayerPos
#undef GetPlayerVehicleID
#undef GetPlayerVehicleSeat
#undef GetPlayerVirtualWorld
#undef GetPlayerInterior
#undef GetObjectPos
#undef GetPlayerName
#undef GetPlayerIp
#undef GetVehiclePos

bool Platform::IsPlayerConnected(int playerid) { return sampgdk_IsPlayerConnected(playerid); }
bool Platform::IsVehicleValid(int vehicleid) { return sampgdk_GetVehicleModel(vehicleid) > 0; }
bool Platform::IsPlayerStreamedIn(int playerid, int forplayerid) { return sampgdk_IsPlayerStreamedIn(playerid, forplayerid); }
bool Platform::IsVehicleStreamedIn(int vehicleid, int forplayerid) { return sampgdk_IsVehicleStreamedIn(vehicleid, forplayerid); }
bool Platform::GetPlayerPos(int playerid, float& x, float& y, float& z) { return sampgdk_GetPlayerPos(playerid, &x, &y, &z); }
bool Platform::GetVehiclePos(int vehicleid, float& x, float& y, float& z) { return sampgdk_GetVehiclePos(vehicleid, &x, &y, &z); }
int Platform::GetPlayerVehicle(int playerid, int* seat)
{
	const int vehicleid = sampgdk_GetPlayerVehicleID(playerid);
	if (seat) *seat = vehicleid ? sampgdk_GetPlayerVehicleSeat(playerid) : -1;
	return vehicleid;
}
int Platform::GetPlayerVirtualWorld(int playerid) { return sampgdk_GetPlayerVirtualWorld(playerid); }
int Platform::GetPlayerInterior(int playerid) { return sampgdk_GetPlayerInterior(playerid); }
std::string Platform::GetPlayerName(int playerid)
{
	char name[MAX_PLAYER_NAME + 1] = {};
	sampgdk_GetPlayerName(playerid, name, sizeof(name));
	return name;
}
bool Platform::GetObjectPos(int objectid, float& x, float& y, float& z) { return sampgdk_GetObjectPos(objectid, &x, &y, &z); }
std::string Platform::GetPlayerIp(int playerid)
{
	char ip[64] = {};
	if (!sampgdk_IsPlayerConnected(playerid) || sampgdk_GetPlayerIp(playerid, ip, sizeof(ip)) < 0) return "";
	return ip;
}

// В Load() нативы сервера ещё недоступны — читаем server.cfg сами
namespace
{
	std::string ReadServerCfg(const char* key)
	{
		std::string value;
		std::FILE* file = std::fopen(ServerPath("server.cfg").c_str(), "r");
		if (!file) return value;
		char line[512];
		const size_t keyLength = std::strlen(key);
		while (std::fgets(line, sizeof(line), file))
		{
			if (std::strncmp(line, key, keyLength) != 0 || (line[keyLength] != ' ' && line[keyLength] != '\t')) continue;
			std::string v(line + keyLength);
			const size_t b = v.find_first_not_of(" \t");
			const size_t e = v.find_last_not_of(" \t\r\n");
			value = b == std::string::npos ? "" : v.substr(b, e - b + 1);
		}
		std::fclose(file);
		return value;
	}
}

std::string Platform::GetBindAddress() { return ReadServerCfg("bind"); }

int Platform::GetPort()
{
	const std::string port = ReadServerCfg("port");
	return port.empty() ? 7777 : std::atoi(port.c_str());
}
