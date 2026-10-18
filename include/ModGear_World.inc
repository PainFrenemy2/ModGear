/*
 * ============================================================================
 *  ModGear_World  -  мир
 * ----------------------------------------------------------------------------
 *  Объекты, частицы, надписи в небе, зоны и интерьеры.
 *
 *  Подключение:   #include <ModGear_World>      (сам подключает <ModGear>)
 *  Документация:  https://painfrenemy2.github.io/ModGear/
 * ============================================================================
 */
#if defined _ModGear_World_included
	#endinput
#endif
#define _ModGear_World_included

#include <ModGear>

// ----------------------------------------------------------------------------
//  ОБЪЕКТЫ (CREATEOBJECT, ID < 1000)
// ----------------------------------------------------------------------------
native SetObjectDirt(objectid, level, texture = 0);             // грязь 0..15, texture 0 грязь / 1 потёртости
native SetObjectScale(objectid, Float:scale, bool:collision = true); // масштаб объекта

// ----------------------------------------------------------------------------
//  ЧАСТИЦЫ (ЭФФЕКТЫ GTA), PLAYERID -1 - ВСЕМ
// ----------------------------------------------------------------------------
// period -1 - один раз
native CreateParticle(playerid, const name[], Float:x, Float:y, Float:z, interior = 0, period = -1);
native DestroyParticle(playerid, const name[], Float:x, Float:y, Float:z, interior = 0, period = -1); // те же аргументы, что при создании
native DestroyAllParticles(playerid);

// ----------------------------------------------------------------------------
//  НЕБО, ЗОНЫ, ИНТЕРЬЕРЫ
// ----------------------------------------------------------------------------
// надпись в небе "строка1;строка2"
native SetPlayerSkyLogo(playerid, const text[], bool:visible = true, alpha = 255, fadestep = 10, Float:x = 0.0, Float:y = 0.0, Float:z = 0.0, Float:radius = 0.0);
native SetPlayerZoneNamesWorld(playerid, world);                // названия зон как в этом виртуальном мире
native SetInteriorClimbing(playerid, interior, bool:disable);   // запретить карабкаться в интерьере
native SetPlayerWaterLevel(playerid, Float:level, Float:from = -1.0, time = 0); // вода по всей карте на высоте level (0.0 - обычная); from >= 0 и time > 0 - плавный подъём за time мс; playerid -1 - всем
native SetPlayerWorldEffects(playerid, bool:enable);            // эффекты мира (false - отключить); playerid -1 - всем
native SetPortal(portalid, side, Float:x, Float:y, Float:z, Float:dirx, Float:diry, Float:dirz); // сторона портала 1/2 (portalid 0-1003): вход в одну - выход из другой; видят все
native RemovePortal(portalid, side = 0);                        // убрать сторону портала (0 - обе)
native SetPlayerPortals(playerid, bool:enable);                 // порталы работают у игрока; playerid -1 - всем
native LoadPlayerMap(playerid, const file[]);                   // загрузить карту Arizona из data\maps клиента ("streamer_map_mirage.bin"); playerid -1 - всем
native CreateVisualObject(playerid, id, modelid, Float:x, Float:y, Float:z, Float:rx = 0.0, Float:ry = 0.0, Float:rz = 0.0, Float:sx = 1.0, Float:sy = 1.0, Float:sz = 1.0); // Объекты только для отрисовки на клиенте: без коллизии и вне лимита объектов SA-MP. id 0-65535 у каждого игрока свой
native DestroyVisualObject(playerid, id);
native SetVisualObjectPos(playerid, id, Float:x, Float:y, Float:z);
native SetVisualObjectRot(playerid, id, Float:rx, Float:ry, Float:rz);
native SetVisualObjectScale(playerid, id, Float:sx, Float:sy, Float:sz);

// 3D-линии (тросы, лучи, провода). lineid 0-255 у каждого игрока свой, playerid -1 - всем
#define LINE_POINT_WORLD               (0)                      // fx, fy, fz - координаты
#define LINE_POINT_PLAYER              (1)                      // id - игрок, fx..fz - смещение от него
#define LINE_POINT_VEHICLE             (2)
#define LINE_POINT_OBJECT              (3)
#define LINE_POINT_ACTOR               (4)
#define LINE_POINT_PLAYER_BONE         (5)                      // id - игрок, fx - номер кости
native CreateLine(playerid, lineid, fromtype, fromid, Float:fx, Float:fy, Float:fz, totype, toid, Float:tx, Float:ty, Float:tz, color = 0xFFFFFFFF, Float:width = 0.05, color2 = 0);
native DestroyLine(playerid, lineid);
