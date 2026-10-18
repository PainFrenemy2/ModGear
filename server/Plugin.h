#pragma once

// Общая часть плагина: хуки AMX, нативы, колбэки в Pawn

#include <initializer_list>

typedef struct tagAMX AMX;

namespace Plugin
{
	// pAMXFunctions — таблица amx_* функций сервера
	bool Init(void* amxFunctions);
	void InstallRegisterHook();             // перехват amx_Register (SA-MP - из Init, open.mp - из OnAmxLoad)
	void Shutdown();

	void PrintBanner(const char* serverName);
	void OnAmxLoad(AMX* amx);
	void OnAmxUnload(AMX* amx);

	// вызвать public name(value) во всех скриптах (только из основного потока сервера)
	void CallPublic(const char* name, int value);
	void CallPublic(const char* name, std::initializer_list<int> args);
	// то же, но вернуть результат: 0 - хоть один скрипт вернул 0 (отмена), 1 - иначе / public нет
	int CallPublicRet(const char* name, std::initializer_list<int> args);
	// public name(a, const text[], b)
	void CallPublicStr(const char* name, int a, const char* text, int b);
}
