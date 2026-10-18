#pragma once
// ModGear: RakNet из open.mp вынесен из ядра сервера. Вместо ICore / Query / типов SDK —
// эти заглушки: логи уходят в функцию, которую задаёт плагин, query не обслуживается.

#include <cstdarg>
#include <cstddef>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#if defined _WIN32 || defined WIN32
#include <winsock2.h>
#else
#include <netinet/in.h>
#endif

using StringView = std::string_view;
template <typename A, typename B> using Pair = std::pair<A, B>;
template <typename K, typename V> using FlatHashMap = std::unordered_map<K, V>;
template <typename K> using FlatHashSet = std::unordered_set<K>;

template <typename T>
class Span
{
public:
	Span() = default;
	Span(T* data, size_t size) : data_(data), size_(size) {}
	T* data() const { return data_; }
	size_t size() const { return size_; }
private:
	T* data_ = nullptr;
	size_t size_ = 0;
};

enum class RakLogLevel { Debug, Message, Warning, Error };

struct RakConfigShim
{
	bool* getBool(const char*) { return nullptr; }
};

class RakCoreShim
{
public:
	using LogFn = void (*)(RakLogLevel level, const char* text);
	static void SetLog(LogFn fn) { log_ = fn; }

	void printLn(const char* fmt, ...)
	{
		va_list args; va_start(args, fmt); Write(RakLogLevel::Message, fmt, args); va_end(args);
	}
	void logLn(RakLogLevel level, const char* fmt, ...)
	{
		va_list args; va_start(args, fmt); Write(level, fmt, args); va_end(args);
	}
	RakConfigShim& getConfig() { static RakConfigShim config; return config; }

	static RakCoreShim* Get() { static RakCoreShim core; return &core; }

private:
	static void Write(RakLogLevel level, const char* fmt, va_list args);
	static inline LogFn log_ = nullptr;
};

class Query
{
public:
	Span<const char> handleQuery(Span<const char>, SOCKET, const sockaddr_in&, int) { return {}; }
};
