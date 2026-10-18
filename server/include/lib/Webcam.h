#pragma once
// Вебкамера Arizona (AZRTC): связка медиасервера AzRtc с игрой
//   Webcam_Init(port, host)  - поднять медиасервер на своём UDP-порту

#include <lib/InfoServer.h>
#include <raknet/bitstream.h>
#include "../../AzRtc.h"
#include "../../Afk.h"
#include "../../Platform.h"
#include "../../Plugin.h"
#include "../../DebugLog.h"
#include <string>
#include <vector>

namespace Webcam
{
	constexpr int RPC_LINK = 222;
	constexpr int TICKET_RETRY_SEC = 20;
	constexpr int TICKET_FAST_TRIES = 5;      // первые попытки раз в TICKET_RETRY_SEC, дальше раз в минуту - без конца
	constexpr int TICKET_SLOW_SEC = 60;

	inline std::string host;
	inline int ticketWait[MAX_PLAYERS];   // секунд до следующего RPC 222
	inline int ticketTries[MAX_PLAYERS];

	inline bool Valid(int playerid) { return playerid >= 0 && playerid < MAX_PLAYERS && playerid < AzRtc::MAX_PEERS; }

	// адрес, который отдаём клиенту: как у медиасервера (Media) - явный, локальный, bind сервера или внешний IP
	inline std::string ResolveHost(const std::string& given)
	{
		if (!given.empty()) return given;
		std::string bind = Platform::GetBindAddress();
		if (bind == "localhost") bind = "127.0.0.1";
		if (bind.compare(0, 4, "127.") == 0) return "127.0.0.1";
		if (!bind.empty() && bind != "0.0.0.0") return bind;   // как у сервера (bind в server.cfg)
		return Media::PublicIp();
	}

	inline bool Init(int port, const std::string& givenHost)
	{
		if (AzRtc::Running()) return true;
		host = ResolveHost(givenHost);
		if (host.empty())
		{
			Platform::Log(Platform::LogLevel::Warning, "[ModGear]: Вебкамера: не удалось узнать внешний IP сервера. Укажите его в Webcam_Init(port, \"ip\").");
			return false;
		}
		if (!AzRtc::Start(port, Platform::GetBindAddress(), Platform::GetPort()))
		{
			Platform::Log(Platform::LogLevel::Warning, "[ModGear]: Вебкамера: медиасервер не запустился (UDP-порт %d занят?).", port);
			return false;
		}
		for (int i = 0; i < MAX_PLAYERS; i++) { ticketWait[i] = 0; ticketTries[i] = 0; }
		DebugLog::Write("[ModGear]: Вебкамера: сервер AZRTC на %s:%d (UDP)", host.c_str(), port);
		return true;
	}

	inline void SendTicket(int playerid)
	{
		std::vector<uint8_t> payload;
		if (!AzRtc::MakeTicket(playerid, host, payload)) return;
		BitStream bs;
		for (uint8_t b : payload) bs.Write(b);
		Platform::SendRPC(playerid, RPC_LINK, &bs);
	}

	// отправить адрес заново сейчас (например, игрок нажал "вебкамера", а AZRTC ещё не подключён)
	inline void Relink(int playerid)
	{
		if (!Valid(playerid) || !AzRtc::Running() || AzRtc::IsLinked(playerid)) return;
		ticketWait[playerid] = 0;
		ticketTries[playerid] = 0;
	}

	inline void PlayerLeft(int playerid)
	{
		if (!Valid(playerid)) return;
		ticketWait[playerid] = 0;
		ticketTries[playerid] = 0;
		AzRtc::PlayerLeft(playerid);
	}

	// раз в секунду: билеты, зрители, события медиасервера
	inline void Update()
	{
		if (!AzRtc::Running()) return;

		std::vector<AzRtc::Event> events;
		AzRtc::PollEvents(events);
		for (const AzRtc::Event& e : events)
		{
			switch (e.type)
			{
			case AzRtc::Event::Log:
				// важное - в лог сервера, подробности (статистика, подключения) - в modgear_debug.log
				if (e.value) Platform::Log(Platform::LogLevel::Warning, "%s", e.text.c_str());
				else DebugLog::Write("%s", e.text.c_str());
				break;
			case AzRtc::Event::Linked:
				if (Valid(e.playerid)) ticketTries[e.playerid] = 0;
				Plugin::CallPublic("OnPlayerWebcamLink", { e.playerid, 1 });
				break;
			case AzRtc::Event::Unlinked:
				if (Valid(e.playerid)) ticketWait[e.playerid] = 3; // переподключение - новый билет чуть позже
				Plugin::CallPublic("OnPlayerWebcamLink", { e.playerid, 0 });
				break;
			case AzRtc::Event::Cam:
				Plugin::CallPublic("OnPlayerWebcam", { e.playerid, e.value });
				break;
			}
		}

		for (int i = 0; i < MAX_PLAYERS && i < AzRtc::MAX_PEERS; i++)
		{
			if (!Platform::IsPlayerConnected(i) || !PI[i].pLauncherStat) continue;
			// свёрнутому билет не шлём (не копим: адрес дошлём, когда вернётся)
			if (!AzRtc::IsLinked(i) && !Afk::Away(i) && --ticketWait[i] <= 0)
			{
				SendTicket(i);
				ticketTries[i]++;
				if (ticketTries[i] == TICKET_FAST_TRIES)
					DebugLog::Write("[ModGear]: Вебкамера: игрок %d не подключается к %s:%d (открыт ли UDP-порт?) - пробую раз в минуту",
						i, host.c_str(), AzRtc::Port());
				ticketWait[i] = ticketTries[i] < TICKET_FAST_TRIES ? TICKET_RETRY_SEC : TICKET_SLOW_SEC;
			}
			if (AzRtc::GetCam(i) != AzRtc::CamState::Live) continue;
			std::vector<int> viewers;
			for (int v = 0; v < MAX_PLAYERS && v < AzRtc::MAX_PEERS; v++)
				// свёрнутому не шлём: его клиент копил поток и после возврата проигрывал звук с задержкой.
				// Вернётся - снова зритель, поток начнётся со свежего ключевого кадра
				if (v != i && AzRtc::IsLinked(v) && !Afk::Away(v) && Platform::IsPlayerStreamedIn(i, v)) viewers.push_back(v);
			AzRtc::SetViewers(i, viewers);
		}
	}
}
