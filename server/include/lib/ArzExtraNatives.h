#pragma once
// Нативы для пакетов ядра Arizona (см. ArzExtra.h). Подключается в Plugin.cpp после FuncNative.h

#include <lib/ArzExtra.h>
#include "../../Events.h"
#include <pawn/amx/amx.h>
#include <pawn/plugincommon.h>
#include <string>
#include <vector>

namespace ArzNatives
{
	inline int Args(cell* params) { return (int)(params[0] / (cell)sizeof(cell)); }

	inline std::string Str(AMX* amx, cell param, int max = 512)
	{
		cell* ptr = nullptr;
		if (amx_GetAddr(amx, param, &ptr) != AMX_ERR_NONE || !ptr) return "";
		std::vector<char> buf(max + 1, 0);
		amx_GetString(buf.data(), ptr, 0, max + 1);
		return buf.data();
	}

	inline bool Car(cell carid) { return carid > 0 && carid < MAX_VEHICLES && Platform::IsVehicleValid((int)carid); }
	inline bool Player(cell playerid) { return playerid >= 0 && playerid < MAX_PLAYERS && Platform::IsPlayerConnected((int)playerid); }
	inline bool PlayerOrAll(cell playerid) { return playerid == -1 || Player(playerid); }
}

// SetVehicleDirt(carid, Float:dirt, Float:rate = 1.0, wear = 1) - грязь 0..100, rate - скорость набора грязи
// при езде (0 - не пачкается), wear - износ кузова 0..17 (0-1 нет, 2..17 - текстуры потёртостей)
static cell AMX_NATIVE_CALL n_SetVehicleDirt(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 4 || !ArzNatives::Car(params[1])) return 0;
	VehicleInfo& v = vTemp[params[1]];
	float dirt = amx_ctof(params[2]);
	v.vDirt = dirt < 0.0f ? 0.0f : (dirt > 100.0f ? 100.0f : dirt);
	v.vDirtRate = amx_ctof(params[3]) < 0.0f ? 0.0f : amx_ctof(params[3]);
	v.vWear = params[4] < 0 ? 0 : (params[4] > 17 ? 17 : (int)params[4]);
	ArzExtra::SendVehicleDirt((int)params[1]);
	return 1;
}
// Float:GetVehicleDirt(carid) - текущая грязь (её обновляет и водитель, пока едет); -1 - не задавалась
static cell AMX_NATIVE_CALL n_GetVehicleDirt(AMX* amx, cell* params)
{
	float dirt = -1.0f;
	if (ArzNatives::Args(params) >= 1 && params[1] > 0 && params[1] < MAX_VEHICLES) dirt = vTemp[params[1]].vDirt;
	return amx_ftoc(dirt);
}
// SetVehicleTurnLightColor(carid, color) - цвет подсветки поворотников на земле, 0xRRGGBBAA (AA - яркость)
static cell AMX_NATIVE_CALL n_SetVehicleTurnLightColor(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Car(params[1])) return 0;
	vTemp[params[1]].vTurnColor = (uint32_t)params[2];
	vTemp[params[1]].vTurnColorSet = true;
	ArzExtra::SendTurnLightColor((int)params[1]);
	return 1;
}
// SetObjectDirty(objectid, level, texture = 0) - грязь глобального объекта (CreateObject, id < 1000)
// level 0..15 (0 - чистый), texture 0 - грязь, 1 - потёртости
static cell AMX_NATIVE_CALL n_SetObjectDirty(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || params[1] < 0 || params[1] >= ArzExtra::MAX_DIRTY_OBJECTS) return 0;
	ArzExtra::objDirt[params[1]] = (uint8_t)(params[2] < 0 ? 0 : (params[2] > 15 ? 15 : params[2]));
	ArzExtra::objDirtTex[params[1]] = params[3] == 1 ? 1 : 0;
	ArzExtra::SendObjectDirt((int)params[1]);
	return 1;
}
// SetPlayerAttachedObjectDirty(playerid, slot, level, texture = 0) - грязь аксессуара игрока (слот 0..9)
static cell AMX_NATIVE_CALL n_SetPlayerAttachedObjectDirty(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 4 || !ArzNatives::Player(params[1]) || params[2] < 0 || params[2] >= ArzExtra::ACS_SLOTS) return 0;
	ArzExtra::acsDirt[params[1]][params[2]] = (uint8_t)(params[3] < 0 ? 0 : (params[3] > 15 ? 15 : params[3]));
	ArzExtra::acsDirtTex[params[1]][params[2]] = params[4] == 1 ? 1 : 0;
	ArzExtra::SendAcsDirt((int)params[1], (int)params[2]);
	return 1;
}

// ---- 220:47 / 48 / 49 эффекты частиц GTA
static bool SendParticle(AMX* amx, cell* params, uint8_t sub)
{
	if (ArzNatives::Args(params) < 7 || !ArzNatives::PlayerOrAll(params[1])) return false;
	std::string name = ArzNatives::Str(amx, params[2], 31);
	if (name.empty()) return false;
	BitStream bs;
	ArzExtra::Begin(bs, sub);
	ArzExtra::WriteStr8(bs, name.c_str());
	bs.Write(amx_ctof(params[3]));
	bs.Write(amx_ctof(params[4]));
	bs.Write(amx_ctof(params[5]));
	bs.Write((uint8_t)params[6]);
	bs.Write((uint32_t)params[7]);
	ArzExtra::SendTo((int)params[1], bs);
	return true;
}
// CreateParticle(playerid, const name[], Float:x, Float:y, Float:z, interior = 0, period = -1)
// playerid -1 - всем; period -1 - один раз, иначе эффект пересоздаётся каждые period мс
static cell AMX_NATIVE_CALL n_CreateParticle(AMX* amx, cell* params) { return SendParticle(amx, params, 47) ? 1 : 0; }
// DestroyParticle - те же аргументы, что при создании (клиент ищет точное совпадение)
static cell AMX_NATIVE_CALL n_DestroyParticle(AMX* amx, cell* params) { return SendParticle(amx, params, 48) ? 1 : 0; }
static cell AMX_NATIVE_CALL n_DestroyAllParticles(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 49);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// SetPlayerWeaponWhitelist(playerid, bool:enable, const weapons[] = {0}, count = 0)
// пока включено, игрок переключается только на оружие из списка (не из списка - кулак)
static cell AMX_NATIVE_CALL n_SetPlayerWeaponWhitelist(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 4 || !ArzNatives::Player(params[1])) return 0;
	int count = params[4] < 0 ? 0 : (params[4] > 64 ? 64 : (int)params[4]);
	cell* arr = nullptr;
	if (count && (amx_GetAddr(amx, params[3], &arr) != AMX_ERR_NONE || !arr)) count = 0;
	BitStream bs;
	ArzExtra::Begin(bs, 110);
	bs.Write((uint8_t)(params[2] ? 1 : 0));
	bs.Write((uint8_t)count);
	for (int i = 0; i < count; i++) bs.Write((uint8_t)arr[i]);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// SetPlayerChatMode(playerid, mode) - чат: 0 скрыт, 1 лёгкий, 2 полный
static cell AMX_NATIVE_CALL n_SetPlayerChatMode(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1]) || params[2] < 0 || params[2] > 2) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 52);
	bs.Write((uint8_t)params[2]);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
// LockPlayerChatMode(playerid, bool:lock) - F5/F10 больше не меняют режим чата
static cell AMX_NATIVE_CALL n_LockPlayerChatMode(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 174);
	bs.Write(params[2] != 0);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// AddAdminReport(adminid, fromid, const text[]) - жалоба во всплывающем списке админа
// (кнопки "Ответить" -> /ot id, "Следить" -> /re id); RemoveAdminReport(adminid, fromid)
static cell AMX_NATIVE_CALL n_AddAdminReport(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::PlayerOrAll(params[1]) || !ArzNatives::Player(params[2])) return 0;
	std::string text = ArzNatives::Str(amx, params[3], 255);
	if (text.empty()) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 45);
	bs.Write((int16_t)params[2]);
	ArzExtra::WriteStr8(bs, text.c_str());
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
static cell AMX_NATIVE_CALL n_RemoveAdminReport(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 46);
	bs.Write((int16_t)params[2]);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// SetPlayerZoneWorld(playerid, world) - названия зон (zones.bin) показываются для этого виртуального мира
static cell AMX_NATIVE_CALL n_SetPlayerZoneWorld(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Player(params[1])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 183);
	bs.Write((uint32_t)params[2]);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// UpdatePlayerDialogText(playerid, const text[]) - заменить текст открытого диалога
// Диалог показывается заново с тем же ID, стилем, заголовком и кнопками: 220:59 нынешние диалоги
// Arizona не меняет (пакет доходит, текст остаётся старым). Диалога нет - старый 220:59
static cell AMX_NATIVE_CALL n_UpdatePlayerDialogText(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Player(params[1])) return 0;
	std::string text = ArzNatives::Str(amx, params[2], 4096);
	if (Events::ResendDialogText((int)params[1], text)) return 1;
	BitStream bs;
	ArzExtra::Begin(bs, 59);
	bs.Write((uint32_t)text.size());
	if (!text.empty()) bs.Write(text.c_str(), (int)text.size());
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
// SetPlayerDialogRemember(playerid, bool:on) - диалоги-списки запоминают выбранный пункт
static cell AMX_NATIVE_CALL n_SetPlayerDialogRemember(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 39);
	bs.Write((uint8_t)(params[2] ? 1 : 0));
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
// SetPlayerDialogStyle(playerid, level) - оформление диалогов Arizona, 1..5
static cell AMX_NATIVE_CALL n_SetPlayerDialogStyle(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1]) || params[2] < 0 || params[2] > 5) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 53);
	bs.Write((uint8_t)params[2]);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// SetChatTabFormat(playerid, const tab[], const format[]) - формат сообщений вкладки чата: {NICK}, {TEXT}
static cell AMX_NATIVE_CALL n_SetChatTabFormat(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	std::string tab = ArzNatives::Str(amx, params[2], 64), format = ArzNatives::Str(amx, params[3], 255);
	if (tab.empty()) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 44);
	ArzExtra::WriteStr8(bs, tab.c_str());
	ArzExtra::WriteStr8(bs, format.c_str());
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// SetPlayerAcsMaterial(playerid, slot, index, const txd[], const texture[], color = 0)
// материал на аксессуаре (как SetPlayerAttachedObject + SetObjectMaterial), color 0xAARRGGBB
static cell AMX_NATIVE_CALL n_SetPlayerAcsMaterial(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 6 || !ArzNatives::Player(params[1]) || params[2] < 0 || params[2] > 14 || params[3] < 0 || params[3] > 15) return 0;
	std::string txd = ArzNatives::Str(amx, params[4], 64), tex = ArzNatives::Str(amx, params[5], 64);
	BitStream bs;
	ArzExtra::Begin(bs, 194);
	bs.Write((int16_t)params[1]);
	bs.Write((uint16_t)params[2]);
	bs.Write((int8_t)params[3]);
	ArzExtra::WriteStr8(bs, txd.c_str());
	ArzExtra::WriteStr8(bs, tex.c_str());
	bs.Write((uint32_t)params[6]);
	Native::SendPackerPlayer(bs, -1, (int)params[1], -1, 1);
	return 1;
}
// SetPlayerAcsMaterialText(playerid, slot, index, const text[], matsize = 90, fontsize = 24, align = 1
//                          bool:bold = false, const font[] = "Arial", fontcolor = 0xFFFFFFFF, backcolor = 0)
// текст на аксессуаре (как SetObjectMaterialText), цвета 0xRRGGBBAA; пустой text - убрать
static cell AMX_NATIVE_CALL n_SetPlayerAcsMaterialText(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 11 || !ArzNatives::Player(params[1]) || params[2] < 0 || params[2] > 14 || params[3] < 0 || params[3] > 15) return 0;
	std::string text = ArzNatives::Str(amx, params[4], 255), font = ArzNatives::Str(amx, params[9], 64);
	BitStream bs;
	ArzExtra::Begin(bs, 226);
	bs.Write((int16_t)params[1]);
	bs.Write((uint16_t)params[2]);
	bs.Write((int8_t)params[3]);
	ArzExtra::WriteStr8(bs, text.c_str());
	if (!text.empty())
	{
		bs.Write((uint8_t)params[5]);
		bs.Write((uint8_t)params[6]);
		bs.Write((uint8_t)params[7]);
		bs.Write(params[8] != 0);
		ArzExtra::WriteStr8(bs, font.c_str());
		for (int c = 10; c <= 11; c++)
		{
			const uint32_t v = (uint32_t)params[c];
			bs.Write((uint8_t)(v >> 24));
			bs.Write((uint8_t)(v >> 16));
			bs.Write((uint8_t)(v >> 8));
			bs.Write((uint8_t)v);
		}
	}
	Native::SendPackerPlayer(bs, -1, (int)params[1], -1, 1);
	return 1;
}

// ======== вторая половина ядра (модули с отдельным сабтипом 220) ========

static cell SendBits(cell playerid, uint8_t sub, std::initializer_list<bool> bits)
{
	if (!ArzNatives::PlayerOrAll(playerid)) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, sub);
	for (bool b : bits) bs.Write(b);
	ArzExtra::SendTo((int)playerid, bs);
	return 1;
}

// SetPlayerBulletTracer(shooterid, color, Float:time, bool:laser = false) - трассеры пуль игрока (видят все)
// color 0xAARRGGBB, time - сколько видно (сек). ResetPlayerBulletTracer(shooterid) - стандартные
static cell AMX_NATIVE_CALL n_SetPlayerBulletTracer(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 4 || !ArzNatives::Player(params[1])) return 0;
	ArzExtra::Tracer& t = ArzExtra::tracer[params[1]];
	t.set = true; t.color = (uint32_t)params[2]; t.time = amx_ctof(params[3]); t.laser = params[4] != 0;
	ArzExtra::SendTracer((int)params[1]);
	return 1;
}
static cell AMX_NATIVE_CALL n_ResetPlayerBulletTracer(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::Player(params[1])) return 0;
	ArzExtra::tracer[params[1]] = ArzExtra::Tracer();
	ArzExtra::SendTracer((int)params[1]);
	return 1;
}
// SetPlayerWeaponTracer(shooterid, weaponid, color, Float:time, bool:laser = false) - трассер для одного оружия
static cell AMX_NATIVE_CALL n_SetPlayerWeaponTracer(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 5 || !ArzNatives::Player(params[1])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 35);
	bs.Write((int16_t)params[1]);
	bs.Write((uint8_t)params[2]);
	bs.Write((uint32_t)params[3]);
	bs.Write(amx_ctof(params[4]));
	bs.Write(params[5] != 0);
	ArzExtra::SendTo(-1, bs);
	return 1;
}

// SetPlayerMouseTracking(playerid, mode, Float:mindelta = 0.0) - 0 выкл, 1 слать каждое движение
// 2 - только если сдвинули больше mindelta. Клиент шлёт позицию курсора -> OnPlayerMouseMove(playerid, Float:x, Float:y)
static cell AMX_NATIVE_CALL n_SetPlayerMouseTracking(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::Player(params[1]) || params[2] < 0 || params[2] > 2) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 57);
	bs.Write((uint8_t)params[2]);
	if (params[2] == 2) bs.Write(amx_ctof(params[3]));
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
// MoveTextDrawForPlayer(playerid, textid, Float:x, Float:y) - сдвинуть показанный текстдрав без пересоздания
// (глобальный - id как есть, PlayerTextDraw - id + 2048)
static cell AMX_NATIVE_CALL n_MoveTextDrawForPlayer(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 4 || !ArzNatives::Player(params[1]) || params[2] < 0 || params[2] > 2303) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 58);
	bs.Write((uint16_t)params[2]);
	bs.Write(amx_ctof(params[3]));
	bs.Write(amx_ctof(params[4]));
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
// SetPlayerWaypoint(playerid, Float:x, Float:y, Float:z) / RemovePlayerWaypoint(playerid) - метка навигатора на карте
static cell AMX_NATIVE_CALL n_SetPlayerWaypoint(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 4 || !ArzNatives::Player(params[1])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 65);
	bs.Write(true);
	bs.Write(amx_ctof(params[2])); bs.Write(amx_ctof(params[3])); bs.Write(amx_ctof(params[4]));
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
static cell AMX_NATIVE_CALL n_RemovePlayerWaypoint(AMX* amx, cell* params) { return ArzNatives::Args(params) >= 1 ? SendBits(params[1], 65, { false }) : 0; }
// SetPlayerDiscordStatus(playerid, const text[]) - свой текст в Discord Rich Presence; "" - убрать
static cell AMX_NATIVE_CALL n_SetPlayerDiscordStatus(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	std::string text = ArzNatives::Str(amx, params[2], 127);
	BitStream bs;
	if (text.empty()) ArzExtra::Begin(bs, 82);
	else
	{
		ArzExtra::Begin(bs, 80);
		bs.Write((uint32_t)text.size());
		bs.Write(text.c_str(), (int)text.size());
	}
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
// SetPlayerCommandHelper(playerid, bool:enable) - подсказки команд в чате
static cell AMX_NATIVE_CALL n_SetPlayerCommandHelper(AMX* amx, cell* params) { return ArzNatives::Args(params) >= 2 ? SendBits(params[1], 72, { params[2] != 0 }) : 0; }
// SetInteriorClimbing(playerid, interior, bool:disable) - запретить карабкаться в интерьере (playerid -1 - всем)
static cell AMX_NATIVE_CALL n_SetInteriorClimbing(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::PlayerOrAll(params[1]) || params[2] < 0) return 0; // 0 - основной мир
	BitStream bs;
	ArzExtra::Begin(bs, 77);
	bs.Write((uint32_t)params[2]);
	bs.Write(params[3] != 0);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
// SetPlayerJetpackVisibility(playerid, mode) - 1 джетпаки видны всем, 2 - скрыт только свой, 0 - скрыты все
static cell AMX_NATIVE_CALL n_SetPlayerJetpackVisibility(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 126);
	bs.Write((uint8_t)params[2]);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
// SetObjectScale(objectid, Float:scale, bool:collision = true) - масштаб объекта CreateObject (id < 1000), для всех
static cell AMX_NATIVE_CALL n_SetObjectScale(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || params[1] < 0 || params[1] >= ArzExtra::MAX_DIRTY_OBJECTS) return 0;
	ArzExtra::objScale[params[1]] = amx_ctof(params[2]);
	ArzExtra::objScaleCol[params[1]] = params[3] != 0;
	ArzExtra::SendObjectScale((int)params[1]);
	return 1;
}
// AttachVehicleToVehicle(carid, slot, model, Float:x, Float:y, Float:z, Float:rx = 0.0, Float:ry = 0.0, Float:rz = 0.0
//                        color1 = 1, color2 = 1, Float:health = 1000.0, paintjob = 3, bool:collision = false)
// клиент рисует на машине carid ещё одну машину (эвакуатор, автовоз, прицеп), слот 0..9
static cell AMX_NATIVE_CALL n_AttachVehicleToVehicle(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 14 || !ArzNatives::Car(params[1]) || params[2] < 0 || params[2] >= ArzExtra::VEH_ATTACH_SLOTS) return 0;
	ArzExtra::VehAttach& a = ArzExtra::vehAttach[params[1]][params[2]];
	a = ArzExtra::VehAttach();
	a.used = true;
	a.model = (uint16_t)params[3];
	for (int i = 0; i < 3; i++) { a.off[i] = amx_ctof(params[4 + i]); a.rot[i] = amx_ctof(params[7 + i]); }
	a.color1 = (uint8_t)params[10]; a.color2 = (uint8_t)params[11];
	a.health = amx_ctof(params[12]);
	a.paintjob = (uint8_t)params[13];
	a.flags = params[14] ? 0x40 : 0;
	ArzExtra::SendVehAttach((int)params[1], (int)params[2]);
	return 1;
}
static cell AMX_NATIVE_CALL n_DetachVehicleFromVehicle(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Car(params[1]) || params[2] < 0 || params[2] >= ArzExtra::VEH_ATTACH_SLOTS) return 0;
	ArzExtra::vehAttach[params[1]][params[2]].used = false;
	ArzExtra::SendVehAttach((int)params[1], (int)params[2]);
	return 1;
}
// SetPlayerSkyLogo(playerid, const text[], bool:visible = true, alpha = 255, fadestep = 10
//                  Float:x = 0.0, Float:y = 0.0, Float:z = 0.0, Float:radius = 0.0) - надпись в небе ("строка1;строка2")
// radius > 0 - видно только около точки (плавно гаснет к radius)
static cell AMX_NATIVE_CALL n_SetPlayerSkyLogo(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 9 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	std::string text = ArzNatives::Str(amx, params[2], 255);
	bool visible = params[3] != 0;
	if (text.empty()) { text = " "; visible = false; }
	BitStream bs;
	ArzExtra::Begin(bs, 144);
	ArzExtra::WriteStr8(bs, text.c_str());
	bs.Write((uint8_t)params[5]);
	bs.Write((uint8_t)params[4]);
	bs.Write(visible);
	const float r = amx_ctof(params[9]);
	if (r > 0.0f)
	{
		bs.Write(amx_ctof(params[6])); bs.Write(amx_ctof(params[7])); bs.Write(amx_ctof(params[8]));
		bs.Write(r); bs.Write(r * 0.5f);
	}
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
// SetVehicleHornSound(carid, hornid) - звук клаксона (id звука GTA), -1 - стандартный
static cell AMX_NATIVE_CALL n_SetVehicleHornSound(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Car(params[1])) return 0;
	ArzExtra::vehHorn[params[1]] = params[2] < 0 || params[2] > 254 ? 0 : (int)params[2] + 1;
	ArzExtra::SendHorn((int)params[1]);
	return 1;
}
// SetPlayerNavigation(playerid, const Float:points[], count, bool:onfoot = true, bool:tilt = true) - маршрут
// стрелки навигатора: points = {x, y, z, radius, x, y, z, radius...}, count - число точек; count 0 - убрать
// Дошёл до точки -> OnPlayerNavigationPoint(playerid, index)
static cell AMX_NATIVE_CALL n_SetPlayerNavigation(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 5 || !ArzNatives::Player(params[1])) return 0;
	int count = params[3] < 0 ? 0 : (params[3] > 255 ? 255 : (int)params[3]);
	cell* arr = nullptr;
	if (count && (amx_GetAddr(amx, params[2], &arr) != AMX_ERR_NONE || !arr)) count = 0;
	BitStream bs;
	ArzExtra::Begin(bs, 197);
	bs.Write(params[4] != 0);
	bs.Write(params[5] != 0);
	bs.Write((uint8_t)count);
	for (int i = 0; i < count; i++)
		for (int k = 0; k < 4; k++) bs.Write((int16_t)amx_ctof(arr[i * 4 + k]));
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
// SetPlayerAntiPause(playerid, bool:enable) - игра не ставится на паузу (не AFK) при сворачивании
static cell AMX_NATIVE_CALL n_SetPlayerAntiPause(AMX* amx, cell* params) { return ArzNatives::Args(params) >= 2 ? SendBits(params[1], 54, { params[2] != 0 }) : 0; }
// SetPlayerBoneScale(playerid, bone, Float:scale) - увеличить кость игрока (видят все), scale <= 1.0 - убрать
// bone -1 - сбросить все
static cell AMX_NATIVE_CALL n_SetPlayerBoneScale(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::Player(params[1])) return 0;
	auto& list = ArzExtra::bones[params[1]];
	if (params[2] < 0) list.clear();
	else
	{
		const float scale = amx_ctof(params[3]);
		for (size_t i = 0; i < list.size(); i++) if (list[i].bone == (uint16_t)params[2]) { list.erase(list.begin() + i); break; }
		if (scale > 1.0f && list.size() < 32) list.push_back({ (uint16_t)params[2], scale });
	}
	ArzExtra::SendBones((int)params[1]);
	return 1;
}
// SetPlayerAirBrake(playerid, bool:enable, Float:speed = 100.0) - полёт (режим админа), скорость от 25
static cell AMX_NATIVE_CALL n_SetPlayerAirBrake(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::Player(params[1])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 233);
	bs.Write(params[2] != 0);
	bs.Write(amx_ctof(params[3]) >= 25.0f ? amx_ctof(params[3]) : 100.0f);
	bs.Write((uint16_t)params[1]);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
