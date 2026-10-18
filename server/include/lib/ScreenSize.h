#pragma once
// Р Р°Р·СЂРµС€РµРЅРёРµ СЌРєСЂР°РЅР° РёРіСЂРѕРєР°. РљР»РёРµРЅС‚ Arizona СЃР°Рј РµРіРѕ РЅРµ РїСЂРёСЃС‹Р»Р°РµС‚, РїРѕСЌС‚РѕРјСѓ СЃРїСЂР°С€РёРІР°РµРј С‡РµСЂРµР· РёРЅС‚РµСЂС„РµР№СЃ
// РІ CEF (РѕРєРЅРѕ 0) РІС‹РїРѕР»РЅСЏРµС‚СЃСЏ JS, РєРѕС‚РѕСЂС‹Р№ С€Р»С‘С‚ "mg_screen|С€РёСЂРёРЅР°|РІС‹СЃРѕС‚Р°" РѕР±СЂР°С‚РЅРѕ С‡РµСЂРµР· cef.SendMessage

#include <lib/Native.h>
#include "../../DebugLog.h"
#include "../../Plugin.h"
#include <cstdlib>
#include <string>

namespace ScreenSize
{
	constexpr const char* PREFIX = "mg_screen|";
	inline int width[MAX_PLAYERS];
	inline int height[MAX_PLAYERS];

	inline void Request(int playerid)
	{
		if (playerid < 0 || playerid >= MAX_PLAYERS || !PI[playerid].pLauncherStat) return;
		Native::ShowCef(playerid,
			"(function(){try{var r=window.devicePixelRatio||1;"
			"cef.SendMessage('mg_screen|'+Math.round(window.innerWidth*r)+'|'+Math.round(window.innerHeight*r),0);"
			"}catch(e){}})();");
	}

	// РѕС‚РІРµС‚ РёР· 220:18; true - СЌС‚Рѕ Р±С‹Р» РЅР°С€ РѕС‚РІРµС‚ (РґР°Р»СЊС€Рµ РІ OnPlayerCefEvent РЅРµ РїРµСЂРµРґР°С‘Рј)
	inline bool OnCefEvent(int playerid, const std::string& event)
	{
		if (event.compare(0, 10, PREFIX) != 0) return false;
		const char* p = event.c_str() + 10;
		char* end = nullptr;
		const long w = std::strtol(p, &end, 10);
		if (!end || *end != '|') return true;
		const long h = std::strtol(end + 1, nullptr, 10);
		if (w <= 0 || h <= 0 || w > 16384 || h > 16384) return true;
		const bool changed = width[playerid] != w || height[playerid] != h;
		width[playerid] = (int)w;
		height[playerid] = (int)h;
		if (changed) Plugin::CallPublic("OnPlayerScreenSize", { playerid, (int)w, (int)h });
		return true;
	}

	inline void PlayerLeft(int playerid)
	{
		if (playerid < 0 || playerid >= MAX_PLAYERS) return;
		width[playerid] = height[playerid] = 0;
	}
}

// RequestPlayerScreenSize(playerid) - СЃРїСЂРѕСЃРёС‚СЊ СЂР°Р·СЂРµС€РµРЅРёРµ СЌРєСЂР°РЅР°; РѕС‚РІРµС‚ РїСЂРёРґС‘С‚ РІ OnPlayerScreenSize
static cell AMX_NATIVE_CALL n_RequestPlayerScreenSize(AMX* amx, cell* params)
{
	if (params[0] / (cell)sizeof(cell) < 1 || params[1] < 0 || params[1] >= MAX_PLAYERS || !PI[params[1]].pLauncherStat) return 0;
	ScreenSize::Request((int)params[1]);
	return 1;
}

// GetPlayerScreenSize(playerid, &width, &height) - РїРѕСЃР»РµРґРЅРµРµ РёР·РІРµСЃС‚РЅРѕРµ СЂР°Р·СЂРµС€РµРЅРёРµ; 0 - РµС‰С‘ РЅРµ Р·РЅР°РµРј
static cell AMX_NATIVE_CALL n_GetPlayerScreenSize(AMX* amx, cell* params)
{
	if (params[0] / (cell)sizeof(cell) < 3 || params[1] < 0 || params[1] >= MAX_PLAYERS) return 0;
	const int id = (int)params[1];
	cell* w = nullptr;
	cell* h = nullptr;
	if (amx_GetAddr(amx, params[2], &w) != AMX_ERR_NONE || amx_GetAddr(amx, params[3], &h) != AMX_ERR_NONE) return 0;
	*w = ScreenSize::width[id];
	*h = ScreenSize::height[id];
	return ScreenSize::width[id] > 0;
}
