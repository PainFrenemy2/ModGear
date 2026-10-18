/*
 * ============================================================================
 *  ModGear_NPC  -  педы
 * ----------------------------------------------------------------------------
 *  ѕеды Arizona (libPED): охранники, NPC, бой и следование.
 *
 *  ѕодключение:   #include <ModGear_NPC>      (сам подключает <ModGear>)
 *  ƒокументаци€:  https://painfrenemy2.github.io/ModGear/
 * ============================================================================
 */
#if defined _ModGear_NPC_included
	#endinput
#endif
#define _ModGear_NPC_included

#include <ModGear>

// ---- ѕеды Arizona (libPED): охранники, NPC ----
// ѕед виден игрокам с лаунчером в том же мире/интерьере ближе SetPedStreamDistance (150 м).
// ¬едЄт педа клиент ближайшего игрока (GetPedSyncOwner), сервер принимает его позиции и поправл€ет
// остальных. ”рон считает сервер: OnPedDamage (return 0 - отменить), смерть - OnPedDeath.
#define PED_MOVE_WALK                  (4)
#define PED_MOVE_RUN                   (6)
#define PED_MOVE_SPRINT                (7)

// SetPedFollowVehicle: садитьс€ ли охраннику за хоз€ином в машину
#define PED_VEHICLE_NONE               (0)
#define PED_VEHICLE_ANIM               (1)     // с анимацией (по умолчанию), тронулс€ - сразу на место
#define PED_VEHICLE_WARP               (2)     // сразу на место

// id педа или -1
native CreatePed(model, Float:x, Float:y, Float:z, Float:angle = 0.0, world = 0, interior = 0);
native DestroyPed(pedid);
native IsValidPed(pedid);
native GetPedPoolSize();                                        // наибольший id + 1 (дл€ циклов)

native SetPedPos(pedid, Float:x, Float:y, Float:z);
native GetPedPos(pedid, &Float:x, &Float:y, &Float:z);
native SetPedAngle(pedid, Float:angle);
native Float:GetPedAngle(pedid);
native SetPedSkin(pedid, skin);
native GetPedSkin(pedid);
native SetPedWeapon(pedid, weaponid);                           // патроны бесконечные
native GetPedWeapon(pedid);
native SetPedName(pedid, const name[], bool:sub = false);       // им€ над головой (sub - втора€ строка)
native SetPedColor(pedid, color, bool:sub = false);
native SetPedCollision(pedid, bool:toggle);
native SetPedAnimGroup(pedid, const group[]);                   // походка: "fatman", "gang1", "woman"...
native SetPedVirtualWorld(pedid, world);
native GetPedVirtualWorld(pedid);
native SetPedInterior(pedid, interior);
native GetPedInterior(pedid);
native SetPedStreamDistance(pedid, Float:distance);             // pedid -1 - дл€ всех

// 0 - убить
native SetPedHealth(pedid, Float:health, Float:maxhealth = 100.0);
native Float:GetPedHealth(pedid);
native SetPedArmour(pedid, Float:armour, Float:maxarmour = 100.0);
native Float:GetPedArmour(pedid);
native SetPedInvulnerable(pedid, bool:toggle);
native IsPedInvulnerable(pedid);
native IsPedDead(pedid);
native RespawnPed(pedid, Float:health = 100.0);

// -> OnPedReachDestination
native PedGoTo(pedid, Float:x, Float:y, Float:z, movetype = PED_MOVE_RUN);
native PedStop(pedid);
native PedApplyAnimation(pedid, const animlib[], const animname[], Float:delta = 4.1, loop = 0, lockx = 1, locky = 1, freeze = 0, time = 0);
native PedShootAt(pedid, Float:x, Float:y, Float:z);
native PedShootAtPlayer(pedid, playerid);
native PedShootAtPed(pedid, targetpedid);
native PedAttackPlayer(pedid, playerid, time = -1);             // атаковать оружием в руке
native PedAttackPed(pedid, targetpedid, time = -1);
native PedStopAttack(pedid);                                    // прекратить атаку (охранник вернЄтс€ к хоз€ину)
native GetPedAttackTarget(pedid, &bool:isped);                  // кого атакует пед, -1 - никого
native SetPedAccuracy(pedid, Float:accuracy);                  // % попаданий по игрокам (0..100, по умолчанию 40; 0 - не ранит)
native Float:GetPedAccuracy(pedid);
native SendPedDeathMessage(pedid, playerid, reason, forplayerid = -1);   // килфид: пед (им€ и цвет педа) убил игрока
native SendDeathMessageByName(const killer[], killercolor, playerid, reason, forplayerid = -1); // килфид с любым именем убийцы
native PedEnterVehicle(pedid, vehicleid, seat = 0, bool:warp = false); // seat как в SA-MP: 0 водитель, 1 передний, 2-3 задние, -1 - любое свободное; зан€то - 0
native PedExitVehicle(pedid);
native IsPedEnteringVehicle(pedid);                             // садитс€ с анимацией (ещЄ не внутри)
native SetPedEnterTime(pedid, ms);                              // врем€ на посадку с анимацией (1000..30000, по умолчанию 8000)
native GetPedVehicle(pedid, &seat = 0);                         // машина или 0
native SetPedNameGradient(pedid, hue1, sat1, hue2, sat2);       // переливающеес€ им€ педа: оттенок 0..359, насыщенность 0..100
native SetPedChatBubble(pedid, const text[], color, Float:drawdistance, expiretime);
native SetPedAttachedObject(pedid, slot, model, bone, Float:fX = 0.0, Float:fY = 0.0, Float:fZ = 0.0, Float:rX = 0.0, Float:rY = 0.0, Float:rZ = 0.0, Float:sX = 1.0, Float:sY = 1.0, Float:sZ = 1.0, color1 = 0, color2 = 0);
native RemovePedAttachedObject(pedid, slot);

// охранник: идти за игроком; -1 - перестать
native SetPedFollowPlayer(pedid, playerid, Float:distance = 3.0);
native GetPedFollowPlayer(pedid);
native SetPedFollowVehicle(pedid, mode);                        // PED_VEHICLE_*: садитьс€ ли за хоз€ином в машину
native GetPedFollowVehicle(pedid);
native GetPedSyncOwner(pedid);                                  // чей клиент ведЄт педа, -1 - никто не видит
native IsPedStreamedIn(pedid, playerid);
native SetPedSyncRate(ms);                                      // как часто клиенты шлют позиции (50..1000, по умолчанию 100)

// return 0 - отменить урон
forward OnPedDamage(pedid, issuerid, bool:issuer_is_ped, Float:amount, weaponid, bodypart);
forward OnPedDeath(pedid, killerid, bool:killer_is_ped, weaponid); // killerid -1 - SetPedHealth(0)
forward OnPlayerDamageByPed(playerid, pedid, Float:amount, weaponid, bodypart);
forward OnPedReachDestination(pedid);
forward OnPedEnterVehicle(pedid, vehicleid, seat);             // сел (с анимацией - когда сел или машина тронулась)
forward OnPedExitVehicle(pedid, vehicleid);
forward OnPedStreamIn(pedid, playerid);
forward OnPedStreamOut(pedid, playerid);
