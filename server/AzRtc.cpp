#include "AzRtc.h"

#if !defined(_WIN32) && !defined(__stdcall)
#define __stdcall
#endif

#include <SAMPRakNet.hpp>
#include <RakNetworkFactory.h>
#include <RakServerInterface.h>
#include <PacketEnumerations.h>
#include <ReliabilityLayer.h>
#include <RakNetStatistics.h>

#include <array>
#include <atomic>
#include <bitset>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <random>
#include <thread>
#include <unordered_map>

// ---- логи RakNet (rakshim.hpp) -> в очередь событий, печатает основной поток
namespace AzRtc { void QueueLog(const char* text, bool important = false); }

void RakCoreShim::Write(RakLogLevel level, const char* fmt, va_list args)
{
	if (level == RakLogLevel::Debug) return;
	char text[512];
	std::vsnprintf(text, sizeof(text), fmt, args);
	AzRtc::QueueLog(text);
}

namespace AzRtc
{
namespace
{
	// пакеты соединения AZRTC (клиент: azrtc.asi, "azrtc link")
	constexpr uint8_t ID_LINK_CONNECT = 0xA0;   // [u16 pid][16 токен][u64][32 подпись]
	constexpr uint8_t ID_LINK_LINKED = 0xA1;    // [u16 pid][u64 время сервера, мс]
	constexpr uint8_t ID_LINK_REJECTED = 0xA2;  // [u16 pid][u8 причина]
	constexpr uint8_t ID_LINK_DATA = 0xA3;      // [u8 канал][...]

	constexpr uint8_t CHANNEL_WEBCAM = 1;
	// сообщения канала вебкамеры. Сервер -> клиент: [тип][u16 pid][...], клиент -> сервер: [тип][...]
	constexpr uint8_t CAM_INIT = 1;         // -> показ: включить свою камеру
	constexpr uint8_t CAM_STOP_SHOW = 2;    // -> показ: выключить свою камеру
	constexpr uint8_t CAM_PEER_START = 3;   // -> зрителю: у игрока pid пошла камера
	constexpr uint8_t CAM_STOPPED = 5;      // <- клиент выключил / отклонил;  -> зрителю: камера pid выключена
	constexpr uint8_t CAM_AUDIO = 6;        // на деле видео: кусок кадра VP8 (заголовок 16 байт + до 400 байт)
	constexpr uint8_t CAM_READY = 7;        // <- камера открылась, показ идёт
	constexpr uint8_t CAM_FAILED = 8;       // <- [u8 причина] камера не открылась / выключена в настройках
	constexpr uint8_t CAM_VIDEO = 9;        // на деле звук: [u16 seq][пакет Opus]
	constexpr uint8_t CAM_PEER_AUDIO_OFF = 10;
	constexpr uint8_t CAM_BATCH = 12;       // [u16 len][кусок видео][u16 seq][u8 n][n x (u16 len + Opus)]
	// кусок видео (тип 6 и начало типа 12): u32 номер кадра, u16 номер куска, u16 кусков, u32 ?, u32 размер кадра, данные
	constexpr size_t VIDEO_FRAG_HEADER = 16;
	// цепочка кадров одной камеры, которую отдаём новому зрителю разом. Больше - клиентский RakNet
	// azrtc.asi не успевает и рвёт соединение по таймауту; тогда зритель ждёт следующий ключевой кадр
	constexpr size_t GOP_MAX_BYTES = 256 * 1024;

	constexpr size_t TOKEN_LEN = 16;
	constexpr size_t SIG_LEN = 32;
	constexpr size_t TICKET_LEN = 2 + TOKEN_LEN + 8 + SIG_LEN;   // то, что клиент возвращает в CONNECT

	constexpr int64_t TICKET_TTL_MS = 120000;
	constexpr int64_t CAM_REQUEST_TIMEOUT_MS = 20000;
	constexpr int64_t COOKIE_RESEED_MS = 300000;

	int64_t NowMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	uint64_t HashAddr(const RakNet::PlayerID& id) { return (uint64_t(id.binaryAddress) << 16) | id.port; }

	// ---- состояние сетевого потока
	struct Peer
	{
		bool ticket = false;
		std::array<uint8_t, TICKET_LEN> ticketData{};
		int64_t ticketTime = 0;

		bool linked = false;
		RakNet::PlayerID addr{};

		CamState cam = CamState::Off;
		int64_t camRequestTime = 0;
		std::vector<int> viewers;          // кому раздаём (задаёт основной поток)
		std::bitset<MAX_PEERS> shown;      // кому уже отправлено "камера пошла"
		// зрители, которым нельзя слать разностные кадры: декодер VP8 ждёт ключевой кадр
		// а кадр с большим номером сбрасывает недособранный - ждём начала следующего ключевого
		std::bitset<MAX_PEERS> waitKey;
		uint32_t curFrameId = 0;           // кадр, куски которого сейчас идут
		bool curFrameKey = false;          // он ключевой
		uint32_t statIn = 0, statOut = 0;  // диагностика: пакетов камеры получено / разослано

		// все куски видео от последнего ключевого кадра (и недособранный текущий кадр). Новому зрителю
		// отдаём их подряд: VP8 декодирует разностный кадр только после всех предыдущих, а ключевые кадры
		std::vector<std::vector<uint8_t>> gop;
		int lastW = 0, lastH = 0;          // разрешение последнего ключевого кадра (для лога)
		size_t gopBytes = 0;
		bool gopValid = false;
	};

	struct Command
	{
		enum Type { Ticket, StartCam, StopCam, Viewers, Drop } type;
		int playerid;
		std::array<uint8_t, TICKET_LEN> ticket{};
		std::vector<int> viewers;
	};

	RakNet::RakServerInterface* server = nullptr;
	std::thread worker;
	std::atomic<bool> running{ false };
	int serverPort = 0;

	Peer peers[MAX_PEERS];                         // только сетевой поток
	std::unordered_map<uint64_t, int> addrToPeer;  // только сетевой поток

	std::mutex cmdMx;
	std::vector<Command> commands;

	std::mutex evMx;
	std::vector<Event> events;

	// зеркало для основного потока
	bool linkedMain[MAX_PEERS];
	CamState camMain[MAX_PEERS];

	void PushEvent(Event::Type type, int playerid, int value)
	{
		std::lock_guard<std::mutex> lock(evMx);
		events.push_back({ type, playerid, value, {} });
	}

	void PushCommand(Command&& cmd)
	{
		std::lock_guard<std::mutex> lock(cmdMx);
		commands.push_back(std::move(cmd));
	}

	bool ValidPeer(int id) { return id >= 0 && id < MAX_PEERS; }

	// ---- отправка
	void SendRaw(const RakNet::PlayerID& to, const std::vector<uint8_t>& data, bool reliable)
	{
		// длина в Send у RakNet open.mp - в БИТАХ
		server->Send(reinterpret_cast<const char*>(data.data()), (int)data.size() * 8,
			reliable ? RakNet::HIGH_PRIORITY : RakNet::MEDIUM_PRIORITY,
			reliable ? RakNet::RELIABLE_ORDERED : RakNet::UNRELIABLE, 0, to, false);
	}

	void SendCam(int to, uint8_t type, int pid, const uint8_t* payload = nullptr, size_t len = 0, bool reliable = true)
	{
		if (!peers[to].linked) return;
		std::vector<uint8_t> out;
		out.reserve(5 + len);
		out.push_back(ID_LINK_DATA);
		out.push_back(CHANNEL_WEBCAM);
		out.push_back(type);
		out.push_back(uint8_t(pid & 0xFF));
		out.push_back(uint8_t(pid >> 8));
		if (len) out.insert(out.end(), payload, payload + len);
		SendRaw(peers[to].addr, out, reliable);
	}

	// ---- зрители
	void HideFromAll(int pid)
	{
		Peer& p = peers[pid];
		for (int v = 0; v < MAX_PEERS; v++)
		{
			if (!p.shown.test(v)) continue;
			SendCam(v, CAM_STOPPED, pid);
			SendCam(v, CAM_PEER_AUDIO_OFF, pid);
		}
		p.shown.reset();
	}

	void SyncViewers(int pid)
	{
		Peer& p = peers[pid];
		if (p.cam != CamState::Live) { if (p.shown.any()) HideFromAll(pid); return; }
		std::bitset<MAX_PEERS> want;
		for (int v : p.viewers)
			if (ValidPeer(v) && v != pid && peers[v].linked) want.set(v);
		for (int v = 0; v < MAX_PEERS; v++)
		{
			if (want.test(v) && !p.shown.test(v))
			{
				const bool ready = p.gopValid && !p.gop.empty();
				SendCam(v, CAM_PEER_START, pid);
				if (ready) for (const auto& frag : p.gop) SendCam(v, CAM_AUDIO, pid, frag.data(), frag.size(), false);
				else p.waitKey.set(v);
			}
			else if (!want.test(v) && p.shown.test(v))
			{
				SendCam(v, CAM_STOPPED, pid);
				SendCam(v, CAM_PEER_AUDIO_OFF, pid);
			}
		}
		p.shown = want;
		p.waitKey &= want;
	}

	void SetCam(int pid, CamState state, int eventValue)
	{
		Peer& p = peers[pid];
		if (p.cam == state) return;
		p.cam = state;
		if (state != CamState::Live)
		{
			HideFromAll(pid);
			p.gop.clear();
			p.gopBytes = 0;
			p.gopValid = false;
			p.waitKey.reset();
		}
		else SyncViewers(pid);
		PushEvent(Event::Cam, pid, eventValue);
	}

	// игрок перестал быть зрителем у всех (отключился)
	void ForgetViewer(int v)
	{
		for (int pid = 0; pid < MAX_PEERS; pid++) { peers[pid].shown.reset(v); peers[pid].waitKey.reset(v); }
	}

	void Unlink(int pid, bool kick, const char* reason)
	{
		Peer& p = peers[pid];
		if (!p.linked) return;
		{
			char text[160];
			std::snprintf(text, sizeof(text), "[ModGear]: Вебкамера: игрок %d отключён (%s)", pid, reason);
			QueueLog(text);
		}
		if (p.cam == CamState::Live) PushEvent(Event::Cam, pid, 0);
		p.cam = CamState::Off;
		HideFromAll(pid);
		p.gop.clear();
		p.gopBytes = 0;
		p.gopValid = false;
		addrToPeer.erase(HashAddr(p.addr));
		if (kick) server->Kick(p.addr);
		p.linked = false;
		ForgetViewer(pid);
		PushEvent(Event::Unlinked, pid, 0);
	}

	// ---- приём
	void OnConnect(RakNet::Packet* packet)
	{
		const uint8_t* d = packet->data;
		const size_t len = packet->length;
		if (len < 1 + TICKET_LEN) return;
		const int pid = d[1] | (d[2] << 8);
		auto reject = [&](uint8_t reason)
		{
			std::vector<uint8_t> out = { ID_LINK_REJECTED, uint8_t(pid & 0xFF), uint8_t(pid >> 8), reason };
			SendRaw(packet->playerId, out, true);
		};
		if (!ValidPeer(pid)) { reject(1); return; }
		Peer& p = peers[pid];
		const int64_t now = NowMs();
		if (!p.ticket || now - p.ticketTime > TICKET_TTL_MS || std::memcmp(p.ticketData.data(), d + 1, TICKET_LEN) != 0)
		{
			reject(2); // обычно - переподключение со старым билетом, новый уже отправлен
			return;
		}
		// старое соединение этого игрока (переподключение) - закрыть
		if (p.linked && p.addr != packet->playerId) Unlink(pid, true, "новое соединение с другого адреса");
		auto old = addrToPeer.find(HashAddr(packet->playerId));
		if (old != addrToPeer.end() && old->second != pid) Unlink(old->second, false, "адрес занят другим игроком");

		p.ticket = false; // билет одноразовый
		p.linked = true;
		p.addr = packet->playerId;
		addrToPeer[HashAddr(p.addr)] = pid;
		// без этого RakNet open.mp выкинет соединение через 30 с ("didn't logon to the game")
		if (auto* remote = server->GetRemoteSystemFromPlayerID(packet->playerId)) remote->isLogon = true;

		const uint64_t serverTime = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();
		std::vector<uint8_t> out = { ID_LINK_LINKED, uint8_t(pid & 0xFF), uint8_t(pid >> 8) };
		for (int i = 0; i < 8; i++) out.push_back(uint8_t(serverTime >> (8 * i)));
		SendRaw(p.addr, out, true);
		PushEvent(Event::Linked, pid, 1);
		{
			char text[128];
			std::snprintf(text, sizeof(text), "[ModGear]: Вебкамера: игрок %d подключён", pid);
			QueueLog(text);
		}

		// камеру попросили до подключения - отправить VideoInit сейчас
		if (p.cam == CamState::Requested) { p.camRequestTime = now; SendCam(pid, CAM_INIT, pid); }
	}

	// video - кусок кадра (тип 6 или начало типа 12), nullptr - чистый звук
	void Relay(int pid, uint8_t type, const uint8_t* payload, size_t len, const uint8_t* video, size_t vlen)
	{
		Peer& p = peers[pid];
		if (p.cam != CamState::Live) return;
		p.statIn++;
		bool keyStart = false;
		if (video && vlen > VIDEO_FRAG_HEADER)
		{
			const uint32_t frameId = video[0] | (video[1] << 8) | (video[2] << 16) | (uint32_t(video[3]) << 24);
			const uint16_t index = video[4] | (video[5] << 8);
			if (index == 0)
			{
				p.curFrameId = frameId;
				p.curFrameKey = (video[VIDEO_FRAG_HEADER] & 1) == 0;
				keyStart = p.curFrameKey;
			}
			else if (frameId != p.curFrameId) p.curFrameKey = false;
		}
		for (int v = 0; v < MAX_PEERS; v++)
		{
			if (!p.shown.test(v)) continue;
			if (video)
			{
				if (keyStart) p.waitKey.reset(v);
				if (p.waitKey.test(v)) continue; // ждём начала ключевого кадра (звук из пачек тоже пропадает, до ~2 с)
			}
			// видео и звук - без подтверждений (UNRELIABLE). Клиент azrtc.asi не подтверждает (ACK) надёжные
			// пакеты потока: при RELIABLE_ORDERED сервер через ~10 с считал зрителя отключённым ("таймаут")
			// зритель переподключался и так по кругу - картинки не было. Порядок кусков клиент собирает сам
			SendCam(v, type, pid, payload, len, false);
			p.statOut++;
		}
	}

	// кусок видеокадра от транслирующего: запомнить, если это ключевой кадр
	void TrackVideoFragment(Peer& p, const uint8_t* f, size_t len)
	{
		if (len <= VIDEO_FRAG_HEADER) return;
		const uint16_t index = f[4] | (f[5] << 8), count = f[6] | (f[7] << 8);
		if (!count || index >= count || count > 1024) return;
		// VP8: младший бит первого байта кадра 0 - ключевой кадр, с него цепочка начинается заново
		if (index == 0 && (f[VIDEO_FRAG_HEADER] & 1) == 0)
		{
			// ключевой кадр VP8: 3 байта тега, 9D 01 2A, ширина и высота по 14 бит. Зритель (azrtc.asi)
			// показывает только 240x240 - другое разрешение он отбрасывает, картинки не будет
			const uint8_t* k = f + VIDEO_FRAG_HEADER;
			if (len >= VIDEO_FRAG_HEADER + 10 && k[3] == 0x9D && k[4] == 0x01 && k[5] == 0x2A)
			{
				const int w = (k[6] | (k[7] << 8)) & 0x3FFF, h = (k[8] | (k[9] << 8)) & 0x3FFF;
				if (w != p.lastW || h != p.lastH)
				{
					p.lastW = w; p.lastH = h;
					if (w != 240 || h != 240)
					{
						char text[160];
						std::snprintf(text, sizeof(text), "[ModGear]: Вебкамера: камера шлёт %dx%d - зрители Arizona видят только 240x240", w, h);
						QueueLog(text, true);
					}
				}
			}
			p.gop.clear();
			p.gopBytes = 0;
			p.gopValid = true;
		}
		if (!p.gopValid) return;
		p.gopBytes += len;
		if (p.gopBytes > GOP_MAX_BYTES) { p.gop.clear(); p.gopBytes = 0; p.gopValid = false; return; } // слишком длинная цепочка - ждём ключевой
		p.gop.emplace_back(f, f + len);
	}

	void OnWebcam(int pid, const uint8_t* d, size_t len)
	{
		if (len < 1) return;
		Peer& p = peers[pid];
		const uint8_t type = d[0];
		switch (type)
		{
		case CAM_READY:
			if (p.cam == CamState::Requested) SetCam(pid, CamState::Live, 1);
			break;
		case CAM_STOPPED:
		case CAM_FAILED:
			if (p.cam == CamState::Requested) SetCam(pid, CamState::Off, -1);
			else if (p.cam == CamState::Live) SetCam(pid, CamState::Off, 0);
			break;
		case CAM_AUDIO:
		case CAM_VIDEO:
		case CAM_BATCH:
			// клиент отвечает "готов" (7) только на повторный VideoInit, а на первый сразу шлёт видео и звук
			if (p.cam == CamState::Requested) SetCam(pid, CamState::Live, 1);
			{
				const uint8_t* video = nullptr;
				size_t vlen = 0;
				if (type == CAM_AUDIO) { video = d + 1; vlen = len - 1; }
				else if (type == CAM_BATCH && len >= 3)
				{
					vlen = d[1] | (d[2] << 8);
					if (vlen && 3 + vlen <= len) video = d + 3; else vlen = 0;
				}
				if (video) TrackVideoFragment(p, video, vlen);
				Relay(pid, type, d + 1, len - 1, video, vlen);
			}
			break;
		default:
			break;
		}
	}

	void OnPacket(RakNet::Packet* packet)
	{
		if (!packet->data || packet->length < 1) return;
		const uint8_t id = packet->data[0];
		if (id == ID_LINK_CONNECT) { OnConnect(packet); return; }

		auto it = addrToPeer.find(HashAddr(packet->playerId));
		const int pid = it == addrToPeer.end() ? -1 : it->second;

		if (id == RakNet::ID_NEW_INCOMING_CONNECTION) return; // дальше ждём CONNECT с билетом
		if (id == RakNet::ID_DISCONNECTION_NOTIFICATION || id == RakNet::ID_CONNECTION_LOST)
		{
			if (pid != -1) Unlink(pid, false, id == RakNet::ID_CONNECTION_LOST ? "связь потеряна (таймаут)" : "клиент закрыл соединение");
			return;
		}
		if (pid == -1) return;
		if (id == ID_LINK_DATA && packet->length >= 2 && packet->data[1] == CHANNEL_WEBCAM)
			OnWebcam(pid, packet->data + 2, packet->length - 2);
	}

	void RunCommands()
	{
		std::vector<Command> list;
		{
			std::lock_guard<std::mutex> lock(cmdMx);
			list.swap(commands);
		}
		const int64_t now = NowMs();
		for (Command& c : list)
		{
			if (!ValidPeer(c.playerid)) continue;
			Peer& p = peers[c.playerid];
			switch (c.type)
			{
			case Command::Ticket:
				p.ticket = true;
				p.ticketData = c.ticket;
				p.ticketTime = now;
				break;
			case Command::StartCam:
				if (p.cam != CamState::Off) break;
				p.cam = CamState::Requested;
				p.camRequestTime = now;
				if (p.linked) SendCam(c.playerid, CAM_INIT, c.playerid);
				break;
			case Command::StopCam:
				if (p.cam == CamState::Off) break;
				SendCam(c.playerid, CAM_STOP_SHOW, c.playerid);
				SetCam(c.playerid, CamState::Off, 0);
				break;
			case Command::Viewers:
				p.viewers.swap(c.viewers);
				SyncViewers(c.playerid);
				break;
			case Command::Drop:
				p.ticket = false;
				if (p.linked) Unlink(c.playerid, true, "вышел с сервера");
				else if (p.cam != CamState::Off) SetCam(c.playerid, CamState::Off, 0);
				p.viewers.clear();
				break;
			}
		}
	}

	void CheckTimeouts()
	{
		const int64_t now = NowMs();
		for (int pid = 0; pid < MAX_PEERS; pid++)
		{
			Peer& p = peers[pid];
			if (p.ticket && now - p.ticketTime > TICKET_TTL_MS) p.ticket = false;
			// клиент не ответил на VideoInit (нет AZRTC / камера так и не открылась)
			if (p.cam == CamState::Requested && p.linked && now - p.camRequestTime > CAM_REQUEST_TIMEOUT_MS)
				SetCam(pid, CamState::Off, -1);
		}
	}

	void Loop()
	{
		int64_t lastCookie = NowMs(), lastCheck = 0, lastStats = NowMs(), lastCamStats = NowMs();
		while (running)
		{
			RunCommands();
			while (RakNet::Packet* packet = server->Receive())
			{
				OnPacket(packet);
				server->DeallocatePacket(packet);
			}
			const int64_t now = NowMs();
			if (now - lastCheck > 500) { CheckTimeouts(); lastCheck = now; }
			if (now - lastCamStats > 10000)
			{
				lastCamStats = now;
				for (int pid = 0; pid < MAX_PEERS; pid++)
				{
					Peer& p = peers[pid];
					if (p.cam != CamState::Live) continue;
					char text[200];
					p.statIn = p.statOut = 0;
					// у зрителя копится очередь отправки - картинка и звук отстают (сеть / канал зрителя не тянет)
					for (int v = 0; v < MAX_PEERS; v++)
					{
						if (!p.shown.test(v) || !peers[v].linked) continue;
						const RakNet::RakNetStatisticsStruct* st = server->GetStatistics(peers[v].addr);
						if (!st) continue;
						unsigned queued = 0;
						for (int i = 0; i < RakNet::NUMBER_OF_PRIORITIES; i++) queued += st->messageSendBuffer[i];
						if (queued < 50) continue;
						std::snprintf(text, sizeof(text), "[ModGear]: Вебкамера: у игрока %d не тянет сеть - картинка и звук отстают (в очереди %u)", v, queued);
						QueueLog(text, true);
					}
				}
			}
			// диагностика: доходят ли до порта пакеты клиентов и расшифровываются ли они
			if (now - lastStats > 60000)
			{
				lastStats = now;
				const uint32_t raw = SAMPRakNet::rawPackets_.exchange(0), bad = SAMPRakNet::badPackets_.exchange(0);
				if (raw && bad) // только когда что-то не так: пакеты есть, но не разбираются
				{
					const uint32_t a = SAMPRakNet::lastAddress_;
					char text[200];
					std::snprintf(text, sizeof(text), "[ModGear]: Вебкамера: пакеты не расшифровываются (%u из %u, последний от %u.%u.%u.%u)",
						bad, raw, a & 0xFF, (a >> 8) & 0xFF, (a >> 16) & 0xFF, a >> 24);
					QueueLog(text);
				}
			}
			if (now - lastCookie > COOKIE_RESEED_MS) { SAMPRakNet::SeedCookie(); lastCookie = now; }
			std::this_thread::sleep_for(std::chrono::milliseconds(2));
		}
	}
}

void QueueLog(const char* text, bool important)
{
	std::lock_guard<std::mutex> lock(evMx);
	events.push_back({ Event::Log, -1, important ? 1 : 0, text });
}

bool Start(int port, const std::string& bindAddress, int gamePort)
{
	if (running) return true;
	if (port <= 0 || port > 65535) return false;

	SAMPRakNet::Init(RakCoreShim::Get());
	SAMPRakNet::SeedToken();
	// видео идёт десятками пакетов в секунду (кадр больше MTU режется на части) - лимиты open.mp
	// по умолчанию рассчитаны на синхронизацию игрока и банили бы за камеру
	SAMPRakNet::SetMessagesLimit(20000);
	SAMPRakNet::SetMessageHoleLimit(20000);
	SAMPRakNet::SetAcksLimit(20000);

	server = RakNet::RakNetworkFactory::GetRakServerInterface();
	if (!server) return false;
	// azrtc.asi разбирает из датаграммы не все сообщения - шлём по одному (см. ReliabilityLayer.h)
	RakNet::ReliabilityLayer::oneMessagePerDatagram = true;
	server->SetMTUSize(576); // только до Start - после него RakNet размер не меняет
	const char* bind = (bindAddress.empty() || bindAddress == "0.0.0.0") ? nullptr : bindAddress.c_str();
	if (!server->Start(MAX_PEERS, 0, 5, (unsigned short)port, bind))
	{
		RakNet::RakNetworkFactory::DestroyRakServerInterface(server);
		server = nullptr;
		return false;
	}
	server->StartOccasionalPing();
	// ключ расшифровки пакетов клиента SA-MP зависит от порта. samp.dll хранит один порт на все соединения
	// порт игрового сервера, поэтому AZRTC шифрует им; свой порт - запасной (тест, другие клиенты)
	SAMPRakNet::SetPort((uint16_t)port);
	SAMPRakNet::SetPlainPackets(true); // у azrtc.asi свой RakNet, шифрование SA-MP в нём пустое - пакеты приходят как есть
	SAMPRakNet::SetSkipAuth(true); // ID_AUTH_KEY второму соединению samp.dll отвечать некому
	SAMPRakNet::SetAltPort((uint16_t)(gamePort > 0 && gamePort < 65536 ? gamePort : 0));
	SAMPRakNet::SeedCookie();
	// поток камеры ~300 кбит/с, а RakNet open.mp начинает соединение с 2 x 96 кбит/с - иначе кадры копятся в очереди
	SAMPRakNet::SetMinimumSendBitsPerSecond(1000000.0f);

	for (int i = 0; i < MAX_PEERS; i++)
	{
		peers[i] = Peer();
		linkedMain[i] = false;
		camMain[i] = CamState::Off;
	}
	addrToPeer.clear();
	serverPort = port;
	running = true;
	worker = std::thread(Loop);
	return true;
}

void Stop()
{
	if (!running) return;
	running = false;
	if (worker.joinable()) worker.join();
	server->Disconnect(100);
	RakNet::RakNetworkFactory::DestroyRakServerInterface(server);
	server = nullptr;
	serverPort = 0;
	std::lock_guard<std::mutex> lock(cmdMx);
	commands.clear();
}

bool Running() { return running; }
int Port() { return serverPort; }

bool MakeTicket(int playerid, const std::string& host, std::vector<uint8_t>& out)
{
	if (!running || !ValidPeer(playerid) || host.empty() || host.size() > 255) return false;
	static std::random_device rd;
	Command cmd{ Command::Ticket, playerid };
	auto& t = cmd.ticket;
	t[0] = uint8_t(playerid & 0xFF);
	t[1] = uint8_t(playerid >> 8);
	for (size_t i = 2; i < t.size(); i += 4)
	{
		const uint32_t r = rd();
		for (size_t k = 0; k < 4 && i + k < t.size(); k++) t[i + k] = uint8_t(r >> (8 * k));
	}
	// u64 после токена - время выдачи (клиенту всё равно, он возвращает билет как есть)
	const uint64_t stamp = (uint64_t)std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
	for (int i = 0; i < 8; i++) t[2 + TOKEN_LEN + i] = uint8_t(stamp >> (8 * i));

	// RPC 222: [билет 58 байт][u8 len + host][u16 port][u8 len + строка 2 (пусто)]
	out.assign(t.begin(), t.end());
	out.push_back(uint8_t(host.size()));
	out.insert(out.end(), host.begin(), host.end());
	out.push_back(uint8_t(serverPort & 0xFF));
	out.push_back(uint8_t(serverPort >> 8));
	out.push_back(0);
	PushCommand(std::move(cmd));
	return true;
}

bool IsLinked(int playerid) { return ValidPeer(playerid) && linkedMain[playerid]; }
CamState GetCam(int playerid) { return ValidPeer(playerid) ? camMain[playerid] : CamState::Off; }

bool StartCam(int playerid)
{
	if (!running || !ValidPeer(playerid)) return false;
	if (camMain[playerid] != CamState::Off) return true;
	camMain[playerid] = CamState::Requested;
	PushCommand({ Command::StartCam, playerid });
	return true;
}

bool StopCam(int playerid)
{
	if (!running || !ValidPeer(playerid)) return false;
	camMain[playerid] = CamState::Off;
	PushCommand({ Command::StopCam, playerid });
	return true;
}

void SetViewers(int playerid, const std::vector<int>& viewers)
{
	if (!running || !ValidPeer(playerid)) return;
	Command cmd{ Command::Viewers, playerid };
	cmd.viewers = viewers;
	PushCommand(std::move(cmd));
}

void PlayerLeft(int playerid)
{
	if (!ValidPeer(playerid)) return;
	linkedMain[playerid] = false;
	camMain[playerid] = CamState::Off;
	if (running) PushCommand({ Command::Drop, playerid });
}

void PollEvents(std::vector<Event>& out)
{
	{
		std::lock_guard<std::mutex> lock(evMx);
		out.swap(events);
		events.clear();
	}
	for (const Event& e : out)
	{
		if (!ValidPeer(e.playerid)) continue;
		if (e.type == Event::Linked) linkedMain[e.playerid] = true;
		else if (e.type == Event::Unlinked) { linkedMain[e.playerid] = false; camMain[e.playerid] = CamState::Off; }
		else if (e.type == Event::Cam) camMain[e.playerid] = e.value == 1 ? CamState::Live : CamState::Off;
	}
}
}
