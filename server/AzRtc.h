#pragma once

// Медиасервер AZRTC (вебкамера Arizona) внутри плагина
// Клиент Arizona (azrtc.asi) по RPC 222 получает адрес, свой id и билет (токен + подпись) и открывает

#include <cstdint>
#include <string>
#include <vector>

namespace AzRtc
{
	constexpr int MAX_PEERS = 1000;

	enum class CamState { Off, Requested, Live };

	// gamePort - порт игрового сервера: samp.dll шифрует пакеты AZRTC его ключом, а не ключом нашего порта
	bool Start(int port, const std::string& bindAddress, int gamePort);
	void Stop();
	bool Running();
	int Port();

	// выдать игроку новый билет; out - готовое тело RPC 222 (host - адрес, который увидит клиент)
	bool MakeTicket(int playerid, const std::string& host, std::vector<uint8_t>& out);

	bool IsLinked(int playerid);
	CamState GetCam(int playerid);

	bool StartCam(int playerid);   // VideoInit: включить показ камеры игрока
	bool StopCam(int playerid);
	void SetViewers(int playerid, const std::vector<int>& viewers); // кому раздавать камеру игрока
	void PlayerLeft(int playerid); // игрок вышел с сервера: закрыть его соединение

	struct Event
	{
		enum Type { Linked, Unlinked, Cam, Log } type;
		int playerid;
		int value;          // Log: 1 - важное (в лог сервера), 0 - подробности (modgear_debug.log); Cam: 1 - показ идёт, 0 - остановлен, -1 - отклонён клиентом (камера выключена/не открылась)
		std::string text;   // Log
	};
	// события из сетевого потока (подключился/отключился, состояние камеры, сообщения в лог)
	void PollEvents(std::vector<Event>& out);
}
