#include "Events.h"
#include "Platform.h"
#include "Afk.h"
#include "Version.h"

#include <cstring>
#include <string>
#include <bitset>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <raknet/bitstream.h>
#include <lib/InfoServer.h>
#include <lib/Native.h>
#include <lib/Handling.h>
#include <lib/Video.h>
#include <lib/Voice.h>
#include <lib/Webcam.h>
#include <lib/ArzExtra.h>
#include <lib/ArzChat.h>
#include <lib/Peds.h>
#include <lib/ArzCore.h>
#include <lib/CefResponse.h>
#include <lib/ScreenSize.h>
#include <lib/StrCompress.h>
#include "Media.h"
#include "Plugin.h"

namespace
{
	unsigned char fullWeapon[MAX_PLAYERS]; // полный ID оружия игрока из его синхронизации (кастомное оружие Arizona)

	uint32_t ReplaceSkin(int viewerid, uint32_t skin)
	{
		if (PI[viewerid].pLauncherStat || skin < 312) return skin;
		return Native::CallingBackSkin(skin);
	}
}

// ---- досылка состояния при появлении в зоне стрима
// Состояние (детали, диски, материалы, номера, аксессуары...) шлём сразу после RPC создания машины/игрока
namespace
{
	// повторы после появления: клиент SA-MP создаёт машину, только когда догрузит её модель (после
	// телепорта - не сразу), а пакет для ещё не созданной машины выбрасывает. Частые повторы - тюнинг
	// виден почти сразу, как машина появилась; повтор безопасен (детали и материалы перезаписываются)
	constexpr int64_t STREAM_RETRY_MS[] = { 100, 300, 700, 1500 };
	constexpr int STREAM_RETRIES = sizeof(STREAM_RETRY_MS) / sizeof(STREAM_RETRY_MS[0]);
	struct Pending { bool vehicle; int id; int viewer; int64_t start; int step; };
	std::vector<Pending> pending;

	int64_t NowMs()
	{
		return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	}

	void Schedule(bool vehicle, int id, int viewer)
	{
		for (Pending& p : pending)
			if (p.vehicle == vehicle && p.id == id && p.viewer == viewer) { p.start = NowMs(); p.step = 0; return; }
		pending.push_back({ vehicle, id, viewer, NowMs(), 0 });
	}

	// Модели, которые игрок недавно видел: GTA держит их загруженными, машина / игрок у клиента появляется
	// сразу, и состояние доходит с первого раза - повторы не нужны. Повторы - только для модели, новой
	// для игрока (после телепорта в другой район, первая такая машина): её клиент ещё грузит
	constexpr int64_t MODEL_WARM_MS = 120000; // модель не видел дольше - могла выгрузиться
	constexpr int64_t MODEL_LOAD_MS = 2000;   // после первого появления модель точно успела загрузиться
	constexpr int SKIN_KEY = 100000;          // ключи скинов отдельно от моделей машин
	struct Seen { int64_t first = 0, last = 0; };
	std::unordered_map<int, Seen> seenModels[MAX_PLAYERS];

	// true - модель у игрока уже загружена: впервые видел её не меньше MODEL_LOAD_MS назад и с тех пор
	// не пропадал дольше MODEL_WARM_MS. Пять одинаковых машин сразу после телепорта - все с повторами
	bool ModelWarm(int viewer, int model)
	{
		if (model <= 0) return false;
		const int64_t now = NowMs();
		Seen& v = seenModels[viewer][model];
		if (!v.first || now - v.last > MODEL_WARM_MS) v.first = now;
		v.last = now;
		return now - v.first >= MODEL_LOAD_MS;
	}

	void Unschedule(bool vehicle, int id, int viewer)
	{
		for (size_t i = 0; i < pending.size(); i++)
			if (pending[i].vehicle == vehicle && pending[i].id == id && (viewer == -1 || pending[i].viewer == viewer))
				pending[i--] = pending.back(), pending.pop_back();
	}
}

// ---- ответ на диалог: listitem как у SA-MP
// Клиент Arizona в ответе на поле ввода / пароль / сообщение шлёт listitem 0, а SA-MP шлёт -1
namespace
{
	constexpr int RPC_SHOW_DIALOG = 61;
	constexpr int RPC_DIALOG_RESPONSE = 62;
	constexpr uint16_t DIALOG_ID_ARIZONA = 1;
	constexpr uint16_t DIALOG_ID_ALIAS = 32767;
	int8_t dialogStyle[MAX_PLAYERS]; // стиль последнего показанного диалога, -1 - неизвестен
	// заголовок открытого диалога (для SetPlayerDialogText): ID как у сервера (до замены 1 -> 32767)
	struct OpenDialog { bool open = false; uint16_t id = 0; uint8_t style = 0; std::string title, button1, button2; };
	OpenDialog openDialog[MAX_PLAYERS];

	bool ReadStr8(BitStream& bs, std::string& out)
	{
		uint8_t len = 0;
		if (!bs.Read(len)) return false;
		out.assign(len, ' ');
		return !len || bs.Read(&out[0], len);
	}

	bool DialogHasList(int style) { return style == 2 || style == 4 || style == 5; } // LIST, TABLIST, TABLIST_HEADERS
}

void Events::OnPlayerConnect(int playerid, bool mobile)
{
	if (playerid < 0 || playerid >= MAX_PLAYERS) return;
	connectedPlayers.insert(playerid);
	Afk::Joined(playerid); // пакеты плагина - когда игрок будет в игре (не свёрнут)
	dialogStyle[playerid] = -1;
	openDialog[playerid] = OpenDialog{};
	for (int i = 0; i < 10; i++) Native::ResetAcs(playerid, i, -1);
	PI[playerid].pLauncherStat = 0;
	PI[playerid].pStatusLoadCef = 0;
	PI[playerid].pCefStarted = 0;
	PI[playerid].pHints = 0;
	PI[playerid].pMobile = mobile;
	PI[playerid].pSpeed = 0;
	PI[playerid].pRadio = -1;
	PI[playerid].pHud = 0;
	memset(PI[playerid].pTag, 0, sizeof(PI[playerid].pTag));
	PI[playerid].pLoadRadioCheck = false;
	PI[playerid].pChatOpen = 0;
	PI[playerid].pChat = 0;
	PI[playerid].pColision = 0;
	PI[playerid].pTagStatus = false;
	// индексы - HOT_KEY_* из include: рация R, рации фракции и семьи нет, взаимодействие R, телефон P
	strcpy(PI[playerid].pHotKey[0], "R");
	strcpy(PI[playerid].pHotKey[1], "null");
	strcpy(PI[playerid].pHotKey[2], "null");
	strcpy(PI[playerid].pHotKey[3], "R");
	strcpy(PI[playerid].pHotKey[4], "P");
}

void Events::OnPlayerDisconnect(int playerid)
{
	if (playerid >= 0 && playerid < MAX_PLAYERS) fullWeapon[playerid] = 0;
	if (playerid < 0 || playerid >= MAX_PLAYERS) return;
	Afk::Left(playerid);
	Unschedule(false, playerid, -1);
	seenModels[playerid].clear();
	for (size_t i = 0; i < pending.size(); i++) if (pending[i].viewer == playerid) pending[i--] = pending.back(), pending.pop_back();
	PI[playerid].pLauncherStat = 0;
	PI[playerid].pCefStarted = 0;
#if MODGEAR_FULL
	Video::PlayerLeft(playerid);
	Voice::PlayerLeft(playerid);
	Webcam::PlayerLeft(playerid);
	ArzExtra::PlayerLeft(playerid);
	ArzChat::PlayerLeft(playerid);
	ArzCore::PlayerLeft(playerid);
	ScreenSize::PlayerLeft(playerid);
	Peds::PlayerLeft(playerid);
	if (PI[playerid].pRadio != -1) Native::Audio_Free(PI[playerid].pRadio); // удалить зону у всех, кто его видел
	for (int i = 0; i < (int)aTemp.size(); i++)
		if (aTemp[i].aType == 1 && aTemp[i].aUid == playerid) Native::Audio_Free(i); // его фоновая музыка
#endif
	PI[playerid].pRadio = -1;
	connectedPlayers.erase(playerid);
}

static void SendPlayerState(int viewerid, int playerid, bool audio = true)
{
#if MODGEAR_FULL
	if (!PI[viewerid].pLauncherStat) return;
	ArzExtra::OnPlayerStreamIn(viewerid, playerid); // грязь аксессуаров
	ArzCore::OnPlayerStreamIn(viewerid, playerid); // походка
	for (int i = 0; i < 10; i++)
	{
		if (PI[playerid].pAcs_Model[i] != -1 && PI[playerid].pAcs_bone[i])
		{
			Native::AttachPlyerAcs(viewerid, playerid, i, PI[playerid].pAcs_create[i], PI[playerid].pAcs_bone[i], PI[playerid].pAcs_Model[i],
				PI[playerid].pAcs_OffsetX[i], PI[playerid].pAcs_OffsetY[i], PI[playerid].pAcs_OffsetZ[i],
				PI[playerid].pAcs_fRotX[i], PI[playerid].pAcs_fRotY[i], PI[playerid].pAcs_fRotZ[i],
				PI[playerid].pAcs_fScaleX[i], PI[playerid].pAcs_fScaleY[i], PI[playerid].pAcs_fScaleZ[i],
				PI[playerid].pAcs_color_1[i], PI[playerid].pAcs_color_2[i]);
		}
	}
	if (strlen(PI[playerid].pTag)) Native::b_ShowPlayerTag(playerid, viewerid, (PI[viewerid].pTagStatus = true));
	if (PI[playerid].pColision) Native::b_SetPlayerCollisionStatus(playerid, viewerid, 1);
	// звук игрока: зона и трек тем, у кого он появился
	if (audio && PI[playerid].pRadio != -1)
	{
		Native::Audio_SendZone(PI[playerid].pRadio, viewerid);
		if (!aTemp[PI[playerid].pRadio].aPause) Native::Audio_SendPlay(PI[playerid].pRadio, viewerid);
	}
#endif
}

void Events::OnPlayerStreamOut(int viewerid, int playerid)
{
	Unschedule(false, playerid, viewerid);
#if MODGEAR_FULL
	if (!PI[viewerid].pLauncherStat) return;
	if (PI[playerid].pColision) Native::b_SetPlayerCollisionStatus(viewerid, playerid, 0);
	if (PI[playerid].pTagStatus) Native::b_ShowPlayerTag(playerid, viewerid, (PI[viewerid].pTagStatus = false));
	if (PI[playerid].pRadio != -1) Native::Audio_SendDestroy(PI[playerid].pRadio, viewerid);
#endif
}

static void SendVehicleState(int veh, int playerid, bool audio = true)
{
	if (veh <= 0 || veh >= MAX_VEHICLES || !PI[playerid].pLauncherStat) return;
	// свой цвет (материал Arizona) - только если его задали через плагин, иначе машина своего цвета SA-MP
	if (vTemp[veh].vColorCustom && !vTemp[veh].vVinil) Native::b_SetVehicleColor(veh, vTemp[veh].vColor, playerid);
#if MODGEAR_FULL
	// как у Arizona: машина появилась у игрока -> зона звука, и трек, если он сейчас играет
	if (audio && vTemp[veh].vUIDradio != -1)
	{
		Native::Audio_SendZone(vTemp[veh].vUIDradio, playerid);
		if (!aTemp[vTemp[veh].vUIDradio].aPause) Native::Audio_SendPlay(vTemp[veh].vUIDradio, playerid);
	}
	if (vTemp[veh].vDiski) Native::b_InstallVehicleComponent(veh, 4, vTemp[veh].vDiski, playerid);
	if (vTemp[veh].vWheel_1[0]) Native::b_WheelSettingsCustom(veh, 0, vTemp[veh].vWheel_1[0], vTemp[veh].vWheel_1[1], playerid);
	if (vTemp[veh].vWheel_2[0]) Native::b_WheelSettingsCustom(veh, 1, vTemp[veh].vWheel_2[0], vTemp[veh].vWheel_2[1], playerid);
	if (vTemp[veh].vDiameteWheel) Native::b_CustomizeCarWheel(veh, vTemp[veh].vDiameteWheel, playerid);
	if (vTemp[veh].vNeon != -1) Native::b_NeonColorChange(veh, vTemp[veh].vNeon, playerid);
	if (vTemp[veh].vHidravlic) Native::b_SetVehicleHidraulics(veh, vTemp[veh].vHidravlic, playerid);
	if (vTemp[veh].vHandlingMask) Handling::Send(veh, Handling::Stored(veh), playerid);
	if (vTemp[veh].vColorShift != 1) Native::b_SmokeColorShift(veh, vTemp[veh].vColorShift, playerid);
	if (vTemp[veh].vHeaderlamp != 1) Native::b_SetColorHeadlamp(veh, vTemp[veh].vHeaderlamp, playerid);
	if (vTemp[veh].vOtstrel) Native::b_SetShotsExhaust(veh, vTemp[veh].vOtstrel, playerid);
	// тонировка, детали, номер и винил - для любой машины, а не только для кастомных моделей (> 611)
	for (int i = 0; i < 7; i++)
	{
		if (vTemp[veh].vToner[i]) Native::b_VehicleToneCustomizer(veh, i, vTemp[veh].vToner_red[i], vTemp[veh].vToner_green[i], vTemp[veh].vToner_blue[i], vTemp[veh].vToner_size[i], playerid);
	}
	for (int slot = 1; slot < PART_SLOTS; slot++)
		if (slot != PART_WHEELS && vTemp[veh].vParts[slot]) Native::b_InstallVehicleComponent(veh, slot, vTemp[veh].vParts[slot], playerid);
	if (vTemp[veh].vNumberType) Native::b_InstallNumberVehicle(veh, vTemp[veh].vNumberType, vTemp[veh].vNumber, vTemp[veh].vRegion, playerid);
	// винил 0 - это просто цвет, он уже отправлен выше
	if (vTemp[veh].vVinil) Native::b_SetVinylCustomization(veh, vTemp[veh].vVinil, vTemp[veh].vColor, playerid);
	if (vTemp[veh].vStrobe) Native::b_SetVehicleStrobeLights(veh, vTemp[veh].vStrobe, vTemp[veh].vStrobeSpeed, vTemp[veh].vStrobeBeam, playerid);
	if (vTemp[veh].vCalipers) Native::b_SetVehicleBrakeCalipers(veh, vTemp[veh].vCalipers, vTemp[veh].vCalipersSimple, playerid);
	if (vTemp[veh].vDrift) Native::b_SetVehicleDrift(veh, true, playerid);
	if (vTemp[veh].vSiren) Native::b_SetVehicleSiren(veh, true, playerid);
	ArzExtra::OnVehicleStreamIn(veh, playerid); // грязь/износ, цвет поворотников
	if (vTemp[veh].vNitro || vTemp[veh].vNitroColor >= 0) ArzCore::SendVehicleNitro(veh, playerid);
	if (vTemp[veh].vSilentEngine) ArzCore::SendSilentEngine(veh, playerid);
	if (!vTemp[veh].vHeadlightTexture.empty()) ArzCore::SendHeadlightTexture(veh, playerid);
#endif
}

void Events::OnPlayerStreamIn(int viewerid, int playerid)
{
	if (viewerid < 0 || viewerid >= MAX_PLAYERS || playerid < 0 || playerid >= MAX_PLAYERS || !PI[viewerid].pLauncherStat) return;
	SendPlayerState(viewerid, playerid);
	if (!ModelWarm(viewerid, SKIN_KEY + PI[playerid].pSkin)) Schedule(false, playerid, viewerid);
}

void Events::OnVehicleStreamIn(int veh, int playerid)
{
	if (veh <= 0 || veh >= MAX_VEHICLES || playerid < 0 || playerid >= MAX_PLAYERS || !PI[playerid].pLauncherStat) return;
	SendVehicleState(veh, playerid);
	const int model = vTemp[veh].vModelLuxe ? vTemp[veh].vModelLuxe : vTemp[veh].vDefaultModel;
	if (!ModelWarm(playerid, model)) Schedule(true, veh, playerid);
}

static void RunPending()
{
	if (pending.empty()) return;
	const int64_t now = NowMs();
	for (size_t i = 0; i < pending.size(); i++)
	{
		Pending& e = pending[i];
		if (now - e.start < STREAM_RETRY_MS[e.step]) continue;
		const Pending p = e;
		if (++e.step >= STREAM_RETRIES) pending[i--] = pending.back(), pending.pop_back();
		if (!Platform::IsPlayerConnected(p.viewer) || !PI[p.viewer].pLauncherStat) continue;
		if (p.vehicle)
		{
			if (Platform::IsVehicleValid(p.id) && Platform::IsVehicleStreamedIn(p.id, p.viewer)) SendVehicleState(p.id, p.viewer, false);
		}
		else if (Platform::IsPlayerConnected(p.id) && Platform::IsPlayerStreamedIn(p.id, p.viewer)) SendPlayerState(p.viewer, p.id, false);
	}
}

void Events::ResyncStreamed(int playerid)
{
	if (playerid < 0 || playerid >= MAX_PLAYERS || !PI[playerid].pLauncherStat) return;
	for (int veh = 1; veh < MAX_VEHICLES; veh++)
		if (Platform::IsVehicleValid(veh) && Platform::IsVehicleStreamedIn(veh, playerid)) OnVehicleStreamIn(veh, playerid);
	for (int other : connectedPlayers)
		if (other != playerid && Platform::IsPlayerStreamedIn(other, playerid)) OnPlayerStreamIn(playerid, other);
#if MODGEAR_FULL
	// свои аксессуары игрок видит всегда
	for (int i = 0; i < 10; i++)
	{
		if (PI[playerid].pAcs_Model[i] == -1 || !PI[playerid].pAcs_bone[i]) continue;
		Native::AttachPlyerAcs(playerid, playerid, i, PI[playerid].pAcs_create[i], PI[playerid].pAcs_bone[i], PI[playerid].pAcs_Model[i],
			PI[playerid].pAcs_OffsetX[i], PI[playerid].pAcs_OffsetY[i], PI[playerid].pAcs_OffsetZ[i],
			PI[playerid].pAcs_fRotX[i], PI[playerid].pAcs_fRotY[i], PI[playerid].pAcs_fRotZ[i],
			PI[playerid].pAcs_fScaleX[i], PI[playerid].pAcs_fScaleY[i], PI[playerid].pAcs_fScaleZ[i],
			PI[playerid].pAcs_color_1[i], PI[playerid].pAcs_color_2[i]);
	}
#endif
}

void Events::OnVehicleStreamOut(int veh, int playerid)
{
	Unschedule(true, veh, playerid);
#if MODGEAR_FULL
	if (veh <= 0 || veh >= MAX_VEHICLES) return;
	// машина пропала у игрока (или удалена с сервера) -> удалить у него зону звука
	if (vTemp[veh].vUIDradio != -1) Native::Audio_SendDestroy(vTemp[veh].vUIDradio, playerid);
#endif
}

void Events::OnIncomingCustomPacket(int playerid, BitStream& bs)
{
#if MODGEAR_FULL
	if (playerid < 0 || playerid >= MAX_PLAYERS) return;
	uint8_t custom = 0;
	if (!bs.Read(custom)) return;
#ifdef MODGEAR_DEBUG_CEF
	Platform::Log(Platform::LogLevel::Message, "[ModGear] 220:%d from %d", custom, playerid);
#endif
	switch (custom)
	{
		case 1: // открыт / закрыт чат
		{
			uint8_t statuschat = 0;
			bs.Read(statuschat);
			PI[playerid].pChatOpen = statuschat;
			if (!PI[playerid].pChat) break;
			for (int i : connectedPlayers)
			{
				if (i != playerid && PI[i].pLauncherStat && Platform::IsPlayerStreamedIn(playerid, i))
					Native::SendChatTyping(i, playerid, PI[playerid].pChatOpen);
			}
			break;
		}
		case 20: // размеры экрана, клиент готов
		{
			bs.Read(PI[playerid].pEkranWindows[0]);
			bs.Read(PI[playerid].pEkranWindows[1]);
			bs.IgnoreBits(16);
			bs.Read(PI[playerid].pEkranWindows[2]);
			bs.Read(PI[playerid].pEkranWindows[3]);
			Native::LoadFileCef(playerid, 0, "file:///frontend/svelte_js/index.html", "or7lq8Q8heur");
			Native::LoadFileCef(playerid, 2, "", "");
			Native::ShowCustom(playerid, { 8, 0 });
			PI[playerid].pCefStarted = true;
			break;
		}
		case 26: // грязь машины от водителя: float 0..100 -> остальным, кто видит машину
		{
			float dirt = 0.0f;
			if (bs.Read(dirt)) ArzExtra::OnClientDirt(playerid, dirt);
			break;
		}
		case 57: // позиция курсора (после SetPlayerMouseTracking): float x, float y
		{
			float mx = 0.0f, my = 0.0f;
			if (!bs.Read(mx) || !bs.Read(my)) break;
			int ix, iy; memcpy(&ix, &mx, 4); memcpy(&iy, &my, 4);
			Plugin::CallPublic("OnPlayerMouseMove", { playerid, ix, iy });
			break;
		}
		case 2: // поворотники от водителя (Q / E): u8 0 выкл, 1 левый, 2 правый, 3 аварийка
			// Другим игрокам их синхронизирует сам клиент - сервер только запоминает и сообщает в скрипт
		{
			uint8_t state = 0;
			if (!bs.Read(state) || state > 3) break;
			int seat = -1;
			const int veh = Platform::GetPlayerVehicle(playerid, &seat);
			if (veh <= 0 || veh >= MAX_VEHICLES || seat != 0 || vTemp[veh].vTurnSignal == state) break;
			vTemp[veh].vTurnSignal = state;
			Plugin::CallPublic("OnVehicleTurnSignal", { veh, playerid, (int)state });
			break;
		}
		case 115: // прицеливание от первого лица (после SetPlayerFirstPersonAim): bit вошёл / вышел
		{
			bool aiming = false;
			if (bs.Read(aiming)) Plugin::CallPublic("OnPlayerFirstPersonAim", { playerid, aiming ? 1 : 0 });
			break;
		}
		case 198: // дошёл до точки маршрута навигатора (SetPlayerNavigation): u8 индекс
		{
			uint8_t index = 0;
			if (bs.Read(index)) Plugin::CallPublic("OnPlayerNavigationPoint", { playerid, (int)index });
			break;
		}
		case 18: // событие из CEF
		{
			uint16_t length = 0; // клиент шлёт u16 длина, текст, u32 uid (cef.SendMessage)
			if (!bs.Read(length) || length > 8192) break;
			std::string event(length, '\0');
			if (length && !bs.Read(&event[0], length)) break;
			uint32_t uid = 0;
			bs.Read(uid);
			if (ScreenSize::OnCefEvent(playerid, event)) break; // ответ на RequestPlayerScreenSize - не событие мода
			Plugin::CallPublicStr("OnPlayerCefEvent", playerid, event.c_str(), (int)uid);
			CefResponse::Dispatch(playerid, event); // то же, разобранное: OnPlayerCefResponse + Cef_GetArg*
#ifdef MODGEAR_DEBUG_CEF
			Platform::Log(Platform::LogLevel::Message, "[ModGear] 220:18 from %d: %s", playerid, event.c_str());
#endif
			// новый клиент Arizona: CEF с главным меню открывает сам и 220:20 не шлёт
			// Первые события от него - onSvelteAppVersion|... и mainMenu.*
			if (!PI[playerid].pCefStarted && (event.rfind("onSvelteAppVersion", 0) == 0 || event.rfind("mainMenu.", 0) == 0))
			{
				PI[playerid].pCefStarted = true;
				break;
			}
			if (PI[playerid].pStatusLoadCef) break;
			if (event.find("onSvelteAppInit") != std::string::npos)
			{
				// закрыть главное меню клиента (так делает и сервер Arizona после onSvelteAppInit)
				Native::ShowCef(playerid, "window.executeEvent('event.setActiveView', '[ null ]');");
				Native::b_InterfaceCEF(playerid, Custom_Hud, PI[playerid].pHud);
				Native::b_InterfaceCEF(playerid, Custom_Speedometer, PI[playerid].pSpeed);
				Native::b_InterfaceCEF(playerid, Custom_Hints, PI[playerid].pHints);
				PI[playerid].pStatusLoadCef = true;
				ScreenSize::Request(playerid); // интерфейс загрузился - узнать разрешение экрана
			}
			break;
		}
	}
#endif
}

namespace
{
	// размер пакета без байта Arizona (ID + структура SA-MP 0.3.7) и где в нём поле оружия (6 бит)
	bool SyncLayout(int id, unsigned int& size, unsigned int& weaponAt)
	{
		switch (id)
		{
			case 207: size = 1 + 68; weaponAt = 1 + 0x24; return true; // ONFOOT_SYNC_DATA
			case 200: size = 1 + 63; weaponAt = 1 + 0x36; return true; // INCAR_SYNC_DATA
			case 211: size = 1 + 24; weaponAt = 1 + 3; return true;    // PASSENGER_SYNC_DATA
		}
		return false;
	}
}

bool Events::IsSyncPacket(int id) { return id == 200 || id == 207 || id == 211; }

void Events::OnIncomingSync(int playerid, const unsigned char* data, unsigned int length)
{
	unsigned int size = 0, weaponAt = 0;
	if (playerid < 0 || playerid >= MAX_PLAYERS || !data || !length || !SyncLayout(data[0], size, weaponAt)) return;
	if (length == size + 1) fullWeapon[playerid] = data[length - 1];             // клиент Arizona: полный ID в конце
	else if (length >= size) fullWeapon[playerid] = data[weaponAt] & 0x3F;      // обычный клиент: только 6 бит
}

int Events::OutgoingSyncWeapon(int viewerid, const unsigned char* data, unsigned int bits)
{
	if (viewerid < 0 || viewerid >= MAX_PLAYERS || !PI[viewerid].pLauncherStat || !data || bits < 24 || !IsSyncPacket(data[0])) return -1;
	const int source = data[1] | (data[2] << 8); // ID игрока сразу после ID пакета (u16)
	return source >= 0 && source < MAX_PLAYERS ? fullWeapon[source] : -1;
}

int Events::GetFullWeapon(int playerid)
{
	return playerid >= 0 && playerid < MAX_PLAYERS ? fullWeapon[playerid] : 0;
}

void Events::AppendByte(const unsigned char* data, unsigned int bits, unsigned char value, std::vector<unsigned char>& out)
{
	const unsigned int bytes = (bits + 7) / 8;
	out.assign(data, data + bytes);
	out.resize((bits + 8 + 7) / 8, 0);
	if (bits % 8) out[bytes - 1] &= (unsigned char)(0xFF << (8 - bits % 8)); // хвост последнего байта - не данные
	for (int i = 0; i < 8; i++) // биты RakNet - от старшего к младшему
		if (value & (0x80 >> i)) out[(bits + i) / 8] |= (unsigned char)(0x80 >> ((bits + i) % 8));
}

void Events::OnVoicePacket(int playerid, const unsigned char* data, unsigned int length)
{
	Voice::Relay(playerid, data, length);
}

void Events::OnIncomingRPC(int playerid, int rpcid, BitStream& bs)
{
	// RPC 62: [u16 id][u8 ответ][u16 listitem][u8 длина][текст]
	if (rpcid == RPC_DIALOG_RESPONSE && playerid >= 0 && playerid < MAX_PLAYERS && bs.GetNumberOfBitsUsed() >= 40)
	{
		openDialog[playerid].open = false;
		{
			const auto readOffset = bs.GetReadOffset();
			const auto writeOffset = bs.GetWriteOffset();
			bs.SetReadOffset(0);
			uint16_t id = 0;
			if (bs.Read(id) && id == DIALOG_ID_ALIAS)
			{
				bs.SetWriteOffset(0);
				bs.Write(DIALOG_ID_ARIZONA);
			}
			bs.SetReadOffset(readOffset);
			bs.SetWriteOffset(writeOffset);
		}
		const int style = dialogStyle[playerid];
		if (style >= 0 && !DialogHasList(style))
		{
			const auto readOffset = bs.GetReadOffset();
			const auto writeOffset = bs.GetWriteOffset();
			bs.SetReadOffset(24);
			uint16_t item = 0;
			bs.Read(item);
			if (item != 0xFFFF)
			{
				bs.SetWriteOffset(24);
				bs.Write(uint16_t(0xFFFF));
			}
			bs.SetReadOffset(readOffset);
			bs.SetWriteOffset(writeOffset);
		}
		return;
	}
	if (rpcid == 252 && playerid >= 0 && playerid < MAX_PLAYERS) // 252:5 - кнопка голоса
	{
		uint8_t sub = 0, channel = 0;
		bool talking = false;
		if (!bs.Read(sub)) return;
		if (sub == 3) Voice::Handshake(playerid);
		else if (sub == 5 && bs.Read(channel) && bs.Read(talking))
			Plugin::CallPublic("OnPlayerVoice", { playerid, (int)channel - Voice::CHANNEL_BASE + 1, talking ? 1 : 0 });
		return;
	}
#if MODGEAR_FULL
	if (rpcid != 116) return; // EditAttachedObject
	int32_t response = 0, index = 0, modelid = 0, boneid = 0;
	float f[9];
	bs.Read(response);
	bs.Read(index);
	bs.Read(modelid);
	bs.Read(boneid);
	if (index >= 0) return;
	for (float& v : f) if (!bs.Read(v)) return;
	Native::OnPlayerEditAttachedObjectEx(playerid, response, -index, modelid, boneid, f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8]);
#endif
}

void Events::OnOutgoingRPC(int playerid, int rpcid, BitStream& bs)
{
	// RPC 61: [u16 id][u8 стиль]... - стиль нужен для ответа (см. OnIncomingRPC)
	if (rpcid == RPC_SHOW_DIALOG && playerid >= 0 && playerid < MAX_PLAYERS)
	{
		const auto readOffset = bs.GetReadOffset();
		uint16_t id = 0;
		uint8_t style = 0;
		bs.SetReadOffset(0);
		dialogStyle[playerid] = (bs.Read(id) && bs.Read(style) && id != 0xFFFF) ? int8_t(style) : -1;
		OpenDialog& d = openDialog[playerid];
		d.open = dialogStyle[playerid] >= 0 && ReadStr8(bs, d.title) && ReadStr8(bs, d.button1) && ReadStr8(bs, d.button2);
		d.id = id;
		d.style = style;
		bs.SetReadOffset(readOffset);
		if (id == DIALOG_ID_ARIZONA)
		{
			const auto writeOffset = bs.GetWriteOffset();
			bs.SetWriteOffset(0);
			bs.Write(DIALOG_ID_ALIAS);
			bs.SetWriteOffset(writeOffset);
		}
		return;
	}
	if (playerid < 0 || playerid >= MAX_PLAYERS) return;
	if (rpcid != 164 && rpcid != 56 && rpcid != 32 && rpcid != 153 && rpcid != 134 && rpcid != 171 && rpcid != 68 && rpcid != 96 && rpcid != 22 && rpcid != 67) return;

	const auto readOffset = bs.GetReadOffset();
	const auto writeOffset = bs.GetWriteOffset();
	// пишем только внутри уже существующих данных (буфер может быть чужим)
	const int totalBits = bs.GetNumberOfBitsUsed();
	auto fits = [totalBits](int offset, int bits) { return offset + bits <= totalBits; };
	switch (rpcid)
	{
		case 164: // WorldVehicleAdd
		{
			uint16_t vehicleid = 0;
			bs.Read(vehicleid);
			if (vehicleid >= MAX_VEHICLES || !fits(16, 32)) break;
			bs.SetWriteOffset(16);
			bs.Write((uint32_t)(PI[playerid].pLauncherStat ? vTemp[vehicleid].vModelLuxe : vTemp[vehicleid].vDefaultModel));
			break;
		}
		case 56: // SetPlayerMapIcon
		{
			if (!fits(104, 8)) break;
			bs.SetReadOffset(104);
			uint8_t icon = 0;
			bs.Read(icon);
			bs.SetWriteOffset(104);
			bs.Write((uint8_t)(PI[playerid].pLauncherStat ? icon : Native::CallingBackIcon(icon)));
			break;
		}
		case 32: // WorldPlayerAdd
		{
			uint16_t other = 0;
			bs.Read(other);
			if (other >= MAX_PLAYERS || !fits(24, 32)) break;
			bs.SetWriteOffset(24);
			bs.Write(ReplaceSkin(playerid, PI[other].pSkin));
			break;
		}
		case 153: // SetPlayerSkin
		{
			uint32_t other = 0;
			bs.Read(other);
			if (other >= MAX_PLAYERS || !fits(32, 32)) break;
			bs.SetWriteOffset(32);
			bs.Write(ReplaceSkin(playerid, PI[other].pSkin));
			break;
		}
		case 171: // ...skin в 16 бите
		case 68:  // RequestClass
		{
			const int offset = rpcid == 171 ? 16 : 8;
			if (!fits(offset, 32)) break;
			bs.SetReadOffset(offset);
			uint32_t skin = 0;
			bs.Read(skin);
			bs.SetWriteOffset(offset);
			bs.Write(ReplaceSkin(playerid, skin));
			break;
		}
		case 134: // модель в Create*
		{
			if (!fits(344, 16)) break;
			bs.SetReadOffset(344);
			uint16_t model = 0;
			bs.Read(model);
			bs.SetWriteOffset(344);
			if (PI[playerid].pLauncherStat) bs.Write(model);
			else
			{
				int num = Native::CallingBackVehicle(model);
				if (!num)
				{
					num = Native::CallingBackSkin(model);
					if (num == 74) num = model;
				}
				bs.Write((uint16_t)num);
			}
			break;
		}
		case 22: // GivePlayerWeapon: [u32 оружие][u32 патроны]
		case 67: // SetPlayerArmedWeapon: [u32 оружие]
		{
			// кастомное оружие Arizona (71+) игроку без лаунчера - базовое из public GetCustomWeapon
			if (PI[playerid].pLauncherStat || !fits(0, 32)) break;
			uint32_t weapon = 0;
			bs.SetReadOffset(0);
			bs.Read(weapon);
			if (weapon <= 46) break;
			bs.SetWriteOffset(0);
			bs.Write((uint32_t)Native::CallingBackWeapon((int)weapon));
			break;
		}
		case 96: // ScmEvent: [u16 игрок][u32 событие][u32 машина][u32 p2][u32 p3]
		{
			// цвет - только событие 3 (ChangeVehicleColor)
			// и при установке детали (событие 2/5, p2 = ID детали) машина перекрашивалась в "цвет" 6753 и т.п
			if (!fits(16, 96)) break;
			bs.SetReadOffset(16);
			uint32_t event = 0, vehicleid = 0, color = 0;
			bs.Read(event);
			bs.Read(vehicleid);
			bs.Read(color);
			if (event == 3 && vehicleid < MAX_VEHICLES && color < 256) vTemp[vehicleid].vColor = color;
			break;
		}
	}
	bs.SetReadOffset(readOffset);
	bs.SetWriteOffset(writeOffset);
}

bool Events::ResendDialogText(int playerid, const std::string& text)
{
	if (playerid < 0 || playerid >= MAX_PLAYERS || !openDialog[playerid].open) return false;
	const OpenDialog d = openDialog[playerid]; // копия: отправка снова пройдёт через OnOutgoingRPC
	// RPC 61: [u16 id][u8 стиль][u8+заголовок][u8+кнопка 1][u8+кнопка 2][текст, сжатый как в RakNet]
	BitStream bs;
	bs.Write(d.id);
	bs.Write(d.style);
	for (const std::string* s : { &d.title, &d.button1, &d.button2 })
	{
		bs.Write((uint8_t)s->size());
		if (!s->empty()) bs.Write(s->data(), (int)s->size());
	}
	StrCompress::Write(bs, text.size() > 4095 ? text.substr(0, 4095) : text);
	return Platform::SendRPC(playerid, RPC_SHOW_DIALOG, &bs);
}

void Events::OnOutgoingRPCSent(int playerid, int rpcid, BitStream& bs)
{
	if (rpcid != 96 || playerid < 0 || playerid >= MAX_PLAYERS || !PI[playerid].pLauncherStat) return;
	if (bs.GetNumberOfBitsUsed() < 48 + 32) return;
	const auto readOffset = bs.GetReadOffset();
	bs.SetReadOffset(16);
	uint32_t event = 0, vehicleid = 0;
	bs.Read(event);
	bs.Read(vehicleid);
	bs.SetReadOffset(readOffset);
	// ChangeVehicleColor (3) меняет и цвет плагина (vColor уже обновлён в OnOutgoingRPC) - отправляем его
	// После деталей (2/5/6) не шлём: клиент сам накладывает свои материалы заново (core.asi сбрасывает
	// только кэш), а лишний RPC 161 во время загрузки модели детали ронял клиент
	if (event == 3 && vehicleid < MAX_VEHICLES && vTemp[vehicleid].vColorCustom) Native::RestoreBodyMaterials(vehicleid, playerid);
}

// каждый тик сервера: повторы состояния после появления машины/игрока
void Events::OnTick()
{
	Afk::Flush();
	RunPending();
#if MODGEAR_FULL
	ArzCore::RefillNitro();
#endif
}

void Events::OnFastTimer()
{
#if MODGEAR_FULL
	Peds::Update();
#endif
}

void Events::OnPedPacket(int playerid, BitStream& bs)
{
#if MODGEAR_FULL
	Peds::OnPacket(playerid, bs);
#else
	(void)playerid; (void)bs;
#endif
}

void Events::OnSecondTimer()
{
#if MODGEAR_FULL
	for (size_t i = 0; i < aTemp.size(); i++)
	{
		AudioInfo& audio = aTemp[i];
		// машину удалили с сервера: освобождаем её поток (иначе новая машина с тем же ID получит чужое радио)
		if (audio.aType == 3 && !Platform::IsVehicleValid(audio.aUid)) { Native::Audio_Free((int)i); continue; }
		// фон игрока, который уже вышел (на всякий случай - обычно удаляется в OnPlayerDisconnect)
		if (audio.aType == 1 && !Platform::IsPlayerConnected(audio.aUid)) { Native::Audio_Free((int)i); continue; }
		if (!audio.aType || audio.aPause || audio.aSource.empty()) continue;
		Native::Audio_SyncTime((int)i);
		if (audio.aDurationMs < 0) audio.aDurationMs = Media::DurationMs(audio.aSource);
		// трек дошёл до конца: время в мс, как у клиента
		// на каждом круге сервер отставал на ~1 с, и новые слушатели включали трек не с того места)
		if (audio.aDurationMs > 0 && Native::SteadyMs() - audio.aZeroMs >= audio.aDurationMs)
		{
			if (audio.aLoop)
			{
				// по кругу: на ссылках клиент трек не повторяет (BASS_SAMPLE_LOOP у потока не срабатывает) - у
				// слушающих он заканчивался. Перезапуск с 0: сначала стоп, потом play - второго экземпляра не будет
				audio.aTime = 0;
				audio.aZeroMs = Native::SteadyMs();
				Native::Audio_SendStop((int)i);
				Native::Audio_SendPlay((int)i);
			}
			else                              // стоп у всех и public OnAudioStreamFinished(audioID)
			{
				audio.aPause = 1;
				Native::Audio_SendStop((int)i);
				Plugin::CallPublic("OnAudioStreamFinished", (int)i);
			}
		}
	}
	Video::Update(); // видео на экранах: включить подошедшим, выключить ушедшим
	Voice::UpdateAll(); // голос: локальные каналы по расстоянию
	Webcam::Update(); // вебкамера: билеты RPC 222, зрители, события медиасервера
#endif
}
