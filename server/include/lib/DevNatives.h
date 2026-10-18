#pragma once
// Функции для разработчиков: прочитать всё, что плагин хранит о машинах и игроках (чтобы не дублировать это
// в скрипте), и сбросить тюнинг машины одной функцией. Подключается в Plugin.cpp после ArzCoreNatives.h

#include <lib/ArzCoreNatives.h>

namespace DevNatives
{
	inline bool SetRef(AMX* amx, cell param, cell value)
	{
		cell* ptr = nullptr;
		if (amx_GetAddr(amx, param, &ptr) != AMX_ERR_NONE || !ptr) return false;
		*ptr = value;
		return true;
	}

	inline bool SetRefFloat(AMX* amx, cell param, float value) { return SetRef(amx, param, amx_ftoc(value)); }

	inline cell SetStr(AMX* amx, cell param, cell size, const char* text)
	{
		cell* ptr = nullptr;
		if (amx_GetAddr(amx, param, &ptr) != AMX_ERR_NONE || !ptr || size <= 0) return 0;
		amx_SetString(ptr, text ? text : "", 0, 0, (size_t)size);
		return 1;
	}
}

// GetVehicleVinyl(carid, &color) - винил 0-17 (0 - без винила); color - цвет кузова плагина, -1 - цвет не задавали
static cell AMX_NATIVE_CALL n_GetVehicleVinyl(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Car(params[1])) return -1;
	const VehicleInfo& v = vTemp[params[1]];
	DevNatives::SetRef(amx, params[2], v.vColorCustom ? v.vColor : -1);
	return v.vVinil;
}

// GetVehicleNeonColor(carid) - цвет неона, 0 / -1 - нет
static cell AMX_NATIVE_CALL n_GetVehicleNeonColor(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::Car(params[1])) return -1;
	return vTemp[params[1]].vNeon;
}

// GetVehicleSmokeColor(carid) / GetVehicleHeadlightColor(carid) / GetVehicleWheelColor(carid) - 1 / -1 - не задавали
static cell AMX_NATIVE_CALL n_GetVehicleSmokeColor(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::Car(params[1])) return -1;
	return vTemp[params[1]].vColorShift;
}
static cell AMX_NATIVE_CALL n_GetVehicleHeadlightColor(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::Car(params[1])) return -1;
	return vTemp[params[1]].vHeaderlamp;
}
static cell AMX_NATIVE_CALL n_GetVehicleWheelColor(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::Car(params[1])) return -1;
	return vTemp[params[1]].vDiskiColor;
}

// Float:GetVehicleWheelSize(carid) - 0.0 - не задавали
static cell AMX_NATIVE_CALL n_GetVehicleWheelSize(AMX* amx, cell* params)
{
	float size = 0.0f;
	if (ArzNatives::Args(params) < 1 || !ArzNatives::Car(params[1])) return amx_ftoc(size);
	size = vTemp[params[1]].vDiameteWheel;
	return amx_ftoc(size);
}

// GetVehicleWheelCamber(carid, axle, &Float:angle, &Float:offset) - axle 0 перед, 1 зад
static cell AMX_NATIVE_CALL n_GetVehicleWheelCamber(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 4 || !ArzNatives::Car(params[1]) || params[2] < 0 || params[2] > 1) return 0;
	const float* w = params[2] == 0 ? vTemp[params[1]].vWheel_1 : vTemp[params[1]].vWheel_2;
	DevNatives::SetRefFloat(amx, params[3], w[0]);
	DevNatives::SetRefFloat(amx, params[4], w[1]);
	return w[0] != 0.0f || w[1] != 0.0f;
}

// GetVehiclePlate(carid, number[], region[], numbersize = sizeof number, regionsize = sizeof region) - тип номера, 0 - нет
static cell AMX_NATIVE_CALL n_GetVehiclePlate(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 5 || !ArzNatives::Car(params[1])) return 0;
	const VehicleInfo& v = vTemp[params[1]];
	DevNatives::SetStr(amx, params[2], params[4], v.vNumber);
	DevNatives::SetStr(amx, params[3], params[5], v.vRegion);
	return v.vNumberType;
}

// GetVehicleToner(carid, part, &red, &green, &blue, &brightness) - 1 - тонировка на детали есть
static cell AMX_NATIVE_CALL n_GetVehicleToner(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 6 || !ArzNatives::Car(params[1]) || params[2] < 0 || params[2] > 6) return 0;
	const VehicleInfo& v = vTemp[params[1]];
	const int p = (int)params[2];
	DevNatives::SetRef(amx, params[3], v.vToner_red[p]);
	DevNatives::SetRef(amx, params[4], v.vToner_green[p]);
	DevNatives::SetRef(amx, params[5], v.vToner_blue[p]);
	DevNatives::SetRef(amx, params[6], v.vToner_size[p]);
	return v.vToner[p] ? 1 : 0;
}

// GetVehicleHydraulics(carid) - VEHICLE_HYDRAULICS_*
static cell AMX_NATIVE_CALL n_GetVehicleHydraulics(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::Car(params[1])) return 0;
	return vTemp[params[1]].vHidravlic;
}

// GetVehicleStrobeLights(carid, &Float:speed, &bool:beam) - режим стробоскопов, 0 - выкл
static cell AMX_NATIVE_CALL n_GetVehicleStrobeLights(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::Car(params[1])) return 0;
	const VehicleInfo& v = vTemp[params[1]];
	DevNatives::SetRefFloat(amx, params[2], v.vStrobeSpeed);
	DevNatives::SetRef(amx, params[3], v.vStrobeBeam ? 1 : 0);
	return v.vStrobe;
}

// GetVehicleBrakeCalipers(carid, &bool:simplemodel) - модель суппортов, 0 - нет
static cell AMX_NATIVE_CALL n_GetVehicleBrakeCalipers(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Car(params[1])) return 0;
	DevNatives::SetRef(amx, params[2], vTemp[params[1]].vCalipersSimple ? 1 : 0);
	return vTemp[params[1]].vCalipers;
}

// IsVehicleExhaustShots / IsVehicleSiren / IsVehicleDrift / IsVehicleNitro / IsVehicleSilentEngine (carid)
static cell AMX_NATIVE_CALL n_IsVehicleExhaustShots(AMX* amx, cell* params)
{
	return ArzNatives::Args(params) >= 1 && ArzNatives::Car(params[1]) && vTemp[params[1]].vOtstrel != 0;
}
static cell AMX_NATIVE_CALL n_IsVehicleSiren(AMX* amx, cell* params)
{
	return ArzNatives::Args(params) >= 1 && ArzNatives::Car(params[1]) && vTemp[params[1]].vSiren;
}
static cell AMX_NATIVE_CALL n_IsVehicleDrift(AMX* amx, cell* params)
{
	return ArzNatives::Args(params) >= 1 && ArzNatives::Car(params[1]) && vTemp[params[1]].vDrift;
}
static cell AMX_NATIVE_CALL n_IsVehicleNitro(AMX* amx, cell* params)
{
	return ArzNatives::Args(params) >= 1 && ArzNatives::Car(params[1]) && vTemp[params[1]].vNitro;
}
static cell AMX_NATIVE_CALL n_IsVehicleSilentEngine(AMX* amx, cell* params)
{
	return ArzNatives::Args(params) >= 1 && ArzNatives::Car(params[1]) && vTemp[params[1]].vSilentEngine;
}

// GetVehicleNitroColor(carid) - 0..3, -1 - не задавали
static cell AMX_NATIVE_CALL n_GetVehicleNitroColor(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::Car(params[1])) return -1;
	return vTemp[params[1]].vNitroColor;
}

// GetVehicleHeadlightTexture(carid, dest[], size = sizeof dest)
static cell AMX_NATIVE_CALL n_GetVehicleHeadlightTexture(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::Car(params[1])) return 0;
	DevNatives::SetStr(amx, params[2], params[3], vTemp[params[1]].vHeadlightTexture.c_str());
	return (cell)vTemp[params[1]].vHeadlightTexture.size();
}

// GetPlayerLightMode(playerid)
static cell AMX_NATIVE_CALL n_GetPlayerLightMode(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::Player(params[1])) return 0;
	return ArzCore::lightMode[params[1]];
}

// GetPlayerSkinTextureCount(playerid) - сколько текстур скина задано
static cell AMX_NATIVE_CALL n_GetPlayerSkinTextureCount(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::Player(params[1])) return 0;
	return (cell)ArzCore::skinMaterials[params[1]].size();
}

// ResetVehicleTuning(carid) - снять всё, что плагин поставил машине: детали и диски, неон, тонировку, нитро, звук
// двигателя, мигалку, дрифт, стробоскопы, суппорты, гидравлику, хлопки, цвет дыма и фар, текстуру фар, хендлинг
// Цвет кузова и винил плагина у тех, кто уже видит машину, остаются до её перезагрузки (у новых - цвет SA-MP)
static cell AMX_NATIVE_CALL n_ResetVehicleTuning(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || !ArzNatives::Car(params[1])) return 0;
	const int car = (int)params[1];
	VehicleInfo& v = vTemp[car];
	for (int slot = 1; slot < PART_SLOTS; slot++)
		if (slot != PART_WHEELS && v.vParts[slot]) Native::b_RemoveVehicleComponent(car, v.vParts[slot]);
	if (v.vDiski) Native::b_RemoveVehicleComponent(car, v.vDiski);
	if (v.vNeon > 0) { v.vNeon = -1; Native::b_NeonColorChange(car, 0); }
	for (int i = 0; i < 7; i++)
		if (v.vToner[i]) { v.vToner[i] = 0; Native::b_VehicleToneCustomizer(car, i, 0, 0, 0, 0); }
	if (v.vNitro || v.vNitroColor >= 0) { v.vNitro = false; v.vNitroColor = -1; ArzCore::SendVehicleNitro(car); }
	if (v.vSilentEngine) { v.vSilentEngine = false; ArzCore::SendSilentEngine(car); }
	if (!v.vHeadlightTexture.empty()) { v.vHeadlightTexture.clear(); ArzCore::SendHeadlightTexture(car); }
	if (v.vSiren) { v.vSiren = false; Native::b_SetVehicleSiren(car, false); }
	if (v.vDrift) { v.vDrift = false; Native::b_SetVehicleDrift(car, false); }
	if (v.vStrobe) { v.vStrobe = 0; Native::b_SetVehicleStrobeLights(car, 0, 1.0f, false); }
	if (v.vCalipers) { v.vCalipers = 0; Native::b_SetVehicleBrakeCalipers(car, 0, false); }
	if (v.vHidravlic) { v.vHidravlic = 0; Native::b_SetVehicleHidraulics(car, 0); }
	if (v.vOtstrel) { v.vOtstrel = 0; Native::b_SetShotsExhaust(car, 0); }
	if (v.vColorShift != 1) { v.vColorShift = 1; Native::b_SmokeColorShift(car, 0); }
	if (v.vHeaderlamp != 1) { v.vHeaderlamp = 1; Native::b_SetColorHeadlamp(car, 0); }
	v.vHandlingMask = 0;
	v.vColorCustom = false;
	v.vVinil = 0;
	v.vDiskiColor = -1;
	return 1;
}

// IsVehicleHandlingSet(carid, const field[] = "") - задан ли машине хендлинг через плагин: field "" - хоть один
// параметр, иначе этот параметр ("maxspeed", "fMaxVelocity", "0x0A")
static cell AMX_NATIVE_CALL n_IsVehicleHandlingSet(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1 || params[1] <= 0 || params[1] >= MAX_VEHICLES) return 0;
	const unsigned long long mask = vTemp[params[1]].vHandlingMask;
	const std::string field = ArzNatives::Args(params) >= 2 ? ArzNatives::Str(amx, params[2], 48) : "";
	if (field.empty()) return mask != 0;
	const int id = Handling::ArzId(field.c_str());
	return id != -1 && (mask >> id & 1) ? 1 : 0;
}
