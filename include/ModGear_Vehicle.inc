/*
 * ============================================================================
 *  ModGear_Vehicle  -  машины
 * ----------------------------------------------------------------------------
 *  Тюнинг и внешний вид, грязь, сцепка, хендлинг.
 *  Всё сохраняется на машине и само отправляется игрокам,
 *  у которых она появляется в зоне стрима.
 *
 *  Подключение:   #include <ModGear_Vehicle>      (сам подключает <ModGear>)
 *  Документация:  https://htmlpreview.github.io/?https://raw.githubusercontent.com/PainFrenemy2/ModGear/HEAD/docs/index.html
 * ============================================================================
 */
#if defined _ModGear_Vehicle_included
	#endinput
#endif
#define _ModGear_Vehicle_included

#include <ModGear>

// ----------------------------------------------------------------------------
//  ДЕТАЛИ ТЮНИНГА
// ----------------------------------------------------------------------------
#define VEHICLE_PART_HOOD              (1)
#define VEHICLE_PART_SPOILER           (2)
#define VEHICLE_PART_EXHAUST           (3)
#define VEHICLE_PART_WHEELS            (4)
#define VEHICLE_PART_FRONT_BUMPER      (5)
#define VEHICLE_PART_REAR_BUMPER       (6)
#define VEHICLE_PART_ROOF              (7)
#define VEHICLE_PART_WING_LEFT         (8)
#define VEHICLE_PART_WING_RIGHT        (9)
#define VEHICLE_PART_HOOD_LEFT         (10)
#define VEHICLE_PART_HOOD_RIGHT        (11)
#define VEHICLE_PART_AUTO              (0)                      // слот по самой детали

// Детали кастомных машин - simple id из arizona/NamedModels.json (bnt_* капот, spl_* спойлер, exh_* выхлоп,
// fbmp_/rbmp_* бамперы, rf_* крыша, wg_l_/wg_r_* крылья). Слот плагин берёт по детали сам, старую деталь
// в этом слоте снимает. Диски (VEHICLE_PART_WHEELS) - модели wheel_* из veh_mods.ide.

native SetVehicleTuningPart(carid, slot, modelid);              // поставить деталь (модель) в слот VEHICLE_PART_*
native GetVehicleTuningPart(carid, slot);                       // модель в слоте или 0
native GetVehicleTuningPartSlot(carid, modelid);                // в каком слоте стоит деталь, 0 - нигде
native RemoveVehicleTuningPart(carid, modelid);

// ----------------------------------------------------------------------------
//  ВНЕШНИЙ ВИД
// ----------------------------------------------------------------------------
native SetVehicleVinyl(carid, vinyl, color);                    // винил 0-17, цвет 0-255
native SetVehicleNeonColor(carid, color);                       // неон, 0 - выкл
native SetVehicleSmokeColor(carid, color);                      // цвет дыма
native SetVehicleHeadlightColor(carid, color);                  // цвет фар
native SetVehicleHeadlightTexture(carid, const texture[]);      // текстура фар по имени, "" - стандартная
native SetVehicleTurnLightColor(carid, color);                  // подсветка поворотников на земле, 0xRRGGBBAA (AA - яркость)
native SetVehiclePlate(carid, type, const number[], const region[]); // номер, type 0-51

#define VEHICLE_TONER_frontal          (0)
#define VEHICLE_TONER_frontal_f        (1)
#define VEHICLE_TONER_frontal_r        (2)
#define VEHICLE_TONER_frontal_d        (3)
#define VEHICLE_TONER_FRONT_LIGHT      (4)
#define VEHICLE_TONER_REAR_LIGHT       (5)
#define VEHICLE_TONER_REAR_GLASS       (6)
native SetVehicleToner(carid, part, red, green, blue, brightness, bool:enable = true); // тонировка части VEHICLE_TONER_*

// ----------------------------------------------------------------------------
//  КОЛЁСА
// ----------------------------------------------------------------------------
#define VEHICLE_WHEELS_FRONT           (0)
#define VEHICLE_WHEELS_REAR            (1)
native SetVehicleWheelCamber(carid, axle, Float:angle, Float:offset); // развал: axle VEHICLE_WHEELS_*, угол -15..15, вынос -0.1..0.1
native SetVehicleWheelSize(carid, Float:size = 0.85);           // размер колёс 0.0-1.05
native SetVehicleWheelColor(carid, color);                      // цвет дисков (только диски лаунчера)
native SetVehicleBrakeCalipers(carid, modelid, bool:simplemodel = false); // тормозные суппорты: modelid - модель, 0 - убрать

// ----------------------------------------------------------------------------
//  ОБОРУДОВАНИЕ
// ----------------------------------------------------------------------------
#define VEHICLE_HYDRAULICS_OFF         (0)
#define VEHICLE_HYDRAULICS_HIGH        (1)
#define VEHICLE_HYDRAULICS_LOW         (2)
native SetVehicleHydraulics(carid, mode);                       // VEHICLE_HYDRAULICS_*
native SetVehicleExhaustShots(carid, bool:enable);              // хлопки из выхлопа
native SetVehicleStrobeLights(carid, mode, Float:speed = 1.0, bool:beam = false); // стробоскопы: mode 0 - выкл, speed - скорость мигания, beam - луч
native SetVehicleSiren(carid, bool:enable);                     // мигалка/сирена
native SetVehicleHornSound(carid, hornid);                      // звук клаксона, -1 - стандартный
native SetVehicleDrift(carid, bool:enable);                     // дрифт-режим машины Arizona
native SetVehicleNitro(carid, bool:enable);                     // нитро-система Arizona на машине
native SetVehicleNitroColor(carid, color);                      // цвет пламени нитро: 0 обычное, 1 красное, 2 оранжевое, 3 розовое
native SetVehicleSilentEngine(carid, bool:enable);              // двигатель без звука (электромобиль)
native GetVehicleTurnSignal(carid);                             // поворотники, включённые водителем: 0 выкл, 1 левый, 2 правый, 3 аварийка
forward OnVehicleTurnSignal(vehicleid, playerid, signal);       // водитель включил поворотники (Q / E)

// ----------------------------------------------------------------------------
//  ГРЯЗЬ И ИЗНОС
// ----------------------------------------------------------------------------
// грязь 0..100, rate - скорость набора при езде (0 - не пачкается), wear - износ 0..17
native SetVehicleDirt(carid, Float:dirt, Float:rate = 1.0, wear = 1);
native Float:GetVehicleDirt(carid);                             // текущая грязь (водитель обновляет её сам), -1.0 - не задавалась

// ----------------------------------------------------------------------------
//  МАШИНА НА МАШИНЕ (ЭВАКУАТОР, АВТОВОЗ), СЛОТЫ 0-9
// ----------------------------------------------------------------------------
native AttachVehicleToVehicle(carid, slot, model, Float:x, Float:y, Float:z, Float:rx = 0.0, Float:ry = 0.0, Float:rz = 0.0, color1 = 1, color2 = 1, Float:health = 1000.0, paintjob = 3, bool:collision = false);
native DetachVehicleFromVehicle(carid, slot);

// ----------------------------------------------------------------------------
//  ХЕНДЛИНГ
// ----------------------------------------------------------------------------
// field - короткое имя ("maxspeed", "acceleration", "inertia", "brake", "traction", "suspension"...), полное из
// handling.cfg ("fMaxVelocity") или id параметра Arizona ("0x0A"). handling.cfg у клиента свой - сервер его не читает.
// один или несколько параметров одним пакетом: SetVehicleHandling(car, "maxspeed", 300.0, "acceleration", 20.0)
native SetVehicleHandling(carid, const field[], Float:value, {Float,_}:...);
native GetVehicleHandling(carid, const field[], &Float:value);  // 0 - параметр не задавали
native CopyVehicleHandling(fromcarid, tocarid);
native ResetVehicleHandling(carid);

// ----------------------------------------------------------------------------
//  ПРОЧИТАТЬ, ЧТО ПОСТАВЛЕНО МАШИНЕ (ДЛЯ РАЗРАБОТЧИКОВ)
// ----------------------------------------------------------------------------
native GetVehicleVinyl(carid, &color);                          // винил 0-17; color - цвет кузова плагина, -1 - не задавали
native GetVehicleNeonColor(carid);                              // -1 / 0 - неона нет
native GetVehicleSmokeColor(carid);                             // 1 - не задавали
native GetVehicleHeadlightColor(carid);                         // 1 - не задавали
native GetVehicleWheelColor(carid);                             // -1 - не задавали
native Float:GetVehicleWheelSize(carid);                        // 0.0 - не задавали
native GetVehicleWheelCamber(carid, axle, &Float:angle, &Float:offset); // 1 - развал задан
native GetVehiclePlate(carid, number[], region[], numbersize = sizeof number, regionsize = sizeof region); // тип номера, 0 - нет
native GetVehicleToner(carid, part, &red, &green, &blue, &brightness); // 1 - тонировка на детали есть
native GetVehicleHydraulics(carid);                             // VEHICLE_HYDRAULICS_*
native GetVehicleStrobeLights(carid, &Float:speed, &bool:beam); // режим, 0 - выкл
native GetVehicleBrakeCalipers(carid, &bool:simplemodel);       // модель, 0 - нет
native IsVehicleExhaustShots(carid);
native IsVehicleSiren(carid);
native IsVehicleDrift(carid);
native IsVehicleNitro(carid);
native GetVehicleNitroColor(carid);                             // -1 - не задавали
native IsVehicleSilentEngine(carid);
native GetVehicleHeadlightTexture(carid, dest[], size = sizeof dest);
native IsVehicleHandlingSet(carid, const field[] = "");         // "" - задан ли хоть один параметр хендлинга
native ResetVehicleTuning(carid);                               // снять всё, что плагин поставил машине
