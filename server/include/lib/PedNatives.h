#pragma once
// Нативы педов (ModGear_NPC.inc). Подключается в Plugin.cpp

#include <lib/Peds.h>
#include <lib/ArzChat.h>
#include <pawn/amx/amx.h>
#include <pawn/plugincommon.h>
#include <string>
#include <vector>

namespace PedNatives
{
	inline int Args(cell* params) { return (int)(params[0] / (cell)sizeof(cell)); }
	inline bool Ok(cell* params, int need) { return Args(params) >= need && Peds::Valid((int)params[1]); }
	inline std::string Str(AMX* amx, cell param, int max)
	{
		cell* ptr = nullptr;
		if (amx_GetAddr(amx, param, &ptr) != AMX_ERR_NONE || !ptr) return "";
		std::vector<char> buf(max + 1, 0);
		amx_GetString(buf.data(), ptr, 0, max + 1);
		return buf.data();
	}
	inline void SetRef(AMX* amx, cell param, cell value)
	{
		cell* ptr = nullptr;
		if (amx_GetAddr(amx, param, &ptr) == AMX_ERR_NONE && ptr) *ptr = value;
	}
	inline Peds::Ped& P(cell* params) { return Peds::peds[params[1]]; }
	inline uint16_t Id(cell* params) { return (uint16_t)params[1]; }
}

using namespace PedNatives;

// CreatePed(model, Float:x, Float:y, Float:z, Float:angle = 0.0, world = 0, interior = 0) - id педа или -1
static cell AMX_NATIVE_CALL n_CreatePed(AMX* amx, cell* params)
{
	if (Args(params) < 7) return -1;
	return Peds::Create((int)params[1], amx_ctof(params[2]), amx_ctof(params[3]), amx_ctof(params[4]), amx_ctof(params[5]), (int)params[6], (int)params[7], amx);
}
static cell AMX_NATIVE_CALL n_DestroyPed(AMX* amx, cell* params)
{
	if (!Ok(params, 1)) return 0;
	Peds::Destroy((int)params[1]);
	if (params[1] >= 0 && params[1] < ArzChat::MAX_PEDS) ArzChat::pedGradSet[params[1]] = false;
	return 1;
}
static cell AMX_NATIVE_CALL n_IsValidPed(AMX* amx, cell* params) { return Ok(params, 1) ? 1 : 0; }
// GetPedPoolSize() - наибольший id занятого педа + 1 (для циклов)
static cell AMX_NATIVE_CALL n_GetPedPoolSize(AMX* amx, cell* params)
{
	for (int id = Peds::MAX_PEDS - 1; id >= 0; id--) if (Peds::peds[id].used) return id + 1;
	return 0;
}

// ---- позиция и вид
static cell AMX_NATIVE_CALL n_SetPedPos(AMX* amx, cell* params)
{
	if (!Ok(params, 4)) return 0;
	Peds::Ped& p = P(params);
	p.x = amx_ctof(params[2]); p.y = amx_ctof(params[3]); p.z = amx_ctof(params[4]);
	p.moving = false;
	BitStream bs; Peds::Head(bs, Peds::RPC_POS, Id(params)); bs.Write(p.x); bs.Write(p.y); bs.Write(p.z);
	Peds::SendStreamed((int)params[1], bs);
	return 1;
}
static cell AMX_NATIVE_CALL n_GetPedPos(AMX* amx, cell* params)
{
	if (!Ok(params, 4)) return 0;
	Peds::Ped& p = P(params);
	SetRef(amx, params[2], amx_ftoc(p.x)); SetRef(amx, params[3], amx_ftoc(p.y)); SetRef(amx, params[4], amx_ftoc(p.z));
	return 1;
}
static cell AMX_NATIVE_CALL n_SetPedAngle(AMX* amx, cell* params)
{
	if (!Ok(params, 2)) return 0;
	P(params).angle = amx_ctof(params[2]);
	BitStream bs; Peds::Head(bs, Peds::RPC_ANGLE, Id(params)); bs.Write(P(params).angle);
	Peds::SendStreamed((int)params[1], bs);
	return 1;
}
static cell AMX_NATIVE_CALL n_GetPedAngle(AMX* amx, cell* params) { float a = Ok(params, 1) ? P(params).angle : 0.0f; return amx_ftoc(a); }
static cell AMX_NATIVE_CALL n_SetPedSkin(AMX* amx, cell* params)
{
	if (!Ok(params, 2)) return 0;
	P(params).model = (int)params[2];
	BitStream bs; Peds::Head(bs, Peds::RPC_SKIN, Id(params)); bs.Write((uint16_t)params[2]);
	Peds::SendStreamed((int)params[1], bs);
	return 1;
}
static cell AMX_NATIVE_CALL n_GetPedSkin(AMX* amx, cell* params) { return Ok(params, 1) ? P(params).model : -1; }
static cell AMX_NATIVE_CALL n_SetPedWeapon(AMX* amx, cell* params)
{
	if (!Ok(params, 2)) return 0;
	P(params).weapon = (int)params[2];
	BitStream bs; Peds::Head(bs, Peds::RPC_WEAPON, Id(params)); bs.Write((uint16_t)params[2]); bs.Write((uint8_t)0);
	Peds::SendStreamed((int)params[1], bs);
	return 1;
}
static cell AMX_NATIVE_CALL n_GetPedWeapon(AMX* amx, cell* params) { return Ok(params, 1) ? P(params).weapon : -1; }
// SetPedName(pedid, const name[], bool:sub = false) - имя над головой (sub - вторая строка)
static cell AMX_NATIVE_CALL n_SetPedName(AMX* amx, cell* params)
{
	if (!Ok(params, 3)) return 0;
	std::string text = Str(amx, params[2], 128);
	(params[3] ? P(params).subName : P(params).name) = text;
	BitStream bs; Peds::Head(bs, Peds::RPC_NAME, Id(params)); Peds::Str32(bs, text); bs.Write(params[3] != 0);
	Peds::SendStreamed((int)params[1], bs);
	return 1;
}
static cell AMX_NATIVE_CALL n_SetPedColor(AMX* amx, cell* params)
{
	if (!Ok(params, 3)) return 0;
	(params[3] ? P(params).subColor : P(params).color) = (uint32_t)params[2];
	BitStream bs; Peds::Head(bs, Peds::RPC_COLOR, Id(params)); bs.Write((uint32_t)params[2]); bs.Write(params[3] != 0);
	Peds::SendStreamed((int)params[1], bs);
	return 1;
}
static cell AMX_NATIVE_CALL n_SetPedCollision(AMX* amx, cell* params)
{
	if (!Ok(params, 2)) return 0;
	P(params).collision = params[2] != 0;
	BitStream bs; Peds::Head(bs, Peds::RPC_COLLISION, Id(params)); bs.Write(params[2] != 0);
	Peds::SendStreamed((int)params[1], bs);
	return 1;
}
// SetPedAnimGroup(pedid, const group[]) - походка (группа анимаций GTA: "fatman", "gang1", "woman"...)
static cell AMX_NATIVE_CALL n_SetPedAnimGroup(AMX* amx, cell* params)
{
	if (!Ok(params, 2)) return 0;
	P(params).animGroup = Str(amx, params[2], 64);
	BitStream bs; Peds::Head(bs, Peds::RPC_ANIM_GROUP, Id(params)); Peds::Str8(bs, P(params).animGroup);
	Peds::SendStreamed((int)params[1], bs);
	return 1;
}
// мир / интерьер: пед виден только игрокам в том же мире и интерьере (пересоздаётся у них)
static cell AMX_NATIVE_CALL n_SetPedVirtualWorld(AMX* amx, cell* params) { if (!Ok(params, 2)) return 0; P(params).world = (int)params[2]; return 1; }
static cell AMX_NATIVE_CALL n_GetPedVirtualWorld(AMX* amx, cell* params) { return Ok(params, 1) ? P(params).world : 0; }
static cell AMX_NATIVE_CALL n_SetPedInterior(AMX* amx, cell* params) { if (!Ok(params, 2)) return 0; P(params).interior = (int)params[2]; return 1; }
static cell AMX_NATIVE_CALL n_GetPedInterior(AMX* amx, cell* params) { return Ok(params, 1) ? P(params).interior : 0; }
// SetPedStreamDistance(pedid, Float:distance) - pedid -1: для всех педов (по умолчанию 150 м)
static cell AMX_NATIVE_CALL n_SetPedStreamDistance(AMX* amx, cell* params)
{
	if (Args(params) < 2) return 0;
	const float d = amx_ctof(params[2]);
	if (params[1] == -1) { Peds::streamDistance = d > 10.0f ? d : 10.0f; return 1; }
	if (!Ok(params, 2)) return 0;
	P(params).streamDistance = d;
	return 1;
}

// ---- здоровье
static void SendHealth(int id, bool armour)
{
	Peds::Ped& p = Peds::peds[id];
	BitStream bs; Peds::Head(bs, armour ? Peds::RPC_ARMOUR : Peds::RPC_HEALTH, (uint16_t)id);
	bs.Write((uint32_t)0);
	bs.Write(armour ? p.armour : p.health);
	bs.Write(armour ? p.maxArmour : p.maxHealth);
	Peds::SendStreamed(id, bs);
}
// SetPedHealth(pedid, Float:health, Float:maxhealth = 100.0) - 0 и меньше: убить (OnPedDeath, killer -1)
static cell AMX_NATIVE_CALL n_SetPedHealth(AMX* amx, cell* params)
{
	if (!Ok(params, 3)) return 0;
	Peds::Ped& p = P(params);
	const float h = amx_ctof(params[2]);
	p.maxHealth = amx_ctof(params[3]) > 0.0f ? amx_ctof(params[3]) : 100.0f;
	if (h <= 0.0f) { Peds::Kill((int)params[1], -1, false, 0); return 1; }
	if (p.dead) return 0; // мёртвого - через RespawnPed
	p.health = h;
	SendHealth((int)params[1], false);
	return 1;
}
static cell AMX_NATIVE_CALL n_GetPedHealth(AMX* amx, cell* params) { float h = Ok(params, 1) ? P(params).health : 0.0f; return amx_ftoc(h); }
static cell AMX_NATIVE_CALL n_SetPedArmour(AMX* amx, cell* params)
{
	if (!Ok(params, 3)) return 0;
	Peds::Ped& p = P(params);
	p.armour = amx_ctof(params[2]) < 0.0f ? 0.0f : amx_ctof(params[2]);
	p.maxArmour = amx_ctof(params[3]) > 0.0f ? amx_ctof(params[3]) : 100.0f;
	SendHealth((int)params[1], true);
	return 1;
}
static cell AMX_NATIVE_CALL n_GetPedArmour(AMX* amx, cell* params) { float a = Ok(params, 1) ? P(params).armour : 0.0f; return amx_ftoc(a); }
static cell AMX_NATIVE_CALL n_SetPedInvulnerable(AMX* amx, cell* params)
{
	if (!Ok(params, 2)) return 0;
	P(params).invulnerable = params[2] != 0;
	BitStream bs; Peds::Head(bs, Peds::RPC_INVULNERABLE, Id(params)); bs.Write(params[2] != 0);
	Peds::SendStreamed((int)params[1], bs);
	return 1;
}
static cell AMX_NATIVE_CALL n_IsPedInvulnerable(AMX* amx, cell* params) { return Ok(params, 1) && P(params).invulnerable ? 1 : 0; }
static cell AMX_NATIVE_CALL n_IsPedDead(AMX* amx, cell* params) { return Ok(params, 1) && P(params).dead ? 1 : 0; }
// RespawnPed(pedid, Float:health = 100.0) - оживить (пересоздаётся у всех на месте)
static cell AMX_NATIVE_CALL n_RespawnPed(AMX* amx, cell* params)
{
	if (!Ok(params, 2)) return 0;
	Peds::Ped& p = P(params);
	p.dead = false;
	p.health = amx_ctof(params[2]) > 0.0f ? amx_ctof(params[2]) : 100.0f;
	p.vehicle = 0; p.seat = -1; p.clientSeat = 0; p.entering = false; p.autoVehicle = false; p.moving = false;
	for (int i = 0; i < MAX_PLAYERS; i++)
		if (p.streamed.test(i)) { Peds::StreamOut((int)params[1], i); Peds::StreamIn((int)params[1], i); }
	return 1;
}

// ---- действия
// PedGoTo(pedid, Float:x, Float:y, Float:z, movetype = PED_MOVE_RUN) - идти в точку (OnPedReachDestination)
static cell AMX_NATIVE_CALL n_PedGoTo(AMX* amx, cell* params)
{
	if (!Ok(params, 5) || P(params).dead) return 0;
	Peds::Ped& p = P(params);
	p.tx = amx_ctof(params[2]); p.ty = amx_ctof(params[3]); p.tz = amx_ctof(params[4]);
	p.moveType = params[5] == 4 || params[5] == 6 || params[5] == 7 ? (int)params[5] : 6;
	p.moving = true;
	p.follow = -1;
	Peds::ClearAttack((int)params[1]);
	Peds::SendMove((int)params[1], -1);
	return 1;
}
static cell AMX_NATIVE_CALL n_PedStop(AMX* amx, cell* params)
{
	if (!Ok(params, 1)) return 0;
	P(params).moving = false;
	P(params).follow = -1;
	Peds::ClearAttack((int)params[1]);
	BitStream bs; Peds::Head(bs, Peds::RPC_STOP, Id(params));
	Peds::SendStreamed((int)params[1], bs);
	return 1;
}
// PedApplyAnimation(pedid, const lib[], const name[], Float:delta = 4.1, loop = 0, lockx = 1, locky = 1, freeze = 0, time = 0)
static cell AMX_NATIVE_CALL n_PedApplyAnimation(AMX* amx, cell* params)
{
	if (!Ok(params, 9)) return 0;
	BitStream bs; Peds::Head(bs, Peds::RPC_ANIM, Id(params));
	Peds::Str32(bs, Str(amx, params[2], 64));
	Peds::Str32(bs, Str(amx, params[3], 64));
	bs.Write(amx_ctof(params[4]));
	bs.Write(params[5] != 0); bs.Write(params[6] != 0); bs.Write(params[7] != 0); bs.Write(params[8] != 0);
	bs.Write((int32_t)params[9]);
	Peds::SendStreamed((int)params[1], bs);
	return 1;
}
static cell AMX_NATIVE_CALL n_PedShootAt(AMX* amx, cell* params)
{
	if (!Ok(params, 4) || P(params).dead) return 0;
	BitStream bs; Peds::Head(bs, Peds::RPC_SHOOT_POS, Id(params));
	bs.Write(amx_ctof(params[2])); bs.Write(amx_ctof(params[3])); bs.Write(amx_ctof(params[4]));
	Peds::SendStreamed((int)params[1], bs);
	return 1;
}
static cell AMX_NATIVE_CALL n_PedShootAtPlayer(AMX* amx, cell* params)
{
	if (!Ok(params, 2) || P(params).dead) return 0;
	BitStream bs; Peds::Head(bs, Peds::RPC_SHOOT_PLAYER, Id(params)); bs.Write((uint16_t)params[2]);
	Peds::SendStreamed((int)params[1], bs);
	return 1;
}
static cell AMX_NATIVE_CALL n_PedShootAtPed(AMX* amx, cell* params)
{
	if (!Ok(params, 2) || P(params).dead || !Peds::Valid((int)params[2])) return 0;
	BitStream bs; Peds::Head(bs, Peds::RPC_SHOOT_PED, Id(params)); bs.Write((uint16_t)params[2]);
	Peds::SendStreamed((int)params[1], bs);
	return 1;
}
// PedAttackPlayer(pedid, playerid, time = -1) / PedAttackPed(pedid, targetpedid, time = -1) - атаковать (оружием в руке)
static cell AMX_NATIVE_CALL n_PedAttackPlayer(AMX* amx, cell* params)
{
	if (!Ok(params, 3) || P(params).dead) return 0;
	if (params[2] < 0 || params[2] >= MAX_PLAYERS || !Platform::IsPlayerConnected((int)params[2])) return 0;
	Peds::Ped& p = P(params);
	p.moving = false;
	p.attackType = 1; p.attackTarget = (int)params[2]; p.attackTime = (int)params[3];
	Peds::SendAttack((int)params[1], -1);
	return 1;
}
static cell AMX_NATIVE_CALL n_PedAttackPed(AMX* amx, cell* params)
{
	if (!Ok(params, 3) || P(params).dead || !Peds::Valid((int)params[2])) return 0;
	Peds::Ped& p = P(params);
	p.moving = false;
	p.attackType = 2; p.attackTarget = (int)params[2]; p.attackTime = (int)params[3];
	Peds::SendAttack((int)params[1], -1);
	return 1;
}
// PedEnterVehicle(pedid, vehicleid, seat = 0, bool:warp = false) - места как в SA-MP: 0 водитель, 1 передний пассажир
// 2-3 задние, -1 - первое свободное пассажирское. Место занято - 0. Без warp садится с анимацией; если машина
// тронулась или время (SetPedEnterTime) вышло - сразу на место. Сел - OnPedEnterVehicle
static cell AMX_NATIVE_CALL n_PedEnterVehicle(AMX* amx, cell* params)
{
	if (!Ok(params, 4) || P(params).dead) return 0;
	const int id = (int)params[1], vehicle = (int)params[2];
	if (!Platform::IsVehicleValid(vehicle) || params[3] > 3) return 0;
	int seat = (int)params[3];
	const bool anySeat = seat < 0;
	if (anySeat) seat = Peds::FreeSeat(vehicle, id);
	if (seat < 0 || Peds::SeatTaken(vehicle, seat, id)) return 0;
	if (P(params).vehicle > 0 && P(params).vehicle != vehicle) Peds::ExitVehicle(id);
	Peds::EnterVehicle(id, vehicle, seat, anySeat, params[4] != 0, false);
	return 1;
}
static cell AMX_NATIVE_CALL n_PedExitVehicle(AMX* amx, cell* params)
{
	if (!Ok(params, 1) || P(params).vehicle <= 0) return 0;
	Peds::Ped& p = P(params);
	if (p.follow >= 0 && Platform::GetPlayerVehicle(p.follow) > 0) p.stayOut = true; // хозяин в машине - не садиться за ним снова, пока не выйдет
	Peds::ExitVehicle((int)params[1]);
	return 1;
}
// SetPedFollowVehicle(pedid, mode) - садиться ли за хозяином (SetPedFollowPlayer) в машину: PED_VEHICLE_NONE
// PED_VEHICLE_ANIM (по умолчанию: с анимацией, если машина рядом), PED_VEHICLE_WARP (сразу на место)
static cell AMX_NATIVE_CALL n_SetPedFollowVehicle(AMX* amx, cell* params)
{
	if (!Ok(params, 2) || params[2] < 0 || params[2] > 2) return 0;
	P(params).vehicleMode = (int)params[2];
	return 1;
}
static cell AMX_NATIVE_CALL n_GetPedFollowVehicle(AMX* amx, cell* params) { return Ok(params, 1) ? P(params).vehicleMode : 0; }
// SetPedEnterTime(pedid, ms) - сколько даётся на посадку с анимацией (1000..30000, по умолчанию 8000)
static cell AMX_NATIVE_CALL n_SetPedEnterTime(AMX* amx, cell* params)
{
	if (!Ok(params, 2)) return 0;
	P(params).enterTime = params[2] < 1000 ? 1000 : (params[2] > 30000 ? 30000 : (int)params[2]);
	return 1;
}
// SetPedAccuracy(pedid, Float:accuracy) - % попаданий по игроку (0..100, по умолчанию 40; 0 - не ранит игроков)
static cell AMX_NATIVE_CALL n_SetPedAccuracy(AMX* amx, cell* params)
{
	if (!Ok(params, 2)) return 0;
	const float a = amx_ctof(params[2]);
	P(params).accuracy = a < 0.0f ? 0.0f : (a > 100.0f ? 100.0f : a);
	return 1;
}
static cell AMX_NATIVE_CALL n_GetPedAccuracy(AMX* amx, cell* params) { float a = Ok(params, 1) ? P(params).accuracy : 0.0f; return amx_ftoc(a); }
// IsPedEnteringVehicle(pedid) - идёт посадка с анимацией (ещё не внутри)
static cell AMX_NATIVE_CALL n_IsPedEnteringVehicle(AMX* amx, cell* params) { return Ok(params, 1) && P(params).entering ? 1 : 0; }
// GetPedVehicle(pedid, &seat = 0) - машина педа или 0
static cell AMX_NATIVE_CALL n_GetPedVehicle(AMX* amx, cell* params)
{
	if (!Ok(params, 2)) return 0;
	SetRef(amx, params[2], P(params).seat);
	return P(params).vehicle;
}
// SetPedChatBubble(pedid, const text[], color, Float:drawdistance, expiretime)
static cell AMX_NATIVE_CALL n_SetPedChatBubble(AMX* amx, cell* params)
{
	if (!Ok(params, 5)) return 0;
	BitStream bs; Peds::Head(bs, Peds::RPC_BUBBLE, Id(params));
	Peds::Str32(bs, Str(amx, params[2], 140)); // у клиента буфер 144 байта
	bs.Write((uint32_t)params[3]); bs.Write(amx_ctof(params[4])); bs.Write((int32_t)params[5]);
	Peds::SendStreamed((int)params[1], bs);
	return 1;
}
// SetPedAttachedObject(pedid, slot, model, bone, Float:fX, Float:fY, Float:fZ, Float:rX, Float:rY, Float:rZ
//                      Float:sX = 1.0, Float:sY = 1.0, Float:sZ = 1.0, color1 = 0, color2 = 0)
static cell AMX_NATIVE_CALL n_SetPedAttachedObject(AMX* amx, cell* params)
{
	if (!Ok(params, 15) || params[2] < 0 || params[2] >= Peds::ATTACH_SLOTS) return 0;
	Peds::Attach& a = P(params).attach[params[2]];
	a.used = true; a.model = (int)params[3]; a.bone = (int)params[4];
	for (int i = 0; i < 3; i++) { a.off[i] = amx_ctof(params[5 + i]); a.rot[i] = amx_ctof(params[8 + i]); a.scale[i] = amx_ctof(params[11 + i]); }
	a.color1 = (uint32_t)params[14]; a.color2 = (uint32_t)params[15];
	Peds::SendAttach((int)params[1], (int)params[2], -1);
	return 1;
}
static cell AMX_NATIVE_CALL n_RemovePedAttachedObject(AMX* amx, cell* params)
{
	if (!Ok(params, 2) || params[2] < 0 || params[2] >= Peds::ATTACH_SLOTS) return 0;
	P(params).attach[params[2]].used = false;
	Peds::SendAttach((int)params[1], (int)params[2], -1);
	return 1;
}

// PedStopAttack(pedid) - прекратить атаку (охранник снова идёт за хозяином, если SetPedFollowPlayer задан)
static cell AMX_NATIVE_CALL n_PedStopAttack(AMX* amx, cell* params)
{
	if (!Ok(params, 1)) return 0;
	Peds::EndAttack((int)params[1]);
	return 1;
}
// GetPedAttackTarget(pedid, &bool:isped) - кого атакует пед, -1 - никого
static cell AMX_NATIVE_CALL n_GetPedAttackTarget(AMX* amx, cell* params)
{
	if (!Ok(params, 2)) return -1;
	const Peds::Ped& p = P(params);
	cell* out = nullptr;
	if (amx_GetAddr(amx, params[2], &out) == AMX_ERR_NONE && out) *out = p.attackType == 2 ? 1 : 0;
	return p.attackType ? p.attackTarget : -1;
}

// ---- охранник и синхронизация
// SetPedFollowPlayer(pedid, playerid, Float:distance = 3.0) - идти за игроком (бегом, если отстал); playerid -1 - перестать
static cell AMX_NATIVE_CALL n_SetPedFollowPlayer(AMX* amx, cell* params)
{
	if (!Ok(params, 3)) return 0;
	Peds::Ped& p = P(params);
	p.follow = params[2] >= 0 && params[2] < MAX_PLAYERS && Platform::IsPlayerConnected((int)params[2]) ? (int)params[2] : -1;
	p.followDistance = amx_ctof(params[3]) > 1.0f ? amx_ctof(params[3]) : 1.0f;
	if (p.follow < 0 && p.moving) { p.moving = false; BitStream bs; Peds::Head(bs, Peds::RPC_STOP, Id(params)); Peds::SendStreamed((int)params[1], bs); }
	return 1;
}
static cell AMX_NATIVE_CALL n_GetPedFollowPlayer(AMX* amx, cell* params) { return Ok(params, 1) ? P(params).follow : -1; }
// GetPedSyncOwner(pedid) - игрок, чей клиент сейчас ведёт педа (ближайший), или -1
static cell AMX_NATIVE_CALL n_GetPedSyncOwner(AMX* amx, cell* params) { return Ok(params, 1) ? P(params).owner : -1; }
static cell AMX_NATIVE_CALL n_IsPedStreamedIn(AMX* amx, cell* params)
{
	return Ok(params, 2) && params[2] >= 0 && params[2] < MAX_PLAYERS && P(params).streamed.test((size_t)params[2]) ? 1 : 0;
}
// Ped_SetSyncRate(ms) - как часто клиенты присылают позиции педов (50..1000, по умолчанию 100)
static cell AMX_NATIVE_CALL n_Ped_SetSyncRate(AMX* amx, cell* params)
{
	if (Args(params) < 1) return 0;
	Peds::syncRate = params[1] < 50 ? 50 : (params[1] > 1000 ? 1000 : (int)params[1]);
	Peds::settingsSent.reset(); // разошлётся заново при следующем стриме
	for (int i = 0; i < MAX_PLAYERS; i++) if (PI[i].pLauncherStat && Platform::IsPlayerConnected(i)) Peds::SendSettings(i);
	return 1;
}

// ---- килфид ----
// Килфид SA-MP берёт имя убийцы из списка игроков клиента по ID - педа туда не передать. Поэтому клиентам на миг
// добавляется "игрок" с нужным именем в свободный слот (RPC 137), уходит сообщение о смерти (RPC 55) и слот
// убирается (RPC 138): килфид копирует имя при добавлении строки, строка остаётся.
static bool FakeKillerDeathMessage(std::string name, uint32_t color, int victim, int reason, int forplayerid)
{
	if (victim < 0 || victim >= MAX_PLAYERS || !Platform::IsPlayerConnected(victim)) return false;
	int fake = -1;
	for (int i = MAX_PLAYERS - 1; i >= 0; i--) if (!Platform::IsPlayerConnected(i)) { fake = i; break; }
	if (fake < 0) return false;
	// {RRGGBB} в имени килфид не понимает
	for (size_t at = name.find('{'); at != std::string::npos; at = name.find('{', at))
		if (at + 7 < name.size() && name[at + 7] == '}') name.erase(at, 8); else at++;
	if (name.empty()) name = "NPC";
	if (name.size() > 24) name.resize(24);
	BitStream join;
	join.Write((uint16_t)fake); join.Write((uint32_t)color); join.Write((uint8_t)0);
	join.Write((uint8_t)name.size()); join.Write(name.c_str(), (int)name.size());
	BitStream col;
	col.Write((uint16_t)fake); col.Write((uint32_t)color);
	BitStream death;
	death.Write((uint16_t)fake); death.Write((uint16_t)victim); death.Write((uint8_t)reason);
	BitStream quit;
	quit.Write((uint16_t)fake); quit.Write((uint8_t)0);
	Native::MBSendRPC(forplayerid, 137, &join, 1, 9);
	Native::MBSendRPC(forplayerid, 72, &col, 1, 9);
	Native::MBSendRPC(forplayerid, 55, &death, 1, 9);
	Native::MBSendRPC(forplayerid, 138, &quit, 1, 9);
	return true;
}
// SendDeathMessageByName(const killer[], killercolor, playerid, reason, forplayerid = -1) - в килфид с любым именем убийцы
static cell AMX_NATIVE_CALL n_SendDeathMessageByName(AMX* amx, cell* params)
{
	if (Args(params) < 5) return 0;
	return FakeKillerDeathMessage(Str(amx, params[1], 64), (uint32_t)params[2], (int)params[3], (int)params[4], (int)params[5]) ? 1 : 0;
}
// SendPedDeathMessage(pedid, playerid, reason, forplayerid = -1) - в килфид: пед (его имя и цвет) убил игрока
static cell AMX_NATIVE_CALL n_SendPedDeathMessage(AMX* amx, cell* params)
{
	if (!Ok(params, 4)) return 0;
	const Peds::Ped& p = P(params);
	return FakeKillerDeathMessage(p.name, p.color, (int)params[2], (int)params[3], (int)params[4]) ? 1 : 0;
}
