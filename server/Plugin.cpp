#include "Plugin.h"
#include "Platform.h"
#include "Version.h"
#include "Media.h"
#include "AzRtc.h"

#include <pawn/amx/amx.h>
#include <pawn/plugincommon.h>

#include <list>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <string>
#include <lib/InfoServer.h>
#include "raknet/urmem.hpp"
#include "lib/FuncNative.h"
#include "lib/ArzExtraNatives.h"
#include "lib/ArzChatNatives.h"
#include "lib/CefNatives.h"
#include "lib/ScreenSize.h"
#include "lib/PedNatives.h"
#include "lib/ArzCoreNatives.h"
#include "lib/DevNatives.h"

#ifndef CDECL
#ifdef _WIN32
#define CDECL __cdecl
#else
#define CDECL
#endif
#endif

extern void* pAMXFunctions;
std::list<AMX*> pAMX;

urmem::hook Hook_amxRegister,
	Hook_amxfCreateVehicle,
	Hook_amxfAddStaticVehicle,
	Hook_amxfAddStaticVehicleEx,
	Hook_amxfGetVehicleModel,
	Hook_amxfDestroy,
	Hook_amxfSetPlayerSkin,
	Hook_amxfSetSpawnInfo;

AMX_NATIVE
	amxf_AddStaticVehicle = nullptr,
	amxf_CreateVehicle = nullptr,
	amxf_AddStaticVehicleEx = nullptr,
	amxf_GetVehicleModel = nullptr,
	amxf_DestroyVehicle = nullptr,
	amxf_SetPlayerSkin = nullptr,
	amxf_SetSpawnInfo = nullptr;

cell AMX_NATIVE_CALL GetVehicleModel_Hook(AMX* amx, cell* params)
{
	Hook_amxfGetVehicleModel.disable();
	cell model = amxf_GetVehicleModel(amx, params);
	Hook_amxfGetVehicleModel.enable();
	if (vTemp[params[1]].vModelLuxe > 611) return vTemp[params[1]].vModelLuxe;
	return model;
}
static bool RessetCars(int carid)
{
	if (carid > 0 && carid < MAX_VEHICLES)
	{
		vTemp[carid].vOtstrel = 0;
		vTemp[carid].vModelLuxe = 0;
		vTemp[carid].vDefaultModel = 0;
		vTemp[carid].vID = 0;
		for (int& part : vTemp[carid].vParts) part = 0;
		vTemp[carid].vNitroColor = -1;
		vTemp[carid].vNitro = false;
		vTemp[carid].vSilentEngine = false;
		vTemp[carid].vTurnSignal = 0;
		vTemp[carid].vHeadlightTexture.clear();
		vTemp[carid].vDiski = 0;
		vTemp[carid].vVinil = 0;
		vTemp[carid].vOtstrel = 0;
		vTemp[carid].vHidravlic = 0;
		vTemp[carid].vNumberType = 0;
		vTemp[carid].vColorShift = 1;
		vTemp[carid].vHeaderlamp = 1;
		vTemp[carid].vDiskiColor = -1; // -1 - цвет дисков не задан
		vTemp[carid].vColor = 1;
		vTemp[carid].vColorCustom = false;
		ArzExtra::ResetVehicle(carid);
		vTemp[carid].vDirt = -1.0f;
		vTemp[carid].vDirtRate = 1.0f;
		vTemp[carid].vWear = 1;
		vTemp[carid].vTurnColor = 0;
		vTemp[carid].vTurnColorSet = false;
		vTemp[carid].vNeon = -1;
		vTemp[carid].vDiameteWheel = 0.0f;
		vTemp[carid].vStrobe = 0;
		vTemp[carid].vStrobeSpeed = 1.0f;
		vTemp[carid].vStrobeBeam = false;
		vTemp[carid].vCalipers = 0;
		vTemp[carid].vCalipersSimple = false;
		vTemp[carid].vDrift = false;
		vTemp[carid].vSiren = false;
		vTemp[carid].vHandlingMask = 0;
		memset(vTemp[carid].vNumber, 0, sizeof(vTemp[carid].vNumber));
		memset(vTemp[carid].vRegion, 0, sizeof(vTemp[carid].vRegion));
		memset(vTemp[carid].vNameCar, 0, sizeof(vTemp[carid].vNameCar));
		memset(vTemp[carid].vNameCar2, 0, sizeof(vTemp[carid].vNameCar2));
		for (int i = 0; i < 7; i++)
		{
			vTemp[carid].vToner[i] = 0;
			vTemp[carid].vToner[i] = 0;
			vTemp[carid].vToner_red[i] = 0;
			vTemp[carid].vToner_green[i] = 0;
			vTemp[carid].vToner_blue[i] = 0;
			vTemp[carid].vToner_size[i] = 0;
			if (i < 2)
			{
				vTemp[carid].vWheel_1[i] = 0.0f;
				vTemp[carid].vWheel_2[i] = 0.0f;
			}
		}
	}
	return true;
}
cell AMX_NATIVE_CALL DestroyVehicle_Hook(AMX* amx, cell* params)
{
	Hook_amxfDestroy.disable();
	int carid = params[1];
	RessetCars(carid);
	cell model = amxf_DestroyVehicle(amx, params);
	Hook_amxfDestroy.enable();
	return model;
}
cell AMX_NATIVE_CALL SetPlayerSkin_Hook(AMX* amx, cell* params)
{
	Hook_amxfSetPlayerSkin.disable();
	PI[params[1]].pSkin = params[2];
	cell status = amxf_SetPlayerSkin(amx, params);
	Hook_amxfSetPlayerSkin.enable();
	return status;
}
cell AMX_NATIVE_CALL SetSpawnInfo_Hook(AMX* amx, cell* params)
{
	Hook_amxfSetSpawnInfo.disable();
	PI[params[1]].pSkin = params[3];
	cell status = amxf_SetSpawnInfo(amx, params);
	Hook_amxfSetSpawnInfo.enable();
	return status;
}
int Native::CallingBackVehicle(int veh)
{
	if (veh == 0) return 0;
	int amx_idx;
	cell ret = 0;
	for (std::list<AMX*>::iterator a = pAMX.begin(); a != pAMX.end(); ++a)
	{
		if (amx_FindPublic(*a, "GetCustomVehicle", &amx_idx) == AMX_ERR_NONE)
		{
			amx_Push(*a, veh);
			amx_Exec(*a, &ret, amx_idx);
			if (ret == -1) return 0;
			return ret;
		}
	}
	return veh;
}
cell HookCreateVehicle(urmem::hook hook, AMX_NATIVE nativka, AMX* amx, cell* params)
{
	hook.disable();
	int model = 0, modeldefault = 0;
	if (params[1] > 611)
	{
		int uidveh = Native::CallingBackVehicle(params[1]);
		if (uidveh)
		{
			model = params[1];
			modeldefault = params[1] = uidveh;
			if (modeldefault > 611)
			{
				modeldefault = params[1] = 490;
				Platform::Log(Platform::LogLevel::Warning, "[ModGear]: Значение defaul_model больше 611 (установлено: 490) для модели: %i", model);
			}
		}
	}
	if (params[1] > 611)
	{
		model = params[1];
		modeldefault = params[1] = 490;
		Platform::Log(Platform::LogLevel::Warning, "[ModGear]: Модель %i не найдена в public GetCustomVehicle", model);
	}
	cell carid = nativka(amx, params);
	if (carid < 2000)
	{
		RessetCars(carid);
		vTemp[carid].vID = carid;
		vTemp[carid].vModelLuxe = (model != 0) ? model : params[1];
		vTemp[carid].vDefaultModel = (modeldefault != 0) ? modeldefault : params[1];
	}
	hook.enable();
	return carid;
}
cell AMX_NATIVE_CALL CreateVehicle_Hook(AMX* amx, cell* params) { return HookCreateVehicle(Hook_amxfCreateVehicle, amxf_CreateVehicle, amx, params); }
cell AMX_NATIVE_CALL AddStaticVehicleEx_Hook(AMX* amx, cell* params) { return HookCreateVehicle(Hook_amxfAddStaticVehicleEx, amxf_AddStaticVehicleEx, amx, params); }
cell AMX_NATIVE_CALL AddStaticVehicle_Hook(AMX* amx, cell* params) { return HookCreateVehicle(Hook_amxfAddStaticVehicle, amxf_AddStaticVehicle, amx, params); }
// перехват нативов сервера (машины с кастомными моделями, скины): ставим хук на функцию натива один раз
static void InterceptNative(const char* name, AMX_NATIVE func)
{
	if (!name || !func) return;
	if (!std::strcmp(name, "CreateVehicle") && amxf_CreateVehicle == nullptr)
	{
		amxf_CreateVehicle = func;
		Hook_amxfCreateVehicle.install((urmem::address_t)amxf_CreateVehicle, (urmem::address_t)CreateVehicle_Hook);
	}
	else if (!std::strcmp(name, "AddStaticVehicle") && amxf_AddStaticVehicle == nullptr)
	{
		amxf_AddStaticVehicle = func;
		Hook_amxfAddStaticVehicle.install((urmem::address_t)amxf_AddStaticVehicle, (urmem::address_t)AddStaticVehicle_Hook);
	}
	else if (!std::strcmp(name, "AddStaticVehicleEx") && amxf_AddStaticVehicleEx == nullptr)
	{
		amxf_AddStaticVehicleEx = func;
		Hook_amxfAddStaticVehicleEx.install((urmem::address_t)amxf_AddStaticVehicleEx, (urmem::address_t)AddStaticVehicleEx_Hook);
	}
	else if (!std::strcmp(name, "GetVehicleModel") && amxf_GetVehicleModel == nullptr)
	{
		amxf_GetVehicleModel = func;
		Hook_amxfGetVehicleModel.install((urmem::address_t)amxf_GetVehicleModel, (urmem::address_t)GetVehicleModel_Hook);
	}
	else if (!std::strcmp(name, "DestroyVehicle") && amxf_DestroyVehicle == nullptr)
	{
		amxf_DestroyVehicle = func;
		Hook_amxfDestroy.install((urmem::address_t)amxf_DestroyVehicle, (urmem::address_t)DestroyVehicle_Hook);
	}
	else if (!std::strcmp(name, "SetPlayerSkin") && amxf_SetPlayerSkin == nullptr)
	{
		amxf_SetPlayerSkin = func;
		Hook_amxfSetPlayerSkin.install((urmem::address_t)amxf_SetPlayerSkin, (urmem::address_t)SetPlayerSkin_Hook);
	}
	else if (!std::strcmp(name, "SetSpawnInfo") && amxf_SetSpawnInfo == nullptr)
	{
		amxf_SetSpawnInfo = func;
		Hook_amxfSetSpawnInfo.install((urmem::address_t)amxf_SetSpawnInfo, (urmem::address_t)SetSpawnInfo_Hook);
	}
}
int CDECL amx_RegisterHook(AMX* amx, AMX_NATIVE_INFO* nativelist, int number)
{
	Hook_amxRegister.disable();
	for (int i = 0; i < number || (number == -1 && nativelist->func); i++)
	{
		if (nativelist[i].func == nullptr) break;
		InterceptNative(nativelist[i].name, (AMX_NATIVE)nativelist[i].func);
	}
	int retval = reinterpret_cast<int(*)(AMX*, AMX_NATIVE_INFO*, int)>(reinterpret_cast<void**>(pAMXFunctions)[PLUGIN_AMX_EXPORT_Register])(amx, nativelist, number);
	Hook_amxRegister.enable();
	return retval;
}
// public GetCustomWeapon(weaponid) -> базовое оружие для игрока без лаунчера (как GetCustomSkin у скинов)
// -1 или нет public - кастомное (47+) заменяется кулаком: у обычного клиента такого оружия нет
int Native::CallingBackWeapon(int weapon)
{
	if (weapon <= 46) return weapon;
	int amx_idx;
	cell ret = 0;
	for (std::list<AMX*>::iterator a = pAMX.begin(); a != pAMX.end(); ++a)
	{
		if (amx_FindPublic(*a, "GetCustomWeapon", &amx_idx) == AMX_ERR_NONE)
		{
			amx_Push(*a, weapon);
			amx_Exec(*a, &ret, amx_idx);
			return ret >= 0 && ret <= 46 ? (int)ret : 0;
		}
	}
	return 0;
}
int Native::CallingBackIcon(int icon)
{
	if (icon <= 0) return 0;
	int amx_idx;
	cell ret = 0;
	for (std::list<AMX*>::iterator a = pAMX.begin(); a != pAMX.end(); ++a)
	{
		if (amx_FindPublic(*a, "GetCustomIcon", &amx_idx) == AMX_ERR_NONE)
		{
			amx_Push(*a, icon);
			amx_Exec(*a, &ret, amx_idx);
			if (ret == -1) return icon;
			return ret;
		}
	}
	return icon;
}
int Native::CallingBackSkin(int skin)
{
	if (skin == 0) return 0;
	int amx_idx;
	cell ret = 0;
	for (std::list<AMX*>::iterator a = pAMX.begin(); a != pAMX.end(); ++a)
	{
		if (amx_FindPublic(*a, "GetCustomSkin", &amx_idx) == AMX_ERR_NONE)
		{
			amx_Push(*a, skin);
			amx_Exec(*a, &ret, amx_idx);
			if (ret == -1) return 74;
			return ret;
		}
	}
	return skin;
}
int Native::OnPlayerEditAttachedObjectEx(int playerid, int response, int index, int modelid, int boneid, float fOffsetX, float fOffsetY, float fOffsetZ, float fRotX, float fRotY, float fRotZ, float fScaleX, float fScaleY, float fScaleZ)
{
	int amx_idx;
	cell ret = 0;
	cell offset_x = amx_ftoc(fOffsetX);
	cell offset_y = amx_ftoc(fOffsetY);
	cell offset_z = amx_ftoc(fOffsetZ);
	cell rot_x = amx_ftoc(fRotX);
	cell rot_y = amx_ftoc(fRotY);
	cell rot_z = amx_ftoc(fRotZ);
	cell scale_x = amx_ftoc(fScaleX);
	cell scale_y = amx_ftoc(fScaleY);
	cell scale_z = amx_ftoc(fScaleZ);

	for (std::list<AMX*>::iterator a = pAMX.begin(); a != pAMX.end(); ++a)
	{
		if (amx_FindPublic(*a, "OnPlayerEditAttachedObject", &amx_idx) == AMX_ERR_NONE)
		{
			amx_Push(*a, scale_z);
			amx_Push(*a, scale_y);
			amx_Push(*a, scale_x);
			amx_Push(*a, rot_z);
			amx_Push(*a, rot_y);
			amx_Push(*a, rot_x);
			amx_Push(*a, offset_z);
			amx_Push(*a, offset_y);
			amx_Push(*a, offset_x);
			amx_Push(*a, boneid);
			amx_Push(*a, modelid);
			amx_Push(*a, index);
			amx_Push(*a, response);
			amx_Push(*a, playerid);
			amx_Exec(*a, &ret, amx_idx);
			return 1;
		}
	}
	return 0;
}
bool CheckVersion()
{
	cell amxAddr = 0;
	int includeFileVersion = 0;
	for (std::list<AMX*>::iterator a = pAMX.begin(); a != pAMX.end(); ++a)
	{
		if (!amx_FindPubVar(*a, "ModGear_IncludeFileVersion", &amxAddr))
		{
			cell* amxPhysAddr = NULL;
			if (!amx_GetAddr(*a, amxAddr, &amxPhysAddr)) includeFileVersion = static_cast<int>(*amxPhysAddr);
			break;
		}
	}
	if (includeFileVersion == 0) Platform::Log(Platform::LogLevel::Warning, "[ModGear]: Неизвестная версия включаемого файла.");
	else if (includeFileVersion < MODGEAR_INCLUDE_VERSION) Platform::Log(Platform::LogLevel::Warning, "[ModGear]: Обновите включаемый файл для скрипта. Текущая версия: 0x%03x, Последняя версия: 0x%03x", MODGEAR_INCLUDE_VERSION, includeFileVersion);
	else if (includeFileVersion > MODGEAR_INCLUDE_VERSION) Platform::Log(Platform::LogLevel::Warning, "[ModGear]: Обновите плагин до последней версии. Текущая версия: 0x%03x, Последняя версия: 0x%03x", MODGEAR_INCLUDE_VERSION, includeFileVersion);
	return true;
}
static AMX_NATIVE_INFO plugin_natives[] =
{
	{ "GetPlayerLauncherStatus", n_GetPlayerLauncherStatus },
	{ "SetPlayerLauncherStatus", n_SetPlayerLauncherStatus },
	{ "KeysToString", n_ConvertedIntString },
	{ "StringToKeys", n_ConvertedStringInt },
#if MODGEAR_FULL
	{ "SetPlayerInterface", n_InterfaceCEF },
	{ "SetServerHudInfo", n_UpdateInfoHudGtaSa },
	{ "ShowPlayerPhoneVideo", n_ShowYouTubeVideoPhone },
	{ "HidePlayerPhoneVideo", n_HideYouTubeVideoPhone },
	{ "SetVehicleVinyl", n_SetVinylCustomization },
	{ "RemoveVehicleTuningPart", n_RemoveAttachComponent },
	{ "SetVehicleTuningPart", n_InstallVehicleComponent },
	{ "GetVehicleTuningPart", n_GetVehicleInComponentSlot },
	{ "GetVehicleTuningPartSlot", n_GetVehicleInSlotComponent },
	{ "SetVehicleWheelCamber", n_CustomizeWheelSettings },
	{ "SetVehicleWheelSize", n_CustomizeCarWheel },
	{ "SetVehicleNeonColor", n_NeonColorChange },
	{ "SetVehicleSmokeColor", n_SmokeColorShift },
	{ "SetVehicleToner", n_VehicleToneCustomizer },
	{ "SetVehicleHeadlightColor", n_SetColorHeadlamp },
	{ "SetVehiclePlate", n_InstallNumberVehicle },
	{ "SetPlayerCollision", n_SetPlayerCollisionStatus },
	{ "SetVehicleWheelColor", n_SetVehicleColorWheel },
	{ "SetPlayerHotKeyHint", n_UpdateInfoHotKey },
	{ "SetVehicleExhaustShots", n_SetShotsExhaust },
	{ "SetPlayerAttachedObjectEx", n_SetPlayerAttachedObjectEx },
	{ "IsPlayerAttachedObjectSlotEx", n_IsPlayerAttachedObjectSlotEx },
	{ "RemovePlayerAttachedObjectEx", n_RemovePlayerAttachedObjectEx },
	{ "SetPlayerChatTab", n_ToggleChatMod },
	{ "SetVehicleHydraulics", n_SetVehicleHidraulics },
	{ "SetVehicleHandling", n_SetVehicleHandling },
	{ "GetVehicleHandling", n_GetVehicleHandling },
	{ "CopyVehicleHandling", n_CopyVehicleHandling },
	{ "ResetVehicleHandling", n_ResetVehicleHandling },
	{ "Video_Create", n_Video_Create },
	{ "Video_Play", n_Video_Play },
	{ "Video_Stop", n_Video_Stop },
	{ "Video_GetTime", n_Video_GetTime },
	{ "Video_Destroy", n_Video_Destroy },
	{ "Video_GetDuration", n_Video_GetDuration },
	{ "Video_IsPlaying", n_Video_IsPlaying },
	{ "Video_SetTime", n_Video_SetTime },
	{ "Audio_GetDuration", n_Audio_GetDuration },
	{ "Media_GetTitle", n_Media_GetTitle },
	{ "Audio_GetTitle", n_Audio_GetTitle },
	{ "Video_GetTitle", n_Video_GetTitle },
	{ "Media_Init", n_Media_Init },
	{ "Webcam_Init", n_Webcam_Init },
	{ "Webcam_Start", n_Webcam_Start },
	{ "Webcam_Stop", n_Webcam_Stop },
	{ "Webcam_GetState", n_Webcam_GetState },
	{ "Webcam_IsLinked", n_Webcam_IsLinked },
	{ "Voice_Create", n_Voice_Create },
	{ "Voice_AddPlayer", n_Voice_AddPlayer },
	{ "Voice_RemovePlayer", n_Voice_RemovePlayer },
	{ "Voice_SetRadius", n_Voice_SetRadius },
	{ "Voice_SetName", n_Voice_SetName },
	{ "Voice_Destroy", n_Voice_Destroy },
	{ "Voice_IsPlayerInChannel", n_Voice_IsPlayerInChannel },
	{ "Voice_IsPlayerInZone", n_Voice_IsPlayerInZone },
	{ "Voice_Mute", n_Voice_Mute },
	{ "Voice_IsMuted", n_Voice_IsMuted },
	{ "Voice_SetFormat", n_Voice_SetFormat },
	{ "Voice_SetEcho", n_Voice_SetEcho },
	{ "Audio_AttachStreamToVehicle", n_Audio_AttachStreamToVehicle },
	{ "Audio_AttachStreamToPlayer", n_Audio_AttachStreamToPlayer },
	{ "Audio_CreateStreamAtPos", n_Audio_CreateStreamAtPos },
	{ "Audio_SetStreamUrl", n_Audio_UpdateAudioStreamID },
	{ "Audio_GetVehicleStreamID", n_Audio_GetVehicleStreamID },
	{ "Audio_GetPlayerStreamID", n_Audio_GetPlayerStreamID },
	{ "Audio_GetStreamTime", n_Audio_GetStreamTime },
	{ "Audio_SetStreamTime", n_Audio_SetStreamTime },
	{ "Audio_DestroyStream", n_Audio_DestroyStream },
	{ "Audio_IsStreamPlaying", n_Audio_IsStreamPlaying },
	{ "Audio_StopStream", n_Audio_StopStream },
	{ "Audio_PlayStream", n_Audio_PlayStream },
	{ "Audio_CreatePlayerBackground", n_Audio_CreatePlayerBackground },
	{ "Audio_PlayForPlayer", n_Audio_PlayForPlayer },
	{ "Audio_StopForPlayer", n_Audio_StopForPlayer },
	{ "Audio_GetPlayerBackgroundID", n_Audio_GetPlayerBackgroundID },
	{ "Audio_IsValidStream", n_Audio_IsValidStream },
	{ "Audio_GetStreamType", n_Audio_GetStreamType },
	{ "Audio_GetStreamOwner", n_Audio_GetStreamOwner },
	{ "Audio_IsStreamLooped", n_Audio_IsStreamLooped },
	{ "Audio_SetStreamLoop", n_Audio_SetStreamLoop },
	{ "Audio_SetStreamRadioEffect", n_Audio_SetStreamRadioEffect },
	{ "SetPlayerCefCursor", n_ShowSelectCef },
	{ "SendPlayerCefEvent", n_ShowCef },
	{ "SendPlayerCefEventMobile", n_ShowCefMobile },
	{ "EditAttachedObjectEx", n_EditAttachedObjectEx },
	{ "SetPlayerNameTagIcon", n_ShowPlayerTag },
	{ "SetVehicleStrobeLights", n_SetVehicleStrobeLights },
	{ "SetVehicleBrakeCalipers", n_SetVehicleBrakeCalipers },
	{ "SetVehicleDrift", n_SetVehicleDrift },
	{ "SetVehicleSiren", n_SetVehicleSiren },
	{ "SetPlayerSpeedLimit", n_SetPlayerSpeedLimit },
	{ "SetPlayerGreenZone", n_SetPlayerGreenZone },
	{ "SetPlayerTargetMode", n_SetPlayerTargetMode },
	{ "SetVehicleDirt", n_SetVehicleDirt },
	{ "GetVehicleDirt", n_GetVehicleDirt },
	{ "SetVehicleTurnLightColor", n_SetVehicleTurnLightColor },
	{ "SetObjectDirt", n_SetObjectDirty },
	{ "SetPlayerAttachedObjectDirt", n_SetPlayerAttachedObjectDirty },
	{ "CreateParticle", n_CreateParticle },
	{ "DestroyParticle", n_DestroyParticle },
	{ "DestroyAllParticles", n_DestroyAllParticles },
	{ "SetPlayerWeaponWhitelist", n_SetPlayerWeaponWhitelist },
	{ "SetPlayerChatMode", n_SetPlayerChatMode },
	{ "LockPlayerChatMode", n_LockPlayerChatMode },
	{ "AddAdminReport", n_AddAdminReport },
	{ "RemoveAdminReport", n_RemoveAdminReport },
	{ "SetPlayerZoneNamesWorld", n_SetPlayerZoneWorld },
	{ "SetPlayerDialogText", n_UpdatePlayerDialogText },
	{ "SetPlayerDialogRemember", n_SetPlayerDialogRemember },
	{ "SetPlayerDialogStyle", n_SetPlayerDialogStyle },
	{ "SetPlayerChatTabFormat", n_SetChatTabFormat },
	{ "SetPlayerAttachedObjectMaterial", n_SetPlayerAcsMaterial },
	{ "SetPlayerAttachedObjectMatText", n_SetPlayerAcsMaterialText },
	{ "SetPlayerBulletTracer", n_SetPlayerBulletTracer },
	{ "ResetPlayerBulletTracer", n_ResetPlayerBulletTracer },
	{ "SetPlayerWeaponTracer", n_SetPlayerWeaponTracer },
	{ "SetPlayerMouseTracking", n_SetPlayerMouseTracking },
	{ "MoveTextDrawForPlayer", n_MoveTextDrawForPlayer },
	{ "SetPlayerWaypoint", n_SetPlayerWaypoint },
	{ "RemovePlayerWaypoint", n_RemovePlayerWaypoint },
	{ "SetPlayerDiscordStatus", n_SetPlayerDiscordStatus },
	{ "SetPlayerCommandHelper", n_SetPlayerCommandHelper },
	{ "SetInteriorClimbing", n_SetInteriorClimbing },
	{ "SetPlayerJetpackVisibility", n_SetPlayerJetpackVisibility },
	{ "SetObjectScale", n_SetObjectScale },
	{ "AttachVehicleToVehicle", n_AttachVehicleToVehicle },
	{ "DetachVehicleFromVehicle", n_DetachVehicleFromVehicle },
	{ "SetPlayerSkyLogo", n_SetPlayerSkyLogo },
	{ "SetVehicleHornSound", n_SetVehicleHornSound },
	{ "SetPlayerNavigation", n_SetPlayerNavigation },
	{ "SetPlayerAntiPause", n_SetPlayerAntiPause },
	{ "SetPlayerBoneScale", n_SetPlayerBoneScale },
	{ "SetPlayerAirBrake", n_SetPlayerAirBrake },
#if !defined(MODGEAR_PLATFORM_OMP) // в open.mp свой RemovePlayerWeapon
	{ "RemovePlayerWeapon", n_RemovePlayerWeapon },
#endif
	{ "SetPlayerWalkStyle", n_SetPlayerWalkStyle },
	{ "GetPlayerWalkStyle", n_GetPlayerWalkStyle },
	{ "TeleportPlayerWithFade", n_TeleportPlayerWithFade },
	{ "FadePlayerScreen", n_FadePlayerScreen },
	{ "SetPlayerWaterLevel", n_SetPlayerWaterLevel },
	{ "SetPlayerCompass", n_SetPlayerCompass },
	{ "SetPlayerFirstPerson", n_SetPlayerFirstPerson },
	{ "SetPlayerFlightLimits", n_SetPlayerFlightLimits },
	{ "SetPlayerBikeFall", n_SetPlayerBikeFall },
	{ "SetPlayerWorldEffects", n_SetPlayerWorldEffects },
	{ "RedirectPlayer", n_RedirectPlayer },
	{ "ShowPlayerLoadScreen", n_ShowPlayerLoadScreen },
	{ "SetPlayerFlyingCar", n_SetPlayerFlyingCar },
	{ "SetPlayerLaunchControl", n_SetPlayerLaunchControl },
	{ "SetPlayerRadarRoute", n_SetPlayerRadarRoute },
	{ "SetVehicleNitro", n_SetVehicleNitro },
	{ "SetVehicleNitroColor", n_SetVehicleNitroColor },
	{ "SetPortal", n_SetPortal },
	{ "RemovePortal", n_RemovePortal },
	{ "SetPlayerPortals", n_SetPlayerPortals },
	{ "SetPlayerScreenNoise", n_SetPlayerScreenNoise },
	{ "LoadPlayerMap", n_LoadPlayerMap },
	{ "CreateVisualObject", n_CreateVisualObject },
	{ "DestroyVisualObject", n_DestroyVisualObject },
	{ "SetVisualObjectPos", n_SetVisualObjectPos },
	{ "SetVisualObjectRot", n_SetVisualObjectRot },
	{ "SetVisualObjectScale", n_SetVisualObjectScale },
	{ "SetPlayerDriveOnWater", n_SetPlayerDriveOnWater },
	{ "SetPlayerInfiniteStamina", n_SetPlayerInfiniteStamina },
	{ "TogglePlayerScoreboard", n_TogglePlayerScoreboard },
	{ "SetPlayerCameraFollowVehicle", n_SetPlayerCameraFollowVehicle },
	{ "GivePlayerCustomWeapon", n_GivePlayerCustomWeapon },
	{ "CreatePlayerWorldIcon", n_CreatePlayerWorldIcon },
	{ "DestroyPlayerWorldIcon", n_DestroyPlayerWorldIcon },
	{ "DestroyAllPlayerWorldIcons", n_DestroyAllPlayerWorldIcons },
	{ "ResetPlayerScreenFade", n_ResetPlayerScreenFade },
	{ "SetPlayerAnimSpeed", n_SetPlayerAnimSpeed },
	{ "SetPlayerTargetMarked", n_SetPlayerTargetMarked },
	{ "ShowPlayerTestDriveHud", n_ShowPlayerTestDriveHud },
	{ "HidePlayerTestDriveHud", n_HidePlayerTestDriveHud },
	{ "GetPlayerCustomWeapon", n_GetPlayerCustomWeapon },
	{ "SetPlayerWeaponClip", n_SetPlayerWeaponClip },
	{ "SetPlayerWeaponAccuracy", n_SetPlayerWeaponAccuracy },
	{ "ResetPlayerWeaponStats", n_ResetPlayerWeaponStats },
	{ "SetPlayerRadarIconsStyle", n_SetPlayerRadarIconsStyle },
	{ "SetPlayerBreath", n_SetPlayerBreath },
	{ "SetPlayerBikeCruise", n_SetPlayerBikeCruise },
	{ "SetPlayerFirstPersonAim", n_SetPlayerFirstPersonAim },
	{ "SetVehicleSilentEngine", n_SetVehicleSilentEngine },
	{ "GetVehicleTurnSignal", n_GetVehicleTurnSignal },
	{ "RemovePlayerSkinTexture", n_RemovePlayerSkinTexture },
	{ "SetPlayerCompassTarget", n_SetPlayerCompassTarget },
	{ "SetVehicleHeadlightTexture", n_SetVehicleHeadlightTexture },
	{ "SetPlayerCanSwim", n_SetPlayerCanSwim },
	{ "SetPlayerLightMode", n_SetPlayerLightMode },
	{ "CreateLine", n_CreateLine },
	{ "DestroyLine", n_DestroyLine },
	{ "SetPlayerHeadMove", n_SetPlayerHeadMove },
	{ "SetPlayerCJAnims", n_SetPlayerCJAnims },
	{ "GetVehicleVinyl", n_GetVehicleVinyl },
	{ "GetVehicleNeonColor", n_GetVehicleNeonColor },
	{ "GetVehicleSmokeColor", n_GetVehicleSmokeColor },
	{ "GetVehicleHeadlightColor", n_GetVehicleHeadlightColor },
	{ "GetVehicleWheelColor", n_GetVehicleWheelColor },
	{ "GetVehicleWheelSize", n_GetVehicleWheelSize },
	{ "GetVehicleWheelCamber", n_GetVehicleWheelCamber },
	{ "GetVehiclePlate", n_GetVehiclePlate },
	{ "GetVehicleToner", n_GetVehicleToner },
	{ "GetVehicleHydraulics", n_GetVehicleHydraulics },
	{ "GetVehicleStrobeLights", n_GetVehicleStrobeLights },
	{ "GetVehicleBrakeCalipers", n_GetVehicleBrakeCalipers },
	{ "IsVehicleExhaustShots", n_IsVehicleExhaustShots },
	{ "IsVehicleSiren", n_IsVehicleSiren },
	{ "IsVehicleDrift", n_IsVehicleDrift },
	{ "IsVehicleNitro", n_IsVehicleNitro },
	{ "IsVehicleSilentEngine", n_IsVehicleSilentEngine },
	{ "GetVehicleNitroColor", n_GetVehicleNitroColor },
	{ "GetVehicleHeadlightTexture", n_GetVehicleHeadlightTexture },
	{ "GetPlayerLightMode", n_GetPlayerLightMode },
	{ "GetPlayerSkinTextureCount", n_GetPlayerSkinTextureCount },
	{ "ResetVehicleTuning", n_ResetVehicleTuning },
	{ "IsVehicleHandlingSet", n_IsVehicleHandlingSet },
	{ "SetPlayerSkinTexture", n_SetPlayerSkinTexture },
	{ "Cef_GetArgCount", n_Cef_GetArgCount },
	{ "Cef_GetArg", n_Cef_GetArg },
	{ "Cef_GetArgInt", n_Cef_GetArgInt },
	{ "Cef_GetArgFloat", n_Cef_GetArgFloat },
	{ "Cef_GetArgByName", n_Cef_GetArgByName },
	{ "Cef_GetArgIntByName", n_Cef_GetArgIntByName },
	{ "Cef_GetArgFloatByName", n_Cef_GetArgFloatByName },
	{ "Cef_GetRaw", n_Cef_GetRaw },
	{ "RequestPlayerScreenSize", n_RequestPlayerScreenSize },
	{ "GetPlayerScreenSize", n_GetPlayerScreenSize },
	{ "ResetPlayerSkinTextures", n_ResetPlayerSkinTextures },
	{ "CreatePlayerChatTab", n_CreatePlayerChatTab },
	{ "RemovePlayerChatTab", n_RemovePlayerChatTab },
	{ "SendClientMessageToTab", n_SendClientMessageToTab },
	{ "SetNameTagStyle", n_SetNameTagStyle },
	{ "SetChatBubbleNoDistance", n_SetChatBubbleNoDistance },
	{ "SetPlayerNameTagTitle", n_SetPlayerNameTagTitle },
	{ "SetPlayerNameTagStatus", n_SetPlayerNameTagStatus },
	{ "SetPlayerNameTagPlate", n_SetPlayerNameTagPlate },
	{ "SetPlayerNameTagParam", n_SetPlayerNameTagParam },
	{ "SetPlayerNameGradient", n_SetPlayerNameGradient },
	{ "ResetPlayerNameTag", n_ResetPlayerNameTag },
	{ "SetPedNameGradient", n_SetPedNameGradient },
	{ "SetPlayerChatBubbleEx", n_SetPlayerChatBubbleEx },
	{ "ShowDamageText", n_ShowDamageText },
	{ "SetPlayerChatBadge", n_SetPlayerChatBadge },
	{ "Cef_CreateBrowser", n_Cef_CreateBrowser },
	{ "Cef_DestroyBrowser", n_Cef_DestroyBrowser },
	{ "Cef_SetBrowserPos", n_Cef_SetBrowserPos },
	{ "Cef_SetBrowserUrl", n_Cef_SetBrowserUrl },
	{ "Cef_ExecuteJS", n_Cef_ExecuteJS },
	{ "Cef_SendMessage", n_Cef_SendMessage },
	{ "Cef_ToggleBrowser", n_Cef_ToggleBrowser },
	{ "Cef_SetBrowserFocus", n_Cef_SetBrowserFocus },
	{ "CreatePed", n_CreatePed },
	{ "DestroyPed", n_DestroyPed },
	{ "IsValidPed", n_IsValidPed },
	{ "GetPedPoolSize", n_GetPedPoolSize },
	{ "SetPedPos", n_SetPedPos },
	{ "GetPedPos", n_GetPedPos },
	{ "SetPedAngle", n_SetPedAngle },
	{ "GetPedAngle", n_GetPedAngle },
	{ "SetPedSkin", n_SetPedSkin },
	{ "GetPedSkin", n_GetPedSkin },
	{ "SetPedWeapon", n_SetPedWeapon },
	{ "GetPedWeapon", n_GetPedWeapon },
	{ "SetPedName", n_SetPedName },
	{ "SetPedColor", n_SetPedColor },
	{ "SetPedCollision", n_SetPedCollision },
	{ "SetPedAnimGroup", n_SetPedAnimGroup },
	{ "SetPedVirtualWorld", n_SetPedVirtualWorld },
	{ "GetPedVirtualWorld", n_GetPedVirtualWorld },
	{ "SetPedInterior", n_SetPedInterior },
	{ "GetPedInterior", n_GetPedInterior },
	{ "SetPedStreamDistance", n_SetPedStreamDistance },
	{ "SetPedHealth", n_SetPedHealth },
	{ "GetPedHealth", n_GetPedHealth },
	{ "SetPedArmour", n_SetPedArmour },
	{ "GetPedArmour", n_GetPedArmour },
	{ "SetPedInvulnerable", n_SetPedInvulnerable },
	{ "IsPedInvulnerable", n_IsPedInvulnerable },
	{ "IsPedDead", n_IsPedDead },
	{ "RespawnPed", n_RespawnPed },
	{ "PedGoTo", n_PedGoTo },
	{ "PedStop", n_PedStop },
	{ "PedApplyAnimation", n_PedApplyAnimation },
	{ "PedShootAt", n_PedShootAt },
	{ "PedShootAtPlayer", n_PedShootAtPlayer },
	{ "PedShootAtPed", n_PedShootAtPed },
	{ "PedAttackPlayer", n_PedAttackPlayer },
	{ "PedAttackPed", n_PedAttackPed },
	{ "PedStopAttack", n_PedStopAttack },
	{ "GetPedAttackTarget", n_GetPedAttackTarget },
	{ "PedEnterVehicle", n_PedEnterVehicle },
	{ "PedExitVehicle", n_PedExitVehicle },
	{ "GetPedVehicle", n_GetPedVehicle },
	{ "SetPedFollowVehicle", n_SetPedFollowVehicle },
	{ "GetPedFollowVehicle", n_GetPedFollowVehicle },
	{ "SetPedEnterTime", n_SetPedEnterTime },
	{ "IsPedEnteringVehicle", n_IsPedEnteringVehicle },
	{ "SetPedAccuracy", n_SetPedAccuracy },
	{ "SendDeathMessageByName", n_SendDeathMessageByName },
	{ "SendPedDeathMessage", n_SendPedDeathMessage },
	{ "GetPedAccuracy", n_GetPedAccuracy },
	{ "SetPedChatBubble", n_SetPedChatBubble },
	{ "SetPedAttachedObject", n_SetPedAttachedObject },
	{ "RemovePedAttachedObject", n_RemovePedAttachedObject },
	{ "SetPedFollowPlayer", n_SetPedFollowPlayer },
	{ "GetPedFollowPlayer", n_GetPedFollowPlayer },
	{ "GetPedSyncOwner", n_GetPedSyncOwner },
	{ "IsPedStreamedIn", n_IsPedStreamedIn },
	{ "SetPedSyncRate", n_Ped_SetSyncRate },
#endif
};

bool Plugin::Init(void* amxFunctions)
{
	pAMXFunctions = amxFunctions;
	if (!pAMXFunctions) return false;
#if !defined(MODGEAR_PLATFORM_OMP)
	InstallRegisterHook();
#endif
	return true;
}

// перехват amx_Register (машины с кастомными моделями, скины). В open.mp - при загрузке первого скрипта: после
// загрузки плагинов (Streamer/sampgdk)
// затирала хук sampgdk, и Streamer не находил нативы (IsPlayerNPC, GetPlayerInterior...)
void Plugin::InstallRegisterHook()
{
	if (!pAMXFunctions) return;
	Hook_amxRegister.install((urmem::address_t)reinterpret_cast<void**>(pAMXFunctions)[PLUGIN_AMX_EXPORT_Register], urmem::get_func_addr(&amx_RegisterHook));
}

void Plugin::Shutdown()
{
	Media::Stop();
#if MODGEAR_FULL
	AzRtc::Stop();
#endif
	Hook_amxRegister.disable();
	pAMX.clear();
}

namespace
{
	// "0.5.8" -> {0,5,8}

	// Рамка лога запуска. Строки плагина в cp1251 (/execution-charset): символ = байт
	// Ширина подстраивается под самую длинную строку - ничего не вылезает и не переносится
	struct BannerRow { std::string label, value; };

	std::string Pad(const std::string& s, size_t width) { return s.size() < width ? s + std::string(width - s.size(), ' ') : s; }
}

void Plugin::PrintBanner(const char* serverName)
{
	const auto M = Platform::LogLevel::Message;
	std::vector<BannerRow> rows;
	auto Row = [&rows](const char* label, const std::string& value) { rows.push_back({ label, value }); };

	Row("Версия", std::string(MODGEAR_NAME) + " " + MODGEAR_VERSION_STRING);
#ifdef _WIN32
	Row("Сервер", std::string(serverName) + " / Windows");
#else
	Row("Сервер", std::string(serverName) + " / Linux");
#endif
	Row("Автор", "PainFrenemy2");
	Row("GitHub", "github.com/PainFrenemy2/ModGear");

	size_t valueWidth = 47;
	for (const BannerRow& r : rows) if (r.value.size() + 1 > valueWidth) valueWidth = r.value.size() + 1;
	const size_t inner = 13 + valueWidth;
	const std::string border = "  +" + std::string(inner, '-') + "+";
	auto Line = [&](const std::string& text) { Platform::Log(M, "  |%s|", Pad(text, inner).c_str()); };

	Platform::Log(M, " ");
	Platform::Log(M, "%s", border.c_str());
	Line(R"(    __  __           _  ____)");
	Line(R"(   |  \/  | ___   __| |/ ___| ___  __ _ _ __)");
	Line(R"(   | |\/| |/ _ \ / _` | |  _ / _ \/ _` | '__|)");
	Line(R"(   | |  | | (_) | (_| | |_| |  __/ (_| | |)");
	Line(R"(   |_|  |_|\___/ \__,_|\____|\___|\__,_|_|)");
	Line("");
	Platform::Log(M, "%s", border.c_str());
	for (const BannerRow& r : rows) Line("  " + Pad(r.label, 10) + " " + r.value);
	Platform::Log(M, "%s", border.c_str());
	Platform::Log(M, " ");
}

void Plugin::OnAmxLoad(AMX* amx)
{
#if defined(MODGEAR_PLATFORM_OMP)
	// open.mp: к загрузке первого скрипта плагины (Streamer) уже стоят, а нативы сервера скрипту ещё не
	// привязаны - ставим перехват сейчас, чтобы не затереть хук sampgdk и поймать CreateVehicle и т.д
	static bool registerHooked = false;
	if (!registerHooked) { registerHooked = true; InstallRegisterHook(); }
#endif
	amx_Register(amx, plugin_natives, (int)(sizeof(plugin_natives) / sizeof(plugin_natives[0])));
	pAMX.push_back(amx);

	int index = 0;
	if (amx_FindNative(amx, MODGEAR_VERSION_CHECK_NATIVE, &index) == AMX_ERR_NONE) CheckVersion();
}

void Plugin::OnAmxUnload(AMX* amx)
{
	pAMX.remove(amx);
	Peds::FreeScript(amx);
#if MODGEAR_FULL
	// экраны и звук этого скрипта гасим у игроков и удаляем: после gmx старое видео/радио не остаётся
	Video::FreeOwner(amx);
	Voice::FreeOwner(amx);
	for (int i = 0; i < (int)aTemp.size(); i++)
		if (aTemp[i].aType && aTemp[i].aOwner == amx) Native::Audio_Free(i);
#endif
}

void Plugin::CallPublic(const char* name, std::initializer_list<int> args)
{
	for (AMX* amx : pAMX)
	{
		int idx = 0;
		if (amx_FindPublic(amx, name, &idx) != AMX_ERR_NONE) continue;
		cell ret = 0;
		for (auto it = std::rbegin(args); it != std::rend(args); ++it) amx_Push(amx, (cell)*it);
		amx_Exec(amx, &ret, idx);
	}
}

int Plugin::CallPublicRet(const char* name, std::initializer_list<int> args)
{
	int result = 1;
	for (AMX* amx : pAMX)
	{
		int idx = 0;
		if (amx_FindPublic(amx, name, &idx) != AMX_ERR_NONE) continue;
		cell ret = 1;
		for (auto it = std::rbegin(args); it != std::rend(args); ++it) amx_Push(amx, (cell)*it);
		amx_Exec(amx, &ret, idx);
		if (!ret) result = 0;
	}
	return result;
}

void Plugin::CallPublicStr(const char* name, int a, const char* text, int b)
{
	for (AMX* amx : pAMX)
	{
		int idx = 0;
		if (amx_FindPublic(amx, name, &idx) != AMX_ERR_NONE) continue;
		cell ret = 0, addr = 0;
		amx_Push(amx, (cell)b);
		cell* phys = nullptr;
		amx_PushString(amx, &addr, &phys, text, 0, 0);
		amx_Push(amx, (cell)a);
		amx_Exec(amx, &ret, idx);
		amx_Release(amx, addr);
	}
}

void Plugin::CallPublic(const char* name, int value)
{
	for (AMX* amx : pAMX)
	{
		int idx = 0;
		if (amx_FindPublic(amx, name, &idx) != AMX_ERR_NONE) continue;
		cell ret = 0;
		amx_Push(amx, (cell)value);
		amx_Exec(amx, &ret, idx);
	}
}
