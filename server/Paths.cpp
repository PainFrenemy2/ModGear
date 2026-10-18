#include "Paths.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <climits>
#include <unistd.h>
#endif

const std::string& ServerDir()
{
	static const std::string dir = []() -> std::string
	{
#ifdef _WIN32
		char path[MAX_PATH];
		const DWORD n = GetModuleFileNameA(nullptr, path, MAX_PATH);
		if (n == 0 || n >= MAX_PATH) return "";
		std::string s(path, n);
		// символ вне кодовой страницы ANSI приходит как '?': такой путь не откроется - остаёмся на относительных
		if (s.find('?') != std::string::npos) return "";
		const size_t slash = s.find_last_of("\\/");
#else
		char path[PATH_MAX];
		const ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
		if (n <= 0) return "";
		std::string s(path, (size_t)n);
		const size_t slash = s.rfind('/');
#endif
		return slash == std::string::npos ? std::string() : s.substr(0, slash + 1);
	}();
	return dir;
}

