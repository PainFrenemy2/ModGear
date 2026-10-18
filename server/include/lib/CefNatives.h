#pragma once
// Свои окна CEF (браузеры лаунчера, cef/loader.dll). Пакеты 220:10..25, строки - Native::WriteCefString
// Окно 0 - главный интерфейс Arizona (его грузит сам плагин), свои окна - 1..7
// Подключается в Plugin.cpp после ArzExtraNatives.h

#include <lib/ArzExtraNatives.h>
#include <lib/CefResponse.h>
#include <cstdlib>

namespace CefNatives
{
	constexpr int MAX_BROWSERS = 8;

	inline bool Browser(cell id) { return id >= 1 && id < MAX_BROWSERS; }

	inline void Send(int playerid, BitStream& bs)
	{
		if (ArzExtra::Launcher(playerid)) Native::MBSendPacket(playerid, &bs, 1, 9);
	}
}

// Cef_CreateBrowser(playerid, browserid, const url[], x = 0, y = 0, width = 0, height = 0) - открыть страницу в окне 1..7
// width/height 0 - до края экрана. Готовность окна не сообщается, события страницы - OnPlayerCefEvent
static cell AMX_NATIVE_CALL n_Cef_CreateBrowser(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 7 || !ArzNatives::Player(params[1]) || !CefNatives::Browser(params[2])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 10);
	bs.Write((int32_t)params[6]);
	bs.Write((int32_t)params[7]);
	bs.Write((int32_t)params[4]);
	bs.Write((int32_t)params[5]);
	Native::WriteCefString(bs, ArzNatives::Str(amx, params[3], 2048).c_str());
	Native::WriteCefString(bs, "");
	bs.Write((int32_t)params[2]);
	CefNatives::Send((int)params[1], bs);
	return 1;
}
// Cef_DestroyBrowser(playerid, browserid)
static cell AMX_NATIVE_CALL n_Cef_DestroyBrowser(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Player(params[1]) || !CefNatives::Browser(params[2])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 14);
	bs.Write((int32_t)params[2]);
	CefNatives::Send((int)params[1], bs);
	return 1;
}
// Cef_SetBrowserPos(playerid, browserid, x, y)
static cell AMX_NATIVE_CALL n_Cef_SetBrowserPos(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 4 || !ArzNatives::Player(params[1]) || !CefNatives::Browser(params[2])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 15);
	bs.Write((int32_t)params[2]);
	bs.Write((int32_t)params[3]);
	bs.Write((int32_t)params[4]);
	CefNatives::Send((int)params[1], bs);
	return 1;
}
// Cef_SetBrowserUrl(playerid, browserid, const url[]) - перейти на другую страницу
static cell AMX_NATIVE_CALL n_Cef_SetBrowserUrl(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::Player(params[1]) || !CefNatives::Browser(params[2])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 16);
	bs.Write((int32_t)params[2]);
	Native::WriteCefString(bs, ArzNatives::Str(amx, params[3], 2048).c_str());
	CefNatives::Send((int)params[1], bs);
	return 1;
}
// Cef_ExecuteJS(playerid, browserid, const code[]) - выполнить JS в окне (0 - главный интерфейс)
static cell AMX_NATIVE_CALL n_Cef_ExecuteJS(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::Player(params[1]) || params[2] < 0 || params[2] >= CefNatives::MAX_BROWSERS) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 17);
	bs.Write((int32_t)params[2]);
	Native::WriteCefString(bs, ArzNatives::Str(amx, params[3], 8192).c_str());
	bs.Write((int32_t)-1);
	CefNatives::Send((int)params[1], bs);
	return 1;
}
// Cef_SendMessage(playerid, uid, const text[]) - во все окна: cef.onMessage(uid, text)
static cell AMX_NATIVE_CALL n_Cef_SendMessage(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::Player(params[1])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 18);
	Native::WriteCefString(bs, ArzNatives::Str(amx, params[3], 8192).c_str());
	bs.Write((int32_t)params[2]);
	CefNatives::Send((int)params[1], bs);
	return 1;
}
// Cef_ToggleBrowser(playerid, browserid) - показать / скрыть окно
static cell AMX_NATIVE_CALL n_Cef_ToggleBrowser(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !ArzNatives::Player(params[1]) || !CefNatives::Browser(params[2])) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 22);
	bs.Write((int32_t)params[2]);
	CefNatives::Send((int)params[1], bs);
	return 1;
}
// Cef_SetBrowserFocus(playerid, browserid, bool:focus) - курсор и клавиатура в окне
static cell AMX_NATIVE_CALL n_Cef_SetBrowserFocus(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3 || !ArzNatives::Player(params[1]) || params[2] < 0 || params[2] >= CefNatives::MAX_BROWSERS) return 0;
	BitStream bs;
	ArzExtra::Begin(bs, 25);
	bs.Write((int32_t)params[2]);
	bs.Write(params[3] != 0);
	CefNatives::Send((int)params[1], bs);
	return 1;
}

// ======== ответы интерфейса: public OnPlayerCefResponse(playerid, const event[], argc) ========
namespace CefResponse
{
	inline cell SetString(AMX* amx, cell dest, cell size, const std::string* value)
	{
		cell* ptr = nullptr;
		if (amx_GetAddr(amx, dest, &ptr) != AMX_ERR_NONE || !ptr || size <= 0) return 0;
		amx_SetString(ptr, value ? value->c_str() : "", 0, 0, (size_t)size);
		return value ? 1 : 0;
	}
}

// Cef_GetArgCount() - сколько параметров у текущего ответа (только внутри OnPlayerCefResponse)
static cell AMX_NATIVE_CALL n_Cef_GetArgCount(AMX* amx, cell* params)
{
	return CefResponse::active ? (cell)CefResponse::current.items.size() : 0;
}

// Cef_GetArg(index, dest[], size = sizeof dest) - параметр строкой, 0 - нет такого
static cell AMX_NATIVE_CALL n_Cef_GetArg(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3) return 0;
	return CefResponse::SetString(amx, params[2], params[3], CefResponse::Arg(params[1]));
}

// Cef_GetArgInt(index) - параметр числом (0, если нет)
static cell AMX_NATIVE_CALL n_Cef_GetArgInt(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1) return 0;
	const std::string* v = CefResponse::Arg(params[1]);
	return v ? (cell)std::strtoll(v->c_str(), nullptr, 0) : 0;
}

// Float:Cef_GetArgFloat(index)
static cell AMX_NATIVE_CALL n_Cef_GetArgFloat(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1) return 0;
	const std::string* v = CefResponse::Arg(params[1]);
	const float f = v ? std::strtof(v->c_str(), nullptr) : 0.0f;
	return amx_ftoc(f);
}

// Cef_GetArgByName(const name[], dest[], size = sizeof dest) - поле объекта JSON {"name": ...}
static cell AMX_NATIVE_CALL n_Cef_GetArgByName(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 3) return 0;
	return CefResponse::SetString(amx, params[2], params[3], CefResponse::Named(ArzNatives::Str(amx, params[1], 128)));
}

// Cef_GetArgIntByName(const name[])
static cell AMX_NATIVE_CALL n_Cef_GetArgIntByName(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1) return 0;
	const std::string* v = CefResponse::Named(ArzNatives::Str(amx, params[1], 128));
	return v ? (cell)std::strtoll(v->c_str(), nullptr, 0) : 0;
}

// Float:Cef_GetArgFloatByName(const name[])
static cell AMX_NATIVE_CALL n_Cef_GetArgFloatByName(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 1) return 0;
	const std::string* v = CefResponse::Named(ArzNatives::Str(amx, params[1], 128));
	const float f = v ? std::strtof(v->c_str(), nullptr) : 0.0f;
	return amx_ftoc(f);
}

// Cef_GetRaw(dest[], size = sizeof dest) - вся строка ответа как пришла
static cell AMX_NATIVE_CALL n_Cef_GetRaw(AMX* amx, cell* params)
{
	if (ArzNatives::Args(params) < 2 || !CefResponse::active) return 0;
	const std::string raw = CefResponse::ToCp1251(CefResponse::current.raw);
	return CefResponse::SetString(amx, params[1], params[2], &raw);
}
