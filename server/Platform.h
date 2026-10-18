#pragma once

// Всё, что зависит от сервера (SA-MP или open.mp), — только через эти функции
// Реализации: platform/samp/*.cpp и platform/omp/*.cpp

#include <string>

class BitStream;

namespace Platform
{
	enum class LogLevel { Message, Warning, Error };

	void Log(LogLevel level, const char* format, ...);

	bool SendPacket(int playerid, BitStream* bs);           // playerid == -1 -> всем
	bool SendRPC(int playerid, int rpcid, BitStream* bs);   // playerid == -1 -> всем

	bool IsPlayerConnected(int playerid);
	bool IsVehicleValid(int vehicleid);
	bool IsPlayerStreamedIn(int playerid, int forplayerid);   // виден ли playerid игроку forplayerid
	bool IsVehicleStreamedIn(int vehicleid, int forplayerid);
	bool GetPlayerPos(int playerid, float& x, float& y, float& z);
	bool GetVehiclePos(int vehicleid, float& x, float& y, float& z);
	int GetPlayerVehicle(int playerid, int* seat = nullptr);    // машина игрока (0 - пешком), seat 0 - водитель
	int GetPlayerVirtualWorld(int playerid);
	int GetPlayerInterior(int playerid);
	std::string GetPlayerName(int playerid);
	std::string GetPlayerIp(int playerid);                       // "" - нет игрока
	bool GetObjectPos(int objectid, float& x, float& y, float& z);   // позиция глобального объекта (CreateObject)

	// адрес, на котором запущен сервер (bind / port из конфига)
	std::string GetBindAddress();
	int GetPort();
}
