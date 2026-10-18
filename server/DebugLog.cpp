#include "DebugLog.h"
#include "Paths.h"

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
	constexpr long MAX_BYTES = 5 * 1024 * 1024;
	std::mutex mx;
}

// строки плагина на Windows в cp1251 (как у консоли SA-MP), файл - в UTF-8, чтобы открывался в любом редакторе
static std::string ToUtf8(const char* text)
{
#ifdef _WIN32
	wchar_t wide[1024];
	const int n = MultiByteToWideChar(1251, 0, text, -1, wide, 1024);
	if (n <= 0) return text;
	char out[3072];
	const int m = WideCharToMultiByte(CP_UTF8, 0, wide, -1, out, sizeof(out), nullptr, nullptr);
	return m > 0 ? std::string(out) : std::string(text);
#else
	return text;
#endif
}

void DebugLog::Write(const char* format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(text, sizeof(text), format, args);
	va_end(args);

	char stamp[32];
	const std::time_t now = std::time(nullptr);
	std::tm tm{};
#ifdef _WIN32
	localtime_s(&tm, &now);
#else
	localtime_r(&now, &tm);
#endif
	std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &tm);

	std::lock_guard<std::mutex> lock(mx);
	static const std::string path = ServerPath("modgear_debug.log");
	FILE* f = std::fopen(path.c_str(), "ab");
	if (!f) return;
	std::fseek(f, 0, SEEK_END);
	if (std::ftell(f) > MAX_BYTES)
	{
		std::fclose(f);
		f = std::fopen(path.c_str(), "wb");
		if (!f) return;
	}
	std::fprintf(f, "[%s] %s\n", stamp, ToUtf8(text).c_str());
	std::fclose(f);
}
