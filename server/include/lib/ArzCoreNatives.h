#pragma once
// Нативы для пакетов модуля core.asi (см. ArzCore.h). Подключается в Plugin.cpp после ArzExtraNatives.h

#include <lib/ArzCore.h>
#include "../../Events.h"
#include <algorithm>
#include <lib/ArzExtraNatives.h>

// RemovePlayerWeapon(playerid, weaponid) - забрать у игрока одно оружие (остальные остаются)
// 220:200 [u8 оружие] -> CPed::ClearWeapon. Сервер SA-MP об этом не знает: GetPlayerWeaponData
// обновится после следующей синхронизации игрока
static cell AMX_NATIVE_CALL n_RemovePlayerWeapon(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Player(params[1]) || params[2] < 0 || params[2] > 46) return 0;
	ArzCore::SendU8((int)params[1], 200, (uint8_t)params[2]);
	return 1;
}

// SetPlayerWalkStyle(playerid, const style[]) - походка игрока: группа анимаций GTA ("man", "fatman"
// "sexywoman", "oldman", "gang1", "drunkman" ...). "" - обычная походка скина. Видят все
// 220:216 [u16 игрок][str8 группа]; новым зрителям досылается при появлении игрока
static cell AMX_NATIVE_CALL n_SetPlayerWalkStyle(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Player(params[1])) return 0;
	ArzCore::walkStyle[params[1]] = ArzNatives::Str(amx, params[2], 64);
	ArzCore::SendWalkStyle((int)params[1]);
	return 1;
}

// GetPlayerWalkStyle(playerid, style[], size = sizeof style)
static cell AMX_NATIVE_CALL n_GetPlayerWalkStyle(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::Player(params[1])) return 0;
	cell* dest = nullptr;
	if (amx_GetAddr(amx, params[2], &dest) != AMX_ERR_NONE || !dest || params[3] <= 0) return 0;
	amx_SetString(dest, ArzCore::walkStyle[params[1]].c_str(), 0, 0, (size_t)params[3]);
	return (cell)ArzCore::walkStyle[params[1]].size();
}

// TeleportPlayerWithFade(playerid, Float:x, Float:y, Float:z, Float:angle = -1.0, interior = -1, fade = 1000)
// Телепорт на стороне клиента с проявлением из чёрного экрана за fade мс (как у Arizona)
static cell AMX_NATIVE_CALL n_TeleportPlayerWithFade(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 7 || !ArzNatives::Player(params[1])) return 0;
	const float angle = amx_ctof(params[5]);
	const int interior = (int)params[6];
	// клиент: прогресс += шаг таймера / (время * 0.001) * 0.01 - 1.0 за время/500 секунд
	int fade = (int)params[7] / 2;
	if (fade < 1) fade = 1;
	if (fade > 0xFFFF) fade = 0xFFFF;
	BitStream bs;
	ArzExtra::Begin(bs, 175);
	bs.Write(amx_ctof(params[2]));
	bs.Write(amx_ctof(params[3]));
	bs.Write(amx_ctof(params[4]));
	bs.Write((uint16_t)fade);
	bs.Write((uint8_t)(interior < 0 ? 0 : interior));
	bs.Write(angle < 0.0f ? 0.0f : angle);
	bs.Write(interior >= 0);
	bs.Write(angle >= 0.0f);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// FadePlayerScreen(playerid, bool:toblack, time = 1000) - плавно затемнить экран игрока (toblack = true)
// или проявить из чёрного (false) за time мс. 220:176 [bit в чёрное][u16 время]
static cell AMX_NATIVE_CALL n_FadePlayerScreen(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::Player(params[1])) return 0;
	int fade = (int)params[3] / 2;
	if (fade < 1) fade = 1;
	if (fade > 0xFFFF) fade = 0xFFFF;
	BitStream bs;
	ArzExtra::Begin(bs, 176);
	bs.Write(params[2] != 0);
	bs.Write((uint16_t)fade);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// SetPlayerWaterLevel(playerid, Float:level, Float:from = -1.0, time = 0) - вода по всей карте на высоте level
// (потоп). level 0.0 - обычная вода GTA. from >= 0 и time > 0 - уровень плавно поднимается от from до level
// за time мс. 220:178 [u8 режим] 0: [float уровень], 1: [float от][float до][u32 мс]
static cell AMX_NATIVE_CALL n_SetPlayerWaterLevel(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 4 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	const float level = amx_ctof(params[2]), from = amx_ctof(params[3]);
	BitStream bs;
	ArzExtra::Begin(bs, 178);
	if (from >= 0.0f && params[4] > 0 && from != level)
	{
		bs.Write((uint8_t)1);
		bs.Write(from);
		bs.Write(level);
		bs.Write((uint32_t)params[4]);
	}
	else
	{
		bs.Write((uint8_t)0);
		bs.Write(level);
	}
	if (params[1] == -1)
	{
		// новым игрокам - сразу итоговый уровень, без плавного перехода
		BitStream fin;
		ArzExtra::Begin(fin, 178);
		fin.Write((uint8_t)0);
		fin.Write(level);
		ArzCore::RememberGlobal(178, fin);
	}
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// SetPlayerCompass(playerid, bool:enable) - компас Arizona над радаром (models/compass.txd). 220:87 [u8 0/1]
static cell AMX_NATIVE_CALL n_SetPlayerCompass(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendU8((int)params[1], 87, params[2] ? 1 : 0);
	return 1;
}

// SetPlayerFirstPerson(playerid, bool:enable) - включить/выключить вид от первого лица. 220:215 [bit]
static cell AMX_NATIVE_CALL n_SetPlayerFirstPerson(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Player(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 215, params[2] != 0);
	return 1;
}

// SetPlayerFlightLimits(playerid, bool:enable) - ограничение высоты полёта самолётов, вертолётов и джетпака
// (false - летать без потолка). 220:101 [bit без ограничения]
static cell AMX_NATIVE_CALL n_SetPlayerFlightLimits(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 101, params[2] == 0);
	return 1;
}

// SetPlayerBikeFall(playerid, bool:enable) - падение с мотоцикла/велосипеда при ударе (false - игрок не
// слетает с байка). 220:92 [bit не падать]
static cell AMX_NATIVE_CALL n_SetPlayerBikeFall(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 92, params[2] == 0);
	return 1;
}

// SetPlayerWorldEffects(playerid, bool:enable) - эффекты мира в RenderEffects (false - отключить)
// 220:102 [bit вкл]
static cell AMX_NATIVE_CALL n_SetPlayerWorldEffects(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 102, params[2] != 0);
	return 1;
}

// RedirectPlayer(playerid, const host[], port, const nickname[] = "", const password[] = "", bool:vicecity = false)
// переподключить игрока на другой сервер (как смена сервера у Arizona). nickname "" - с тем же ником
static cell AMX_NATIVE_CALL n_RedirectPlayer(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 5 || !ArzNatives::Player(params[1]) || params[3] <= 0 || params[3] > 65535) return 0;
	const std::string host = ArzNatives::Str(amx, params[2], 32);
	std::string nick = ArzNatives::Str(amx, params[4], 32);
	const std::string password = ArzNatives::Str(amx, params[5], 64);
	if (host.empty()) return 0;
	if (nick.empty()) nick = Platform::GetPlayerName((int)params[1]);
	BitStream bs;
	ArzExtra::Begin(bs, 103);
	ArzCore::WriteStr32(bs, host);
	bs.Write((uint32_t)params[3]);
	ArzCore::WriteStr32(bs, nick);
	ArzCore::WriteStr32(bs, password);
	bs.Write(ArzNatives::Args(params) >= 6 && params[6] != 0);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// SetPlayerSkinTexture(playerid, const texture[], color = 0, const newtexture[] = "", txd = 0) - перекрасить или
// заменить текстуру на скине игрока (как у Arizona: одежда, цвет кожи...). texture - имя текстуры в модели скина
static cell AMX_NATIVE_CALL n_SetPlayerSkinTexture(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 5 || !ArzNatives::Player(params[1]) || params[5] < 0 || params[5] > 255) return 0;
	ArzCore::SkinMaterial m;
	m.texture = ArzNatives::Str(amx, params[2], 0x8F);
	m.newTexture = ArzNatives::Str(amx, params[4], 0x8F);
	if (m.texture.empty()) return 0;
	if (m.newTexture.empty()) m.newTexture = m.texture;
	m.color = (uint32_t)params[3];
	m.txd = (uint8_t)params[5];
	auto& list = ArzCore::skinMaterials[params[1]];
	bool found = false;
	for (auto& old : list) if (old.texture == m.texture) { old = m; found = true; }
	if (!found)
	{
		if (list.size() >= ArzCore::MAX_SKIN_MATERIALS) return 0;
		list.push_back(m);
	}
	ArzCore::SendSkinMaterials((int)params[1]);
	return 1;
}

// ResetPlayerSkinTextures(playerid) - вернуть скину его текстуры
static cell AMX_NATIVE_CALL n_ResetPlayerSkinTextures(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::Player(params[1])) return 0;
	ArzCore::skinMaterials[params[1]].clear();
	ArzCore::SendSkinMaterials((int)params[1]);
	return 1;
}

// ShowPlayerLoadScreen(playerid, time, background = 0) - экран загрузки Arizona (Vice City): показать на time мс
// 0 - скрыть. background - фон 0..2. 220:104 [u8 0][u8 фон], затем [u8 1][u32 время]
static cell AMX_NATIVE_CALL n_ShowPlayerLoadScreen(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::Player(params[1]) || params[2] < 0 || params[3] < 0 || params[3] > 2) return 0;
	BitStream bg;
	ArzExtra::Begin(bg, 104);
	bg.Write((uint8_t)0);
	bg.Write((uint8_t)params[3]);
	ArzExtra::SendTo((int)params[1], bg);
	BitStream show;
	ArzExtra::Begin(show, 104);
	show.Write((uint8_t)1);
	show.Write((uint32_t)params[2]);
	ArzExtra::SendTo((int)params[1], show);
	if (params[2] == 0)
	{
		// 220:199 - убрать экран загрузки (клиент переводит его в состояние "скрыт")
		BitStream hide;
		ArzExtra::Begin(hide, 199);
		ArzExtra::SendTo((int)params[1], hide);
	}
	return 1;
}

// SetPlayerFlyingCar(playerid, bool:enable) - машина игрока летает: в воздухе управляется клавишами как самолёт
// (CVehicle::FlyingControl). Не для мотоциклов. 220:137 [bit]
static cell AMX_NATIVE_CALL n_SetPlayerFlyingCar(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Player(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 137, params[2] != 0);
	return 1;
}

// SetPlayerLaunchControl(playerid, bool:enable) - launch control Arizona: стоя на месте зажать W+S - "wait..."
// отпустить S - "go! go! go!" и разгон. 220:40 [u8]
static cell AMX_NATIVE_CALL n_SetPlayerLaunchControl(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Player(params[1])) return 0;
	ArzCore::SendU8((int)params[1], 40, params[2] ? 1 : 0);
	return 1;
}

// SetPlayerRadarRoute(playerid, bool:enable) - маршрут по дорогам на радаре до гоночного чекпоинта
// (SetPlayerRaceCheckpoint), как навигатор. 220:98 [bit]
static cell AMX_NATIVE_CALL n_SetPlayerRadarRoute(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 98, params[2] != 0);
	return 1;
}

// SetVehicleNitro(carid, bool:enable) - нитро-система Arizona на машине (сбрасывает заряды GTA). 220:156
static cell AMX_NATIVE_CALL n_SetVehicleNitro(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Car(params[1])) return 0;
	vTemp[params[1]].vNitro = params[2] != 0;
	ArzCore::SendVehicleNitro((int)params[1]);
	return 1;
}

// SetVehicleNitroColor(carid, color) - цвет пламени нитро: 0 обычное, 1 красное, 2 оранжевое, 3 розовое. 220:151
static cell AMX_NATIVE_CALL n_SetVehicleNitroColor(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Car(params[1]) || params[2] < 0 || params[2] > 3) return 0;
	vTemp[params[1]].vNitroColor = (int)params[2];
	ArzCore::SendVehicleNitro((int)params[1]);
	return 1;
}

// SetPortal(portalid, side, Float:x, Float:y, Float:z, Float:dirx, Float:diry, Float:dirz) - сторона портала
// (side 1 или 2): позиция и направление, куда смотрит портал. Вход в одну сторону - выход из другой
// portalid 0..1003, видят все игроки с лаунчером (в том числе зашедшие позже). 220:169
static cell AMX_NATIVE_CALL n_SetPortal(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 8 || params[1] < 0 || params[1] >= ArzCore::MAX_PORTALS || (params[2] != 1 && params[2] != 2)) return 0;
	const int key = (int)params[1] * 2 + (int)params[2] - 1;
	ArzCore::PortalSide& side = ArzCore::portals[key];
	for (int i = 0; i < 3; i++) { side.pos[i] = amx_ctof(params[3 + i]); side.dir[i] = amx_ctof(params[6 + i]); }
	ArzCore::SendPortalSide(key, -1);
	return 1;
}

// RemovePortal(portalid, side = 0) - убрать сторону портала (0 - обе). 220:170
static cell AMX_NATIVE_CALL n_RemovePortal(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || params[1] < 0 || params[1] >= ArzCore::MAX_PORTALS || params[2] < 0 || params[2] > 2) return 0;
	for (int side = 1; side <= 2; side++)
	{
		if (params[2] && params[2] != side) continue;
		if (ArzCore::portals.erase((int)params[1] * 2 + side - 1)) ArzCore::SendPortalRemove((int)params[1], side, -1);
	}
	return 1;
}

// SetPlayerPortals(playerid, bool:enable) - порталы работают у игрока (рисуются и телепортируют). 220:166 [bit]
static cell AMX_NATIVE_CALL n_SetPlayerPortals(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 166, params[2] != 0);
	return 1;
}

// SetPlayerScreenNoise(playerid, bool:enable) - помехи ("снег") поверх экрана, как в камере дрона Arizona. 220:149 [bit]
static cell AMX_NATIVE_CALL n_SetPlayerScreenNoise(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Player(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 149, params[2] != 0);
	return 1;
}

// LoadPlayerMap(playerid, const file[]) - загрузить у игрока карту Arizona из data\maps клиента
// (например "streamer_map_mirage.bin"). 220:165 [str8 имя файла]
static cell AMX_NATIVE_CALL n_LoadPlayerMap(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	const std::string file = ArzNatives::Str(amx, params[2], 200);
	if (file.empty() || file.find("..") != std::string::npos) return 0;
	if (params[1] == -1 && std::find(ArzCore::globalMaps.begin(), ArzCore::globalMaps.end(), file) == ArzCore::globalMaps.end())
		ArzCore::globalMaps.push_back(file); // для тех, кто зайдёт позже
	ArzCore::SendMap((int)params[1], file);
	return 1;
}

// ---- 220:188 объекты только для отрисовки (модель без сущности GTA: без коллизии, вне лимита объектов SA-MP)
// [u8 команда][u16 id] 0 создать: [u16 модель][3 float позиция][3 float поворот][bit][3 float масштаб][u32 0]
// 1 удалить, 2 позиция [3 float], 3 поворот [3 float], 4 масштаб [3 float]. playerid -1 - всем с лаунчером
namespace ArzVisual
{
	inline void Send3(int playerid, uint8_t op, int id, float x, float y, float z)
	{
		BitStream bs;
		ArzExtra::Begin(bs, 188);
		bs.Write(op);
		bs.Write((uint16_t)id);
		bs.Write(x); bs.Write(y); bs.Write(z);
		ArzExtra::SendTo(playerid, bs);
	}
}

// CreateVisualObject(playerid, id, modelid, Float:x, Float:y, Float:z, Float:rx = 0.0, Float:ry = 0.0, Float:rz = 0.0
//                    Float:sx = 1.0, Float:sy = 1.0, Float:sz = 1.0) - id 0..65535 свой у каждого игрока
static cell AMX_NATIVE_CALL n_CreateVisualObject(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 12 || !ArzNatives::PlayerOrAll(params[1]) || params[2] < 0 || params[2] > 0xFFFF || params[3] <= 0 || params[3] > 0xFFFF) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 188);
	bs.Write((uint8_t)0);
	bs.Write((uint16_t)params[2]);
	bs.Write((uint16_t)params[3]);
	for (int i = 4; i <= 9; i++) bs.Write(amx_ctof(params[i]));
	bs.Write(true);
	for (int i = 10; i <= 12; i++) bs.Write(amx_ctof(params[i]));
	bs.Write((uint32_t)0);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// DestroyVisualObject(playerid, id)
static cell AMX_NATIVE_CALL n_DestroyVisualObject(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1]) || params[2] < 0 || params[2] > 0xFFFF) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 188);
	bs.Write((uint8_t)1);
	bs.Write((uint16_t)params[2]);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// SetVisualObjectPos(playerid, id, Float:x, Float:y, Float:z)
static cell AMX_NATIVE_CALL n_SetVisualObjectPos(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 5 || !ArzNatives::PlayerOrAll(params[1]) || params[2] < 0 || params[2] > 0xFFFF) return 0;
	ArzVisual::Send3((int)params[1], 2, (int)params[2], amx_ctof(params[3]), amx_ctof(params[4]), amx_ctof(params[5]));
	return 1;
}

// SetVisualObjectRot(playerid, id, Float:rx, Float:ry, Float:rz)
static cell AMX_NATIVE_CALL n_SetVisualObjectRot(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 5 || !ArzNatives::PlayerOrAll(params[1]) || params[2] < 0 || params[2] > 0xFFFF) return 0;
	ArzVisual::Send3((int)params[1], 3, (int)params[2], amx_ctof(params[3]), amx_ctof(params[4]), amx_ctof(params[5]));
	return 1;
}

// SetVisualObjectScale(playerid, id, Float:sx, Float:sy, Float:sz)
static cell AMX_NATIVE_CALL n_SetVisualObjectScale(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 5 || !ArzNatives::PlayerOrAll(params[1]) || params[2] < 0 || params[2] > 0xFFFF) return 0;
	ArzVisual::Send3((int)params[1], 4, (int)params[2], amx_ctof(params[3]), amx_ctof(params[4]), amx_ctof(params[5]));
	return 1;
}

// SetPlayerDriveOnWater(playerid, bool:enable) - машины игрока ездят по воде (чит GTA "JESUSTAKETHEWHEEL"
// CCheat::m_aCheatsActive[34]; ввести его игрок не может - код у Arizona убран). 220:136 [bit]
static cell AMX_NATIVE_CALL n_SetPlayerDriveOnWater(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 136, params[2] != 0);
	return 1;
}

// ---- параметры оружия у игрока (модуль WeaponUpgrades core.asi)
// 220:157 [u8 оружие][u8 n] n x {u8 тип, значение}: тип 0 - u16 патронов в обойме (CWeaponInfo::m_nAmmoClip)
static bool WeaponUpgrade(int playerid, int weapon, uint8_t type, bool isFloat, cell value)
{
	if (weapon < 0 || weapon > 255) return false;
	BitStream bs;
	ArzExtra::Begin(bs, 157);
	bs.Write((uint8_t)weapon);
	bs.Write((uint8_t)1);
	bs.Write(type);
	if (isFloat) bs.Write(amx_ctof(value));
	else bs.Write((uint16_t)value);
	ArzExtra::SendTo(playerid, bs);
	return true;
}
// SetPlayerWeaponClip(playerid, weaponid, clip) - патронов в обойме у оружия (перезарядка после clip выстрелов)
static cell AMX_NATIVE_CALL n_SetPlayerWeaponClip(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::PlayerOrAll(params[1]) || params[3] < 1 || params[3] > 65535) return 0;
	return WeaponUpgrade((int)params[1], (int)params[2], 0, false, params[3]) ? 1 : 0;
}
// SetPlayerWeaponAccuracy(playerid, weaponid, Float:accuracy) - точность оружия (у GTA ~0.5..1.0, выше - кучнее)
static cell AMX_NATIVE_CALL n_SetPlayerWeaponAccuracy(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	return WeaponUpgrade((int)params[1], (int)params[2], 1, true, params[3]) ? 1 : 0;
}
// ResetPlayerWeaponStats(playerid, weaponid) - вернуть обойму и точность оружия как в игре
static cell AMX_NATIVE_CALL n_ResetPlayerWeaponStats(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1]) || params[2] < 0 || params[2] > 255) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 196);
	bs.Write((uint8_t)params[2]);
	bs.Write((uint8_t)0);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
// SetPlayerRadarIconsStyle(playerid, bool:arizona) - набор иконок радара: Arizona (hud) или как в GTA (hud_default). 220:147
static cell AMX_NATIVE_CALL n_SetPlayerRadarIconsStyle(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 147, params[2] != 0);
	return 1;
}

// ---- кастомное оружие Arizona (core/custom_weapon.txt у клиента: id 71+, модель, базовое оружие, иконка)
// GivePlayerCustomWeapon(playerid, weaponid, ammo) - выдать оружие с любым ID (в т.ч. 71+), минуя проверки сервера
static cell AMX_NATIVE_CALL n_GivePlayerCustomWeapon(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || params[1] < 0 || params[1] >= MAX_PLAYERS || params[2] < 0 || params[2] > 255) return 0;
	const int playerid = (int)params[1];
	if (!Platform::IsPlayerConnected(playerid)) return 0;
	const int weapon = PI[playerid].pLauncherStat ? (int)params[2] : Native::CallingBackWeapon((int)params[2]);
	BitStream bs;
	bs.Write((uint32_t)weapon);
	bs.Write((uint32_t)params[3]);
	return Platform::SendRPC(playerid, 22, &bs) ? 1 : 0;
}
// GetPlayerCustomWeapon(playerid) - полный ID оружия в руках (71+ - кастомное Arizona); GetPlayerWeapon
// видит только базовое. 0 - нет оружия или игрок ещё не прислал синхронизацию
static cell AMX_NATIVE_CALL n_GetPlayerCustomWeapon(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1) return 0;
	return Events::GetFullWeapon((int)params[1]);
}

// ---- 3D-иконки в мире (core.asi: createCustomMarker). Текстуры - из models\marker.txd клиента
// icon_1 .. icon_38, icon_closed, chicken_shop, chicken_airdrop, chicken_safe, crystal_off, lockdown_chest
static cell AMX_NATIVE_CALL n_CreatePlayerWorldIcon(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 8 || !ArzNatives::PlayerOrAll(params[1]) || params[2] < 0) return 0;
	const std::string texture = ArzNatives::Str(amx, params[3], 64), label = ArzNatives::Str(amx, params[8], 255);
	const float distance = amx_ctof(params[7]);
	if (texture.empty() || distance <= 0.0f) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 127);
	bs.Write((uint32_t)params[2]);
	bs.Write((uint8_t)(label.empty() ? 1 : 0));
	bs.Write((uint32_t)0);
	bs.Write((uint32_t)0);
	bs.Write(distance);
	bs.Write(amx_ctof(params[4]));
	bs.Write(amx_ctof(params[5]));
	bs.Write(amx_ctof(params[6]));
	ArzExtra::WriteStr8(bs, texture.c_str());
	ArzExtra::WriteStr8(bs, label.c_str());
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
static cell AMX_NATIVE_CALL n_DestroyPlayerWorldIcon(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1]) || params[2] < 0) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 128);
	bs.Write((uint32_t)params[2]);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
static cell AMX_NATIVE_CALL n_DestroyAllPlayerWorldIcons(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 129);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// ResetPlayerScreenFade(playerid) - сразу убрать затемнение FadePlayerScreen. 220:195
static cell AMX_NATIVE_CALL n_ResetPlayerScreenFade(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 195);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// SetPlayerAnimSpeed(playerid, const anims[], Float:speed, count = sizeof anims) - скорость анимаций GTA
// (AnimationId: 0 ходьба, 1 бег, 2 спринт...) у персонажа игрока - видят все. count 0 - вернуть как было
// 220:222 [u16 игрок][u8 n][n x u16 анимация][float скорость]
static cell AMX_NATIVE_CALL n_SetPlayerAnimSpeed(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 4 || !ArzNatives::Player(params[1])) return 0;
	cell* anims = nullptr;
	int count = (int)params[4];
	if (count < 0) count = 0;
	if (count > 64) count = 64;
	if (count && (amx_GetAddr(amx, params[2], &anims) != AMX_ERR_NONE || !anims)) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 222);
	bs.Write((uint16_t)params[1]);
	bs.Write((uint8_t)count);
	for (int i = 0; i < count; i++) bs.Write((uint16_t)anims[i]);
	bs.Write(amx_ctof(params[3]));
	ArzExtra::SendTo(-1, bs);
	return 1;
}

// SetPlayerTargetMarked(playerid, targetid, bool:marked) - отметить игрока targetid как цель у playerid
// (как SetPlayerTargetMode, режим 220:228). 220:229 [u16 игрок][bit отметить / снять]
static cell AMX_NATIVE_CALL n_SetPlayerTargetMarked(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::PlayerOrAll(params[1]) || params[2] < 0 || params[2] >= MAX_PLAYERS) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 229);
	bs.Write((uint16_t)params[2]);
	bs.Write(params[3] != 0);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// ShowPlayerTestDriveHud(playerid, value1, value2, value3) / HidePlayerTestDriveHud(playerid) - HUD тест-драйва
// (hud_testdrive): три числа 0-255. 220:89 [u8][u8][u8], 220:90 - скрыть
static cell AMX_NATIVE_CALL n_ShowPlayerTestDriveHud(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 4 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 89);
	for (int i = 2; i <= 4; i++) bs.Write((uint8_t)(params[i] < 0 ? 0 : (params[i] > 255 ? 255 : params[i])));
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}
static cell AMX_NATIVE_CALL n_HidePlayerTestDriveHud(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 90);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// TogglePlayerScoreboard(playerid, bool:enable) - таблица игроков по TAB (vorbisFile, ScoreboardTweaks)
// false - TAB её не открывает. 220:73 [u8 1 - выключить, 0 - включить]
static cell AMX_NATIVE_CALL n_TogglePlayerScoreboard(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendU8((int)params[1], 73, params[2] ? 0 : 1);
	return 1;
}

// SetPlayerCameraFollowVehicle(playerid, bool:enable) - камера наклоняется вместе с машиной (vorbisFile
// хук CCam::Process с гравитацией - езда по стенам/потолку). 220:211 [u8]
static cell AMX_NATIVE_CALL n_SetPlayerCameraFollowVehicle(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendU8((int)params[1], 211, params[2] ? 1 : 0);
	return 1;
}

// SetPlayerInfiniteStamina(playerid, bool:enable) - игрок не устаёт при беге и плавании
// (CPlayerInfo::m_bDoesNotGetTired). 220:91 [bit]
static cell AMX_NATIVE_CALL n_SetPlayerInfiniteStamina(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 91, params[2] != 0);
	return 1;
}

// SetPlayerBreath(playerid, Float:value) - запас воздуха под водой (CPlayerPedData::m_fBreath, у GTA до ~1150)
// 220:97 [u32 = float]
static cell AMX_NATIVE_CALL n_SetPlayerBreath(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Player(params[1])) return 0;
	const float value = amx_ctof(params[2]);
	uint32_t bits;
	std::memcpy(&bits, &value, 4);
	BitStream bs;
	ArzExtra::Begin(bs, 97);
	bs.Write(bits);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// SetPlayerBikeCruise(playerid, bool:enable) - круиз-контроль на мотоцикле: нажатие W на ходу включает /
// выключает удержание газа. 220:105 [bit]
static cell AMX_NATIVE_CALL n_SetPlayerBikeCruise(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 105, params[2] != 0);
	return 1;
}

// SetPlayerFirstPersonAim(playerid, bool:enable) - прицеливание от первого лица с любым оружием (клавиша
// прицела переключает режим снайпера). Вход / выход - public OnPlayerFirstPersonAim(playerid, aiming). 220:114 [bit]
static cell AMX_NATIVE_CALL n_SetPlayerFirstPersonAim(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 114, params[2] != 0);
	return 1;
}

// SetVehicleSilentEngine(carid, bool:enable) - двигатель машины без звука (как электромобиль). 220:150
static cell AMX_NATIVE_CALL n_SetVehicleSilentEngine(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Car(params[1])) return 0;
	vTemp[params[1]].vSilentEngine = params[2] != 0;
	ArzCore::SendSilentEngine((int)params[1]);
	return 1;
}

// GetVehicleTurnSignal(carid) - 0 выкл, 1 левый, 2 правый, 3 аварийка
static cell AMX_NATIVE_CALL n_GetVehicleTurnSignal(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::Car(params[1])) return 0;
	return vTemp[params[1]].vTurnSignal;
}

// RemovePlayerSkinTexture(playerid, const texture[]) - вернуть одной текстуре скина оригинал. 220:171 [u16 игрок][str16]
static cell AMX_NATIVE_CALL n_RemovePlayerSkinTexture(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Player(params[1])) return 0;
	const std::string texture = ArzNatives::Str(amx, params[2], 0x8F);
	auto& list = ArzCore::skinMaterials[params[1]];
	size_t before = list.size();
	for (size_t i = 0; i < list.size(); i++) if (list[i].texture == texture) list.erase(list.begin() + (long)i--);
	if (list.size() == before) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 171);
	bs.Write((uint16_t)params[1]);
	ArzCore::WriteStr16(bs, texture);
	ArzExtra::SendTo((int)params[1], bs);
	for (int i : connectedPlayers)
		if (i != params[1] && Platform::IsPlayerStreamedIn((int)params[1], i)) ArzExtra::SendTo(i, bs);
	return 1;
}

// SetPlayerCompassTarget(playerid, Float:x, Float:y) - точка, на которую показывает компас (SetPlayerCompass). 220:88
static cell AMX_NATIVE_CALL n_SetPlayerCompassTarget(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 88);
	bs.Write(amx_ctof(params[2]));
	bs.Write(amx_ctof(params[3]));
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// SetVehicleHeadlightTexture(carid, const texture[]) - текстура фар машины по имени (как у Arizona), "" - стандартная
// 220:193
static cell AMX_NATIVE_CALL n_SetVehicleHeadlightTexture(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Car(params[1])) return 0;
	vTemp[params[1]].vHeadlightTexture = ArzNatives::Str(amx, params[2], 200);
	ArzCore::SendHeadlightTexture((int)params[1]);
	return 1;
}

// SetPlayerCanSwim(playerid, bool:enable) - в режиме Vice City (RedirectPlayer(..., vicecity = true) или сервер
// Arizona VC) игрок в воде сразу тонет, как в оригинальной Vice City; true - разрешить плавать. 220:117 [bit]
static cell AMX_NATIVE_CALL n_SetPlayerCanSwim(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 117, params[2] != 0);
	return 1;
}

// SetPlayerLightMode(playerid, mode) - свет от игрока (модуль света Arizona): 0 - выкл, 1 - пятно света у игрока
// 2 - пятно, вынесенное вперёд. Видят остальные игроки (у самого игрока клиент рисует по своей логике). 220:180
static cell AMX_NATIVE_CALL n_SetPlayerLightMode(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Player(params[1]) || params[2] < 0 || params[2] > 2) return 0;
	ArzCore::lightMode[params[1]] = (uint8_t)params[2];
	ArzCore::SendLightMode((int)params[1]);
	return 1;
}

// ---- 220:212 линии (модуль Lines): 3D-линия между двумя точками - тросы, лучи, провода
// [u8 0][u8 id] - удалить; [u8 1][u8 id][u8 толщина * 200][u8 0][u32 цвет][u32 цвет 2] + 2 точки
namespace ArzLines
{
	enum { WORLD, PLAYER, VEHICLE, OBJECT, ACTOR, PLAYER_BONE };

	inline bool WritePoint(BitStream& bs, int type, int id, float x, float y, float z)
	{
		switch (type)
		{
		case WORLD:
			bs.Write((uint8_t)0);
			bs.Write(x); bs.Write(y); bs.Write(z);
			return true;
		case PLAYER: case VEHICLE: case OBJECT: case ACTOR:
			bs.Write((uint8_t)1);
			bs.Write((uint8_t)(type - PLAYER));
			bs.Write((uint16_t)id);
			bs.Write(x); bs.Write(y); bs.Write(z);
			return true;
		case PLAYER_BONE:
			bs.Write((uint8_t)2);
			bs.Write((uint8_t)0);
			bs.Write((uint16_t)id);
			bs.Write((uint16_t)x); // кость - в fx
			return true;
		}
		return false;
	}

	// 0xRRGGBBAA -> цвет вершины D3D 0xAARRGGBB
	inline uint32_t Argb(cell rgba) { const uint32_t c = (uint32_t)rgba; return (c >> 8) | (c << 24); }
}

// CreateLine(playerid, lineid, fromtype, fromid, Float:fx, Float:fy, Float:fz, totype, toid, Float:tx, Float:ty, Float:tz
//            color = 0xFFFFFFFF, Float:width = 0.05, color2 = 0)
static cell AMX_NATIVE_CALL n_CreateLine(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 15 || !ArzNatives::PlayerOrAll(params[1]) || params[2] < 0 || params[2] > 255) return 0;
	float width = amx_ctof(params[14]) * 200.0f;
	if (width < 1.0f) width = 1.0f;
	if (width > 255.0f) width = 255.0f;
	BitStream bs;
	ArzExtra::Begin(bs, 212);
	bs.Write((uint8_t)1);
	bs.Write((uint8_t)params[2]);
	bs.Write((uint8_t)width);
	bs.Write((uint8_t)0);
	bs.Write(ArzLines::Argb(params[13]));
	bs.Write(ArzLines::Argb(params[15] ? params[15] : params[13]));
	if (!ArzLines::WritePoint(bs, (int)params[3], (int)params[4], amx_ctof(params[5]), amx_ctof(params[6]), amx_ctof(params[7]))) return 0;
	if (!ArzLines::WritePoint(bs, (int)params[8], (int)params[9], amx_ctof(params[10]), amx_ctof(params[11]), amx_ctof(params[12]))) return 0;
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// DestroyLine(playerid, lineid)
static cell AMX_NATIVE_CALL n_DestroyLine(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1]) || params[2] < 0 || params[2] > 255) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 212);
	bs.Write((uint8_t)0);
	bs.Write((uint8_t)params[2]);
	ArzExtra::SendTo((int)params[1], bs);
	return 1;
}

// SetPlayerHeadMove(playerid, bool:enable) - голова персонажа поворачивается туда, куда смотрит камера
// (настройка /headmove SA-MP, CGame+0x59 в samp.dll). 220:221 [bit]
static cell AMX_NATIVE_CALL n_SetPlayerHeadMove(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 221, params[2] != 0);
	return 1;
}

// SetPlayerCJAnims(playerid, bool:enable) - у игрока его персонаж ходит и бегает анимациями CJ, как при
// UsePlayerPedAnims, но только для него самого (CPlayerPed::ProcessAnimGroups). 220:163 [bit]
static cell AMX_NATIVE_CALL n_SetPlayerCJAnims(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::PlayerOrAll(params[1])) return 0;
	ArzCore::SendBit((int)params[1], 163, params[2] != 0);
	return 1;
}
