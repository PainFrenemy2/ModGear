#include <lib/Native.h>
#include "../../Platform.h"
#include "../../Afk.h"
#include <list>
#include <cstring>
#include <cstdarg>
#include <cmath>
#include <chrono>

bool Native::SendPackerPlayer(BitStream& amx, int idrpc, int vehicleid, int playerid, int typeidd)
{
	if (connectedPlayers.size() == 0) return false;
	if (!typeidd) { if (!Platform::IsVehicleValid(vehicleid)) return 0; }
	else if (!Platform::IsPlayerConnected(vehicleid)) return 0;
	auto sendPacket = [&](int i)
	{
		bool send = false;
		if (typeidd == 1) send = PI[i].pLauncherStat == 1 && (playerid != -1 || (vehicleid != -1 && (vehicleid == i || Platform::IsPlayerStreamedIn(vehicleid, i))));
		else if (typeidd == 2) send = PI[i].pLauncherStat == 1 && vehicleid != -1 && vehicleid != i && Platform::IsPlayerStreamedIn(vehicleid, i);
		else send = PI[i].pLauncherStat == 1 && vehicleid != -1 && Platform::IsVehicleStreamedIn(vehicleid, i);
		if (!send) return;
		if (idrpc != -1) Native::MBSendRPC(i, idrpc, &amx, 1, 9);
		else Native::MBSendPacket(i, &amx, 1, 9);
	};
	if (playerid == -1) for (int i : connectedPlayers) { sendPacket(i); }
	else sendPacket(playerid);
	amx.Reset();
	return true;
}
bool Native::AttachPlyerAcs(int playerid, int forplayerid, int index, bool create, int bone, int model, float fSetX, float fSetY, float fSetZ, float fRotX, float fRotY, float fRotZ, float fScaleX, float fScaleY, float fScaleZ, int color_1, int color_2)
{
	BitStream arz;
	arz.Write((uint8_t)220);
	arz.Write((uint8_t)155);
	arz.Write((uint16_t)forplayerid);
	arz.Write((int32_t)index);
	arz.Write(!!(create));
	// playerid - кому слать (-1 - всем, кто видит), forplayerid - чей аксессуар. Флаг хранится у владельца
	// это запись за пределы массива, а при показе зрителю портился флаг аксессуара самого зрителя
	if (forplayerid < 0 || forplayerid >= MAX_PLAYERS || index < 0 || index >= 10) return false;
	PI[forplayerid].pAcs_create[index] = create;
	if (create)
	{
		arz.Write((int32_t)bone);
		arz.Write((int32_t)model);
		arz.Write(fSetX);
		arz.Write(fSetY);
		arz.Write(fSetZ);
		arz.Write(fRotX);
		arz.Write(fRotY);
		arz.Write(fRotZ);
		arz.Write(fScaleX);
		arz.Write(fScaleY);
		arz.Write(fScaleZ);
		arz.Write(color_1);
		arz.Write(color_2);
		// клиент (SimpleAttachments 220:155) читает после цветов ещё bit и u8 - без них падает
		// в обработку ошибки чтения; значение неизвестно, Arizona шлёт 0
		arz.Write(false);
		arz.Write((uint8_t)0);
	}
	SendPackerPlayer(arz, -1, forplayerid, playerid, 1);
	return true;
}

bool Native::ResetAcs(int playerid, int uidtype, int forplayerid)
{
	PI[playerid].pAcs_create[uidtype] = 0;
	PI[playerid].pAcs_bone[uidtype] = 0;
	PI[playerid].pAcs_Model[uidtype] = -1;
	PI[playerid].pAcs_OffsetX[uidtype] = 0.0f;
	PI[playerid].pAcs_OffsetY[uidtype] = 0.0f;
	PI[playerid].pAcs_OffsetZ[uidtype] = 0.0f;
	PI[playerid].pAcs_fRotX[uidtype] = 0.0f;
	PI[playerid].pAcs_fRotY[uidtype] = 0.0f;
	PI[playerid].pAcs_fRotZ[uidtype] = 0.0f;
	PI[playerid].pAcs_fScaleX[uidtype] = 0.0f;
	PI[playerid].pAcs_fScaleY[uidtype] = 0.0f;
	PI[playerid].pAcs_fScaleZ[uidtype] = 0.0f;
	PI[playerid].pAcs_color_1[uidtype] = 0;
	PI[playerid].pAcs_color_2[uidtype] = 0;
	AttachPlyerAcs(forplayerid, playerid, uidtype, PI[playerid].pAcs_create[uidtype]);
	return true;
}
bool Native::b_SetPlayerCollisionStatus(int playerid, int forplayerid, uint8_t status)
{
	BitStream bs;
	bs.Write((uint8_t)220);
	bs.Write((uint8_t)27);
	bs.Write((uint16_t)forplayerid);
	bs.Write(status);
	bs.Write((uint8_t)255);
	MBSendPacket(playerid, &bs, 1, 9);
	bs.Reset();
	return true;
}
// ---- аудио: RPC 252 как у Arizona
// Зона (10) привязывает uid к машине (тип 3) или игроку (тип 2). Клиент получает её, когда источник
static bool AudioValid(int index)
{
	return index >= 0 && index < (int)aTemp.size() && aTemp[index].aType >= 1 && aTemp[index].aType <= 4;
}
// playerid != -1 - только ему; иначе всем с лаунчером, у кого источник в прорисовке (и самому владельцу)
// Точку (тип 4) получают все: слышимость по радиусу клиент считает сам, как у колонок Arizona
// Фон (тип 1) - только владельцу: клиент играет его без 3D, как музыку в наушниках
static void AudioSend(int index, BitStream& bs, int playerid)
{
	const AudioInfo& a = aTemp[index];
	auto sendTo = [&](int i)
	{
		if (i >= 0 && i < MAX_PLAYERS && PI[i].pLauncherStat) Native::MBSendRPC(i, 252, &bs, 1, 9);
	};
	if (a.aType == 1) { if (playerid == -1 || playerid == a.aUid) sendTo(a.aUid); return; }
	if (playerid != -1) return sendTo(playerid);
	for (int i : connectedPlayers)
	{
		const bool listener = a.aType == 4 ? true
			: a.aType == 3 ? Platform::IsVehicleStreamedIn(a.aUid, i)
			: (i == a.aUid || Platform::IsPlayerStreamedIn(a.aUid, i));
		if (listener) sendTo(i);
	}
}
void Native::Audio_SendZone(int index, int playerid)
{
	if (!AudioValid(index)) return;
	BitStream bs;
	bs.Write((uint8_t)10);
	bs.Write((uint16_t)(index + 1));
	bs.Write((uint32_t)0);
	bs.Write((uint16_t)0);
	bs.Write((uint8_t)aTemp[index].aType);
	if (aTemp[index].aType == 4)
	{
		// точка (как колонка Arizona): радиус, позиция сжатым вектором дважды, 8 нулевых байт
		const float* p = aTemp[index].aPos;
		const float mag = std::sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
		bs.Write(aTemp[index].aRadius);
		for (int k = 0; k < 2; k++)
		{
			bs.Write(mag);
			if (mag > 0.0f)
			{
				for (int c = 0; c < 3; c++)
				{
					float n = p[c] / mag;
					if (n < -1.0f) n = -1.0f;
					if (n > 1.0f) n = 1.0f;
					bs.Write((uint16_t)((n + 1.0f) * 32767.5f));
				}
			}
		}
		bs.Write((uint32_t)0);
		bs.Write((uint32_t)0);
	}
	else
	{
		bs.Write((uint16_t)aTemp[index].aUid);
		bs.Write((uint32_t)0);
	}
	AudioSend(index, bs, playerid);
}
void Native::Audio_SendPlay(int index, int playerid)
{
	if (!AudioValid(index)) return;
	Audio_SyncTime(index);
	const AudioInfo& a = aTemp[index];
	const uint16_t len = (uint16_t)strlen(a.aURL);
	BitStream bs;
	bs.Write((uint8_t)17);
	bs.Write((uint16_t)(index + 1));
	bs.Write(true);
	bs.Write(len);
	bs.Write(a.aURL, len);
	bs.Write(0.0f);
	bs.Write(0.7f);
	bs.Write(a.aLoop);              // BASS_SAMPLE_LOOP у клиента: true - сам повторяет трек
	                                // трек без повтора играл по кругу, если стоп от сервера не доходил
	bs.Write((uint32_t)a.aTime);    // с какой секунды (клиент перематывает: BASS_ChannelSetPosition)
	AudioSend(index, bs, playerid);
	if (a.aRadioFx) Audio_SendEffect(index, playerid); // эффекты клиент вешает на канал трека - заново после запуска
}
// эффекты потока (azrtc/AZVoice): 252:18 [u16 поток][u8 2] - набор фильтров BQF + PEAKEQ, звук как из рации
// 252:16 [u16 поток] - снять все эффекты. (Тип 1 - эхо 4 float - на BASS-потоки не ставится)
void Native::Audio_SendEffect(int index, int playerid)
{
	if (!AudioValid(index)) return;
	BitStream bs;
	bs.Write((uint8_t)(aTemp[index].aRadioFx ? 18 : 16));
	bs.Write((uint16_t)(index + 1));
	if (aTemp[index].aRadioFx) bs.Write((uint8_t)2);
	AudioSend(index, bs, playerid);
}
void Native::Audio_SendStop(int index, int playerid)
{
	if (!AudioValid(index)) return;
	BitStream bs;
	bs.Write((uint8_t)13);
	bs.Write((uint16_t)(index + 1));
	AudioSend(index, bs, playerid);
}
void Native::Audio_SendDestroy(int index, int playerid)
{
	if (!AudioValid(index)) return;
	BitStream bs;
	bs.Write((uint8_t)11);
	bs.Write((uint16_t)(index + 1));
	AudioSend(index, bs, playerid);
}
void Native::Audio_Free(int index)
{
	if (!AudioValid(index)) return;
	AudioInfo& a = aTemp[index];
	Audio_SendDestroy(index);
	if (a.aType == 3 && a.aUid > 0 && a.aUid < MAX_VEHICLES) vTemp[a.aUid].vUIDradio = -1;
	else if (a.aType == 2 && a.aUid >= 0 && a.aUid < MAX_PLAYERS) PI[a.aUid].pRadio = -1;
	a = AudioInfo{};
	a.aUid = -1;
}
// клавиша подсказки HOT_KEY_* (0 рация, 1 рация фракции, 2 рация семьи, 3 взаимодействие, 4 телефон)
// Пусто или "null" - null (без клавиши), а не строка "null"
void Native::SendHotKey(int playerid, int type)
{
	static const char* keyEvents[5] = { "setRadioKey", "setFractionRadioKey", "setFamilyRadioKey", "setRadialKey", "setPhoneKey" };
	if (playerid < 0 || playerid >= MAX_PLAYERS || type < 0 || type > 4) return;
	const char* key = PI[playerid].pHotKey[type];
	if (key[0] && strcmp(key, "null") != 0) ShowCef(playerid, "window.executeEvent('event.arizonahud.%s', '[\"%s\"]');", keyEvents[type], key);
	else ShowCef(playerid, "window.executeEvent('event.arizonahud.%s', '[null]');", keyEvents[type]);
}
long long Native::SteadyMs()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
void Native::Audio_SyncTime(int index)
{
	if (index < 0 || index >= (int)aTemp.size() || aTemp[index].aPause || !aTemp[index].aZeroMs) return;
	const long long pos = SteadyMs() - aTemp[index].aZeroMs;
	aTemp[index].aTime = (int)(pos < 0 ? 0 : pos / 1000);
}
void Native::Audio_SetStarted(int index)
{
	if (index < 0 || index >= (int)aTemp.size()) return;
	aTemp[index].aZeroMs = SteadyMs() - (long long)aTemp[index].aTime * 1000;
}
void Native::Audio_SendPointZones(int playerid)
{
	for (int i = 0; i < (int)aTemp.size(); i++)
	{
		if (aTemp[i].aType != 4 && !(aTemp[i].aType == 1 && aTemp[i].aUid == playerid)) continue;
		Audio_SendZone(i, playerid);
		if (!aTemp[i].aPause) Audio_SendPlay(i, playerid);
	}
}
bool Native::MBSendPacket(int player_id, BitStream* bs, int priority, int reliabilit)
{
	return Afk::Send(player_id, false, 0, bs);
}
bool Native::MBSendRPC(int player_id, RPCIndex rpc_id, BitStream* bs, int priority, int reliability)
{
	return Afk::Send(player_id, true, rpc_id, bs);
}
bool Native::b_ShowPlayerTag(int playerid, int forplayerid, bool status)
{
	BitStream bs;
	bs.Write((uint8_t)220);
	bs.Write((uint8_t)120);
	bs.Write((uint16_t)playerid);
	bs.Write(!!(status));
	bs.Write((uint8_t)strlen(PI[playerid].pTag));
	bs.Write(PI[playerid].pTag, strlen(PI[playerid].pTag));
	if (forplayerid == -1) SendPackerPlayer(bs, -1, playerid, forplayerid, 2);
	else
	{
		MBSendPacket(playerid, &bs, 1, 9);
		bs.Reset();
	}
	return true;
}
bool Native::b_SetVehicleColor(int carid, int color, int playerid)
{
	if (color < 0 || color > 255) color = 0; // таблица цветов SA-MP: 0..255
	BitStream arz;
	arz.Write((uint16_t)(carid));
	// материал как у Arizona (RPC 161): 0, 1, 4 (часть + цвет + стиль), 1 (одна часть)
	// u16 длина + часть, ARGB, u16 длина + стиль
	arz.Write((uint8_t)0);
	arz.Write((uint8_t)1);
	arz.Write((uint8_t)4);
	arz.Write((uint8_t)1);
	arz.Write((uint16_t)3);
	arz.Write("wad", 3);
	arz.Write((uint8_t)255);
	arz.Write(ColorRGBArray[color].R);
	arz.Write(ColorRGBArray[color].G);
	arz.Write(ColorRGBArray[color].B);
	arz.Write((uint16_t)7);
	arz.Write("default", 7);
	SendPackerPlayer(arz, 161, carid, playerid);
	return true;
}
bool Native::b_WheelSettingsCustom(int carid, int type, float flot, float flot2, int playerid)
{
	for (int i = 0; i < 2; i++)
	{
		BitStream arz;
		arz.Write((uint16_t)carid);
		arz.Write((uint16_t)0);
		arz.Write((uint8_t)2);
		arz.Write((uint8_t)1);
		// type 0 - передняя ось (lf/rf), 1 - задняя (lb/rb)
		// при type 0 строка "wheel_lf_dummy" уходила с длиной "wheel_lb_dummy" и пакет ломался
		const char* wheel = wheelSettings[type == 0 ? i : i + 2];
		arz.Write((uint16_t)strlen(wheel));
		arz.Write(wheel, strlen(wheel));
		arz.Write((float)(i ? flot2 : -flot2));
		arz.Write((float)0.0f);
		arz.Write((float)0.0f);
		arz.Write((float)(i ? type == 0 ? -flot : flot : type == 0 ? flot : -flot));
		SendPackerPlayer(arz, 161, carid, playerid);
	}
	return 1;
}
bool Native::b_NeonColorChange(int carid, int color, int playerid)
{
	if (color < 0 || color > 255) color = 0; // таблица цветов SA-MP: 0..255
	BitStream arz;
	arz.Write((uint8_t)220);
	arz.Write((uint8_t)142);
	arz.Write((uint16_t)carid);
	// клиент всегда читает 4 байта R, G, B, A; A = 0 - убрать неон
	if (color)
	{
		arz.Write(ColorRGBArray[color].B);
		arz.Write(ColorRGBArray[color].G);
		arz.Write(ColorRGBArray[color].R);
		arz.Write((uint8_t)255);
	}
	else arz.Write((uint32_t)0);
	SendPackerPlayer(arz, -1, carid, playerid);
	return true;
}
bool Native::b_SetVehicleHidraulics(int carid, int type, int playerid)
{
	BitStream arz;
	arz.Write((uint16_t)carid);
	arz.Write((uint8_t)0);
	arz.Write((uint8_t)1);
	arz.Write((uint8_t)0);
	arz.Write((uint8_t)1);
	arz.Write((uint8_t)16);
	arz.Write((uint8_t)(type == 0 ? 0 : 154));
	arz.Write((uint8_t)(type == 0 ? 0 : 153));
	arz.Write((uint8_t)(type == 1 ? 217 : type == 2 ? 25 : 128));
	arz.Write((uint8_t)63);
	SendPackerPlayer(arz, 161, carid, playerid);
	return true;
}
bool Native::b_VehicleToneCustomizer(int carid, int type, int red, int green, int blue, int size, int playerid)
{
	BitStream arz;
	arz.Write((uint16_t)(carid));
	arz.Write((uint8_t)0);
	arz.Write((uint8_t)1);
	arz.Write((uint8_t)(!vTemp[carid].vToner[type] ? 3 : 4));
	arz.Write((uint8_t)1);
	arz.Write((uint8_t)strlen(tonerNames[type]));
	arz.Write((uint8_t)0);
	arz.Write(tonerNames[type], strlen(tonerNames[type]));
	if (vTemp[carid].vToner[type])
	{
		// цвет материала в RPC 161 клиент читает как u32 0xRRGGBBAA: на проводе A, B, G, R
		arz.Write((uint8_t)size);
		arz.Write((uint8_t)blue);
		arz.Write((uint8_t)green);
		arz.Write((uint8_t)red);
		arz.Write((uint8_t)5);
		arz.Write((uint8_t)0);
		arz.Write("white", 5);
	}
	SendPackerPlayer(arz, 161, carid, playerid);
	return true;
}
bool Native::b_InstallNumberVehicle(int carid, int type, char number[10], char region[20], int playerid)
{
	if (type >= 5) strcpy(region, regionNames[type - 5]), type = 5;
	// текстура флага/значка, 100 и цвет. Текстура и параметры зависят от типа номера
	struct PlateStyle { int type; const char* texture; uint8_t a, b; const char* flag; };
	static const PlateStyle styles[] = {
		{ 3, "obj95_nplate1_tx", 0x4D, 0x37, "" },
		{ 13, "nplate1_lav", 0x43, 0x3D, "flag_01" },
	};
	const PlateStyle* style = &styles[0]; // неизвестный тип - как тип 3
	for (const PlateStyle& st : styles) if (st.type == type) style = &st;
	const uint8_t textureLen = (uint8_t)std::strlen(style->texture), flagLen = (uint8_t)std::strlen(style->flag);
	const uint8_t numberLen = (uint8_t)std::strlen(number), regionLen = (uint8_t)std::strlen(region);
	BitStream bs;
	bs.Write((uint8_t)220);
	bs.Write((uint8_t)153);
	bs.Write((uint16_t)carid);
	bs.Write((uint8_t)type);
	bs.Write(numberLen);
	bs.Write(number, numberLen);
	bs.Write(regionLen);
	bs.Write(region, regionLen);
	bs.Write(true);
	bs.Write(textureLen);
	bs.Write(style->texture, textureLen);
	bs.Write(style->a);
	bs.Write(style->b);
	bs.Write(flagLen);
	bs.Write(style->flag, flagLen);
	bs.Write((uint8_t)100);
	bs.Write((uint8_t)255);
	bs.Write((uint8_t)255);
	bs.Write((uint8_t)255);
	static const unsigned char pad = 0;
	bs.WriteBits(&pad, 7, false); // добивка до целого байта, как у Arizona
	SendPackerPlayer(bs, -1, carid, playerid);
	return true;
}
bool Native::b_SetColorHeadlamp(int carid, int color, int playerid)
{
	if (color < 0 || color > 255) color = 0; // таблица цветов SA-MP: 0..255
	BitStream arz;
	arz.Write((uint8_t)220);
	arz.Write((uint8_t)132);
	arz.Write((uint16_t)carid);
	// клиент читает u32 0xRRGGBBAA целиком
	// 0 - обычный (белый) свет
	arz.Write((uint8_t)255);
	if (color)
	{
		arz.Write(ColorRGBArray[color].R);
		arz.Write(ColorRGBArray[color].G);
		arz.Write(ColorRGBArray[color].B);
	}
	else
	{
		arz.Write((uint8_t)255);
		arz.Write((uint8_t)255);
		arz.Write((uint8_t)255);
	}
	SendPackerPlayer(arz, -1, carid, playerid);
	return true;
}
bool Native::b_SetShotsExhaust(int carid, int status, int playerid)
{
	BitStream arz;
	// 220:162 - модуль GearCheangeEffect ядра Arizona (vorbisFile.dll, не core.asi)
	// i16 машина, bit вкл, bit доп. режим - хлопки/огонь из выхлопа при переключении передач
	arz.Write((uint8_t)220);
	arz.Write((uint8_t)162);
	arz.Write((int16_t)carid);
	arz.Write(status != 0);
	arz.Write(status > 1);
	SendPackerPlayer(arz, -1, carid, playerid);
	return true;
}
bool Native::b_SmokeColorShift(int carid, int color, int playerid)
{
	if (color < 0 || color > 255) color = 0; // таблица цветов SA-MP: 0..255
	BitStream arz;
	arz.Write((uint8_t)220);
	arz.Write((uint8_t)139);
	arz.Write((uint16_t)carid);
	arz.Write((uint8_t)205);
	arz.Write((uint8_t)204);
	arz.Write((uint8_t)204);
	arz.Write((uint8_t)62);
	if (color != 0)
	{
		arz.Write(ColorRGBArray[color].B);
		arz.Write(ColorRGBArray[color].G);
		arz.Write(ColorRGBArray[color].R);
	}
	SendPackerPlayer(arz, -1, carid, playerid);
	return true;
}
bool Native::b_CustomizeCarWheel(int carid, float diamet, int playerid)
{
	BitStream arz;
	arz.Write((uint8_t)220u);
	arz.Write((uint8_t)135u);
	arz.Write((uint16_t)carid);
	arz.Write((uint8_t)3); // как у Arizona: 3 - все колёса (передняя и задняя ось)
	arz.Write(diamet);
	SendPackerPlayer(arz, -1, carid, playerid);
	return true;
}
bool Native::b_SetVinylCustomization(int carid, int type, int color, int playerid)
{
	if (color < 0 || color > 255) color = 0; // таблица цветов SA-MP: 0..255
	// без винила: снять материал "wad" (без этого клиент оставлял прошлый винил), затем вернуть цвет
	// одно удаление без цвета оставляло машину без цвета
	int id2 = type == 0 ? 48 : type + 48;
	BitStream arz;
	arz.Write((uint16_t)(carid));
	arz.Write((uint8_t)0);
	arz.Write((uint8_t)1);
	arz.Write((uint8_t)(id2 != 48 ? 4 : 3));
	arz.Write((uint8_t)1);
	arz.Write((uint8_t)3);
	arz.Write((uint8_t)0);
	arz.Write("wad", 3);
	if (id2 != 48)
	{
		arz.Write((uint8_t)255);
		arz.Write(ColorRGBArray[color].R);
		arz.Write(ColorRGBArray[color].G);
		arz.Write(ColorRGBArray[color].B);
		arz.Write((uint8_t)(type > 9 ? 5 : 4));
		arz.Write((uint8_t)0);
		arz.Write((uint8_t)119);
		arz.Write((uint8_t)97);
		arz.Write((uint8_t)100);
		arz.Write((uint8_t)(type > 9 ? 49 : id2));
		if (type > 9) arz.Write((uint8_t)(38 + type));
	}
	SendPackerPlayer(arz, 161, carid, playerid);
	if (type == 0) b_SetVehicleColor(carid, color, playerid);
	return true;
}
bool Native::b_SetVehicleColorWheel(int carid, int color, int playerid, int type)
{
	(void)type;
	if (color < 0 || color > 255) color = 0; // таблица цветов SA-MP: 0..255
	// материал RPC 161 клиент ставит по имени текстуры на всю машину. Красим только текстуру обода
	// этого диска (таблица wheelTable)
	const WheelInfo* wheel = FindWheel(vTemp[carid].vDiski);
	if (!wheel || !wheel->texture[0]) return false;
	const uint16_t len = (uint16_t)strlen(wheel->texture);
	BitStream arz;
	arz.Write((uint16_t)(carid));
	arz.Write((uint8_t)0);
	arz.Write((uint8_t)1);
	arz.Write((uint8_t)4);
	arz.Write((uint8_t)1);
	arz.Write(len);
	arz.Write(wheel->texture, len);
	arz.Write((uint8_t)255);
	arz.Write(ColorRGBArray[color].R);
	arz.Write(ColorRGBArray[color].G);
	arz.Write(ColorRGBArray[color].B);
	arz.Write((uint16_t)7);
	arz.Write("default", 7);
	SendPackerPlayer(arz, 161, carid, playerid);
	return true;
}
void Native::RestoreBodyMaterials(int carid, int playerid)
{
	if (carid <= 0 || carid >= MAX_VEHICLES) return;
	if (vTemp[carid].vColorCustom)
	{
		if (vTemp[carid].vVinil) b_SetVinylCustomization(carid, vTemp[carid].vVinil, vTemp[carid].vColor, playerid);
		else b_SetVehicleColor(carid, vTemp[carid].vColor, playerid);
	}
	for (int i = 0; i < 7; i++)
		if (vTemp[carid].vToner[i]) b_VehicleToneCustomizer(carid, i, vTemp[carid].vToner_red[i], vTemp[carid].vToner_green[i], vTemp[carid].vToner_blue[i], vTemp[carid].vToner_size[i], playerid);
}
static void SendScmPart(int carid, uint32_t event, int component, int playerid)
{
	// ScmEvent (RPC 96): u16 игрок 0xFFFF, u32 событие, u32 машина, u32 деталь, u32 0
	BitStream arz;
	arz.Write((uint8_t)255);
	arz.Write((uint8_t)255);
	arz.Write(event);
	arz.Write((uint32_t)carid);
	arz.Write((uint32_t)component);
	arz.Write((uint32_t)0);
	Native::SendPackerPlayer(arz, 96, carid, playerid);
}
bool Native::b_InstallVehicleComponent(int carid, int type, int component, int playerid)
{
	if (!component || carid <= 0 || carid >= MAX_VEHICLES || type < 0 || type >= PART_SLOTS) return false;
	// материалы кузова (цвет, винил, тонировку) клиент после детали накладывает заново сам: core.asi на
	// событиях 2/5/6 только сбрасывает кэш материалов машины, сами материалы остаются
	if (type == PART_WHEELS)
	{
		// диски - обычные детали GTA (секция wheel в data/carmods.dat) -> событие 2, как в тюнинг-ателье
		// Только модель из секции wheel клиента: другая уходит в CVehicle::AddVehicleUpgrade и клиент падает
		const bool gtaWheel = component == 1025 || (component >= 1073 && component <= 1085) || (component >= 1096 && component <= 1098);
		if (!gtaWheel && !FindWheel(component)) return false;
		SendScmPart(carid, 2, component, playerid);
		if (playerid == -1) vTemp[carid].vDiski = component;
		// цвет дисков - только если его задали через SetVehicleColorWheel
		if (vTemp[carid].vDiskiColor != -1) b_SetVehicleColorWheel(carid, vTemp[carid].vDiskiColor, playerid);
		return true;
	}
	// остальное - simple-детали кастомных машин (data/carmods_s.dat, id из arizona/NamedModels.json) ->
	// событие 5. Слот берём по самой детали: в одном слоте одна деталь, у Arizona их 11 видов
	int slot = SimplePartSlot(component);
	if (!slot)
	{
		if (type < PART_HOOD || type > PART_EXHAUST) return false;
		slot = type;
	}
	if (playerid == -1)
	{
		int& current = vTemp[carid].vParts[slot];
		// новая деталь в занятом слоте: старую снять (событие 6), иначе у клиента остаются обе
		if (current && current != component) SendScmPart(carid, 6, current, -1);
		current = component;
	}
	SendScmPart(carid, 5, component, playerid);
	return true;
}
bool Native::b_RemoveVehicleComponent(int carid, int component)
{
	if (!component || carid <= 0 || carid >= MAX_VEHICLES) return false;
	VehicleInfo& v = vTemp[carid];
	if (v.vDiski == component)
	{
		// диски - обычная деталь GTA: RPC 57 (u16 машина, u16 деталь)
		v.vDiski = 0;
		v.vDiskiColor = -1;
		BitStream arz;
		arz.Write((uint16_t)carid);
		arz.Write((uint16_t)component);
		SendPackerPlayer(arz, 57, carid);
		return true;
	}
	for (int slot = 1; slot < PART_SLOTS; slot++)
	{
		if (slot == PART_WHEELS || v.vParts[slot] != component) continue;
		v.vParts[slot] = 0;
		SendScmPart(carid, 6, component, -1); // simple-детали снимаются только событием 6
		return true;
	}
	return false;
}
int Native::GetVehicleComponentSlot(int carid, int component)
{
	if (!component || carid <= 0 || carid >= MAX_VEHICLES) return 0;
	if (vTemp[carid].vDiski == component) return PART_WHEELS;
	for (int slot = 1; slot < PART_SLOTS; slot++)
		if (slot != PART_WHEELS && vTemp[carid].vParts[slot] == component) return slot;
	return 0;
}
void Native::WriteCefString(BitStream& bs, const char* text)
{
	// у Arizona IsCustomPacketCompression() = 1: строка - u16 длина + u8 флаг сжатия + байты
	// строка съезжала на байт, и всё после неё читалось криво
	size_t len = text ? strlen(text) : 0;
	if (len > 0xFFFF) len = 0xFFFF;
	if (len == 1)
	{
		// длину 1 клиент понимает как "дальше i32, сделай из него текст": цифру так и шлём, иначе добавляем пробел
		bs.Write((uint16_t)(text[0] >= '0' && text[0] <= '9' ? 1 : 2));
		bs.Write((uint8_t)0);
		if (text[0] >= '0' && text[0] <= '9') bs.Write((int32_t)(text[0] - '0'));
		else { bs.Write(text, 1); bs.Write(' '); }
		return;
	}
	bs.Write((uint16_t)len);
	bs.Write((uint8_t)0);
	if (len) bs.Write(text, (int)len);
}
bool Native::LoadFileCef(int playerid, int browserid, const char* url, const char* key)
{
	BitStream bs;
	bs.Write((uint8_t)220);
	bs.Write((uint8_t)10);
	bs.Write((uint8_t)PI[playerid].pEkranWindows[0]);
	bs.Write((uint8_t)PI[playerid].pEkranWindows[1]);
	bs.Write((uint16_t)0);
	bs.Write((uint8_t)PI[playerid].pEkranWindows[2]);
	bs.Write((uint8_t)PI[playerid].pEkranWindows[3]);
	bs.Write((uint32_t)0);
	bs.Write((uint32_t)0);
	bs.Write((uint16_t)0);
	WriteCefString(bs, url);
	WriteCefString(bs, browserid == 0 ? key : "");
	bs.Write((uint32_t)browserid);
	MBSendPacket(playerid, &bs, 1, 9);
	bs.Reset();
	return true;
}
bool Native::ShowCefMobile(int playerid, int ar1, int ar2, const char* format, ...)
{
	char text[4094];
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	BitStream bs;
	bs.Write((uint8_t)220);
	bs.Write((uint8_t)84);
	bs.Write((uint8_t)ar1);
	bs.Write((uint8_t)ar2);
	bs.Write((uint32_t)strlen(text));
	bs.Write(text, strlen(text));
	MBSendPacket(playerid, &bs, 1, 9);
	bs.Reset();
	return true;
}
void Native::SendChatTyping(int to, int playerid, int flag)
{
	BitStream bs;
	bs.Write((uint8_t)220);
	bs.Write((uint8_t)108);
	bs.Write((uint32_t)playerid);
	bs.Write((uint8_t)flag);
	MBSendPacket(to, &bs, 1, 9);
}
bool Native::ShowCustom(int playerid, std::initializer_list<int> args)
{
	BitStream bitStream;
	bitStream.Write((uint8_t)220);
	for (int arg : args)
	{
		bitStream.Write((uint8_t)arg);
	}
	MBSendPacket(playerid, &bitStream, 1, 9);
	bitStream.Reset();
	return true;
}
bool Native::ShowCustomRPC(int playerid, int idrpc, std::initializer_list<int> args)
{
	BitStream bitStream;
	for (int arg : args)
	{
		bitStream.Write((uint8_t)arg);
	}
	MBSendRPC(playerid, idrpc, &bitStream, 1, 9);
	bitStream.Reset();
	return true;
}
bool Native::ShowCef(int playerid, const char* format, ...)
{
	if (!PI[playerid].pLauncherStat) return false;
	char text[4094];
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);
	// формат клиента Arizona: u32 0, u16 длина, u8 0 (текст не сжат), текст, u32 -1
	BitStream bitStream;
	bitStream.Write((uint8_t)220);
	bitStream.Write((uint8_t)17);
	bitStream.Write((uint32_t)0);
	const size_t len = strlen(text);
	bitStream.Write((uint16_t)len);
	bitStream.Write((uint8_t)0);
	bitStream.Write(text, len);
	bitStream.Write((uint32_t)0xFFFFFFFF);
	MBSendPacket(playerid, &bitStream, 1, 9);
	bitStream.Reset();
	return true;
}
bool Native::b_InterfaceCEF(int playerid, int index, int type)
{
	if (type < 0) type = 0;
	if (!PI[playerid].pLauncherStat) return 0;
	switch (index)
	{
	case Custom_Speedometer:
	{
		if (type > 4) return 0;
		if (type == 0)
		{
			ShowCef(playerid, "window.executeEvent('event.arizonahud.speedometerType', '[\"null\"]');");
			if (PI[playerid].pSpeed >= 1 && PI[playerid].pSpeed <= 2) ShowCustom(playerid, { 4, 0 });
		}
		else if (type == 3)
		{
			ShowCef(playerid, "window.executeEvent('event.arizonahud.vehicleUpdateSpeedTime', '[100]');");
			ShowCef(playerid, "window.executeEvent('event.arizonahud.speedometerType', '[\"advanced\"]');");
		}
		else if (type == 4)
		{
			ShowCef(playerid, "window.executeEvent('event.arizonahud.vehicleUpdateSpeedTime', '[350]');");
			ShowCef(playerid, "window.executeEvent('event.arizonahud.speedometerType', '[\"simplified\"]');");
		}
		else if (type >= 1 && type <= 2) ShowCustom(playerid, { 4, type });
		PI[playerid].pSpeed = type;
		break;
	}
	case Custom_Hud:
	{
		if (type > 6) return 0;
		if (type == 0)
		{
			ShowCustom(playerid, { 8, 0 });
			if (PI[playerid].pHud >= 3 && PI[playerid].pHud <= 5)
			{
				ShowCef(playerid, "window.executeEvent('event.hudSA.updateSanAndreasHudVisible', '[false]');");
				ShowCef(playerid, "window.executeEvent('event.hudViceCity.updateViceCityHudVisible', '[false]');");
			}
		}
		else if (type == 3)
		{
			ShowCustom(playerid, { 8, 1 });
			ShowCef(playerid, "window.executeEvent('event.hudSA.updateSanAndreasHudVisible', '[false]');");
			ShowCef(playerid, "window.executeEvent('event.hudViceCity.updateViceCityHudVisible', '[false]');");
		}
		else if (type == 4)
		{
			ShowCustom(playerid, { 8, 2 });
			ShowCef(playerid, "window.executeEvent('event.hud.updateHudVisible', '[true]');");
			ShowCef(playerid, "window.executeEvent('event.hudSA.updateSanAndreasHudVisible', '[false]');");
			ShowCef(playerid, "window.executeEvent('event.hudViceCity.updateViceCityHudVisible', '[true]');");
			ShowCef(playerid, "window.executeEvent('event.hudViceCity.updateDonateMultiplier', '[%d]');", iServer.iXPayDay);
		}
		else if (type == 5)
		{
			ShowCustom(playerid, { 8, 2 });
			ShowCef(playerid, "window.executeEvent('event.hud.updateHudVisible', '[true]');");
			ShowCef(playerid, "window.executeEvent('event.hudViceCity.updateViceCityHudVisible', '[false]');");
			ShowCef(playerid, "window.executeEvent('event.hudSA.updateSanAndreasHudVisible', '[true]');");
			ShowCef(playerid, "window.executeEvent('event.arizonahud.serverInfo', '[{\"id\":%d,\"title\":\"%s\",\"project\":\"%s\",\"type\":\"%s\",\"onLine\":771,\"flag\":%d,\"logo\":%d,\"multiplier\":%d}]');", iServer.iServerNumber, iServer.iServerName, iServer.iServerProject, iServer.iServerType, iServer.iServerFlag, iServer.iServerLogo, iServer.iXPayDay);
		}
		else if (type >= 1 && type <= 2 || type == 6)
		{
			if (type == 6)
			{
				ShowCustom(playerid, { 8, 3 });
				ShowCefMobile(playerid, 8, 0, "%d", iServer.iXPayDay);
			}
			else ShowCustom(playerid, { 8, type });
		}
		PI[playerid].pHud = type;
		break;
	}
	case Custom_Radar:
	{
		if (type > 3) return 0;
		ShowCustom(playerid, { 9, type });
		break;
	}
	case Custom_Fuel:
	{
		if (type > 300) return 0;
		if (PI[playerid].pSpeed == 1 || PI[playerid].pSpeed == 2)
		{
			BitStream bs;
			bs.Write((uint8_t)220);
			bs.Write((uint8_t)7);
			bs.Write((float)type);
			bs.Write((uint8_t)1);
			bs.Write((uint8_t)155);
			bs.Write((uint8_t)155);
			bs.Write((uint8_t)155);
			MBSendPacket(playerid, &bs, 1, 9);
			bs.Reset();
		}
		else ShowCef(playerid, "window.executeEvent('event.arizonahud.vehicleLiters', '[%d]');", type);
		break;
	}
	case Custom_TypeFuel:
	{
		if (type > 2) return 0;
		if (type == 0) ShowCef(playerid, "window.executeEvent('event.arizonahud.vehicleFuelType', '[\"petrol\"]');");
		else if (type == 1) ShowCef(playerid, "window.executeEvent('event.arizonahud.vehicleFuelType', '[\"electro\"]');");
		else if (type == 2) ShowCef(playerid, "window.executeEvent('event.arizonahud.vehicleFuelType', '[\"voltage\"]');");
		break;
	}
	case Custom_Milage:
	{
		if (type > 999999) return 0;
		if (PI[playerid].pSpeed == 1 || PI[playerid].pSpeed == 2)
		{
			BitStream bs;
			bs.Write((uint8_t)220);
			bs.Write((uint8_t)5);
			bs.Write(type);
			bs.Write((uint8_t)155);
			bs.Write((uint8_t)155);
			bs.Write((uint8_t)155);
			MBSendPacket(playerid, &bs, 1, 9);
			bs.Reset();
		}
		else ShowCef(playerid, "window.executeEvent('event.arizonahud.vehicleMileage', '[%d]');", type);
		break;
	}
	case Custom_Hunger:
	{
		if (type > 100) return 0;
		// CEF-худ Arizona (arizonahud) стоит у клиента всегда, кроме Vice City и мобильного
		// слалось только при SetPlayerInterface(Custom_Hud, 5), и на обычном худе сытость не менялась
		if (PI[playerid].pHud == 4) ShowCef(playerid, "window.executeEvent('event.hudViceCity.updateSatiety', '[%d]');", type);
		else if (PI[playerid].pHud != 6) ShowCef(playerid, "window.executeEvent('event.arizonahud.playerSatiety', '[%d]');", type);
		if (PI[playerid].pHud >= 1 && PI[playerid].pHud <= 2 || PI[playerid].pHud == 6) ShowCustom(playerid, { 3, type });
		break;
	}
	case Custom_Hints:
	{
		if (type > 1) return 0;
		if (type == 0) ShowCef(playerid, "window.executeEvent('event.arizonahud.hotkeysVisible', '[false]');");
		else if (type == 1)
		{
			// порядок как у Arizona: рация фракции, семьи, взаимодействие, рация, телефон
			ShowCef(playerid, "window.executeEvent('event.arizonahud.hotkeysVisible', '[true]');");
			for (int k : { 1, 2, 3, 0, 4 }) SendHotKey(playerid, k);
			ShowCef(playerid, "window.executeEvent('event.arizonahud.setWorkbookKey', '[\"\", \"F1\", 0]');");
		}
		PI[playerid].pHints = type;
		break;
	}
	case Custom_TypeDialog:
	{
		if (type > 3) return 0;
		ShowCustom(playerid, { 53, type });
		break;
	}
	case Custom_Map:
	{
		if (type > 1) return 0;
		ShowCustom(playerid, { 131, type ? 16 : 0 });
		break;
	}
	case Custom_WallHack:
	{
		if (type > 1) return 0;
		ShowCustom(playerid, { 43, type });
		break;
	}
	case Custom_TypeAim:
	{
		if (type > 10) return 0;
		// клиент Arizona (CustomCrosshairs): 220:121 - u8 длина + имя текстуры прицела из hud.txd
		// 220:122 - bit, включить свой прицел
		// а не прицел
		if (type != 0)
		{
			char text[30];
			snprintf(text, sizeof(text), "custom_crosshair_%d", type);
			const uint8_t len = (uint8_t)strlen(text);
			BitStream tex;
			tex.Write((uint8_t)220);
			tex.Write((uint8_t)121);
			tex.Write(len);
			tex.Write(text, len);
			MBSendPacket(playerid, &tex, 1, 9);
		}
		BitStream bs;
		bs.Write((uint8_t)220);
		bs.Write((uint8_t)122);
		bs.Write(type != 0);
		MBSendPacket(playerid, &bs, 1, 9);
		break;
	}
	case Custom_Discord:
	{
		if (type > 2) return 0;
		ShowCustom(playerid, { 71, type ? 128 : 0 });
		break;
	}
	case Custom_Dialog:
	{
		if (type > 2) return 0;
		ShowCustom(playerid, { 39, type });
		// новый клиент берёт настройку из CEF
		ShowCef(playerid, "window.executeEvent('cef.setDialogSettings', '[{\"autosavePosition\":true,\"autosaveInputText\":%s}]');", type ? "true" : "false");
		break;
	}
	case Custom_Chat:
	{
		if (type > 2) return 0;
		ShowCustom(playerid, { 33, type });
		break;
	}
	case Custom_SetRadius:
	{
		if (type > 500) return 0;
		// 220:64 bool, float дистанция (как у Arizona: 2.0 -> DC 40 80 00 00 20 00)
		BitStream bs;
		bs.Write((uint8_t)220);
		bs.Write((uint8_t)64);
		bs.Write(type > 0);
		if (type > 0) bs.Write((float)type);
		MBSendPacket(playerid, &bs, 1, 9);
		bs.Reset();
		break;
	}
	case Custom_Show:
	{
		if (type > 1) return 0;
		BitStream bs;
		std::vector<uint8_t> ida = { 220, 47, 8, 83, 78, 79, 87, 70, 88, 73, 73, 0, 0, 0, 0, 0, 0, 128, 63, 0, 0, 0, 64, 0, 208, 7, 0, 0 };
		for (size_t i = 0; i < ida.size(); i++)
		{
			uint8_t value = ida[i];
			if (!type && i == 1) value = 48;
			bs.Write(value);
		}
		MBSendPacket(playerid, &bs, 1, 9);
		bs.Reset();
		break;
	}
	case Custom_Wanted:
	{
		if (type > 7) return 0;
		// как сытость: arizonahud - на любом худе, кроме Vice City и мобильного
		if (PI[playerid].pHud == 4) ShowCef(playerid, "window.executeEvent('event.hudViceCity.updateWantedLevel', '[%d]');", type);
		else if (PI[playerid].pHud != 6) ShowCef(playerid, "window.executeEvent('event.arizonahud.playerWanted', '[%d]');", type);
		break;
	}
	case Custom_ChatBoard:
	{
		if (type > 1) return 0;
		PI[playerid].pChat = type;
		for (int i : connectedPlayers) if (Platform::IsPlayerStreamedIn(i, playerid) && PI[i].pLauncherStat) SendChatTyping(playerid, i, (PI[playerid].pChatOpen && PI[playerid].pChat ? 128 : 0));
		break;
	}
	case Custom_GeoPosition:
	{
		if (type > 1) return 0;
		ShowCef(playerid, "window.executeEvent('event.arizonahud.updateGeoPositionVisibility', '[%s]');", type ? "true" : "false");
		break;
	}
	case Custom_CinemaMode:
	{
		if (type > 1) return 0;
		ShowCustom(playerid, { 86, type ? 128 : 0 });
		break;
	}
	default: return 0;
	}
	return 1;
}

// ---- функции Arizona, которые включает сервер (core.asi, пакет 220)
bool Native::b_SetVehicleStrobeLights(int carid, int mode, float speed, bool beam, int playerid)
{
	// 220:209 - u16 машина, u8 режим (0 - выкл), float скорость, bit луч
	BitStream bs;
	bs.Write((uint8_t)220);
	bs.Write((uint8_t)209);
	bs.Write((uint16_t)carid);
	bs.Write((uint8_t)mode);
	bs.Write(speed);
	bs.Write(beam);
	SendPackerPlayer(bs, -1, carid, playerid);
	return true;
}
bool Native::b_SetVehicleBrakeCalipers(int carid, int model, bool simple, int playerid)
{
	// 220:220 - u16 машина, bit вкл; если вкл: bit simple-модель, u16 модель суппорта
	BitStream bs;
	bs.Write((uint8_t)220);
	bs.Write((uint8_t)220);
	bs.Write((uint16_t)carid);
	bs.Write(model > 0);
	if (model > 0)
	{
		bs.Write(simple);
		bs.Write((uint16_t)model);
	}
	SendPackerPlayer(bs, -1, carid, playerid);
	return true;
}
bool Native::b_SetVehicleDrift(int carid, bool status, int playerid)
{
	// 220:187 - u16 машина, bit (добавить / убрать машину из дрифт-режима)
	BitStream bs;
	bs.Write((uint8_t)220);
	bs.Write((uint8_t)187);
	bs.Write((uint16_t)carid);
	bs.Write(status);
	SendPackerPlayer(bs, -1, carid, playerid);
	return true;
}
bool Native::b_SetVehicleSiren(int carid, bool status, int playerid)
{
	// 220:152 - u16 машина, bit. Клиент подменяет CVehicle::UsesSiren (вызов из samp.dll)
	// так что машина получает мигалку/сирену без пересоздания
	BitStream bs;
	bs.Write((uint8_t)220);
	bs.Write((uint8_t)152);
	bs.Write((uint16_t)carid);
	bs.Write(status);
	SendPackerPlayer(bs, -1, carid, playerid);
	return true;
}
bool Native::b_SetPlayerSpeedLimit(int playerid, float speed, int modelid)
{
	// 220:111 - float скорость (<= 0 - без ограничения), u32 число моделей; 0 моделей - для всех машин
	BitStream bs;
	bs.Write((uint8_t)220);
	bs.Write((uint8_t)111);
	bs.Write(speed > 0.0f ? speed : -1.0f);
	bs.Write((uint32_t)(modelid > 0 ? 1 : 0));
	if (modelid > 0) bs.Write((uint16_t)modelid);
	MBSendPacket(playerid, &bs, 1, 9);
	return true;
}
