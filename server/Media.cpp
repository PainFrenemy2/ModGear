#include "Media.h"
#include "Http.h"
#include "Platform.h"
#include "Paths.h"
#include "DebugLog.h"

#include <mbedtls/net_sockets.h>
#include <mbedtls/sha256.h>

#include <atomic>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <sys/stat.h>

#ifdef _WIN32
	#include <direct.h>
	#include <process.h>
	#include <windows.h>
	#define popen _popen
	#define pclose _pclose
	#define MEDIA_NULL " 2>NUL"
	#define MEDIA_READ "rb"
#else
	#include <dirent.h>
	#include <pthread.h>
	#define MEDIA_NULL " 2>/dev/null"
	#define MEDIA_READ "r"
#endif

#include <algorithm>
#include <vector>
#include <nlohmann/json.hpp>

// предел кэша треков на диске (МБ): дальше удаляются самые старые
#ifndef MEDIA_CACHE_MAX_MB
	#define MEDIA_CACHE_MAX_MB 500
#endif

namespace
{
	// от папки сервера, а не от рабочей папки процесса
	const std::string& CacheDir() { static const std::string dir = ServerPath("scriptfiles/modgear_media"); return dir; }

	std::atomic<bool> running{ false };
	mbedtls_net_context listenCtx;
	int serverPort = 0;
	int videoQuality = 720;   // высота кадра видео Twitch по умолчанию (480/720/1080), задаётся в Media_Init
	std::mutex hostMx;
	std::string serverHost;

	struct Job { std::atomic<bool> done{ false }; std::atomic<bool> ok{ false }; };
	std::mutex jobsMx;
	std::map<std::string, std::shared_ptr<Job>> jobs;

	std::mutex durMx;
	std::map<std::string, int> durations;   // мс; 0 - узнать не удалось
	std::set<std::string> pending;

	std::mutex titleMx;
	std::map<std::string, std::string> titles;   // "" - нет названия/не удалось
	std::set<std::string> titlePending;

	bool FileExists(const std::string& path)
	{
		struct stat st;
		return stat(path.c_str(), &st) == 0;
	}
	long long FileSize(const std::string& path)
	{
		struct stat st;
		return stat(path.c_str(), &st) == 0 ? (long long)st.st_size : -1;
	}
	void MakeCacheDir()
	{
#ifdef _WIN32
		_mkdir(ServerPath("scriptfiles").c_str());
		_mkdir(CacheDir().c_str());
#else
		mkdir(ServerPath("scriptfiles").c_str(), 0755);
		mkdir(CacheDir().c_str(), 0755);
#endif
	}
	// кэш не должен расти бесконечно: держим его под MEDIA_CACHE_MAX_MB, удаляя самые старые треки
	void PruneCache()
	{
		struct Item { std::string id; long long mtime; long long size; };
		std::vector<Item> items;
		long long total = 0;
		const std::string dir = CacheDir();
#ifdef _WIN32
		WIN32_FIND_DATAA fd;
		HANDLE h = FindFirstFileA((dir + "\\*.mp3").c_str(), &fd);
		if (h != INVALID_HANDLE_VALUE)
		{
			do
			{
				std::string name = fd.cFileName;
				if (name.size() <= 4) continue;
				const std::string id = name.substr(0, name.size() - 4);
				struct stat st;
				if (stat((dir + "/" + name).c_str(), &st) != 0) continue;
				items.push_back({ id, (long long)st.st_mtime, (long long)st.st_size });
				total += st.st_size;
			} while (FindNextFileA(h, &fd));
			FindClose(h);
		}
#else
		DIR* d = opendir(dir.c_str());
		if (d)
		{
			struct dirent* e;
			while ((e = readdir(d)))
			{
				std::string name = e->d_name;
				if (name.size() <= 4 || name.compare(name.size() - 4, 4, ".mp3") != 0) continue;
				const std::string id = name.substr(0, name.size() - 4);
				struct stat st;
				if (stat((dir + "/" + name).c_str(), &st) != 0) continue;
				items.push_back({ id, (long long)st.st_mtime, (long long)st.st_size });
				total += st.st_size;
			}
			closedir(d);
		}
#endif
		const long long cap = (long long)MEDIA_CACHE_MAX_MB * 1024 * 1024;
		if (total <= cap) return;

		std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.mtime < b.mtime; });
		for (const Item& it : items)
		{
			if (total <= cap) break;
			remove((dir + "/" + it.id + ".mp3").c_str());
			remove((dir + "/" + it.id + ".ok").c_str());
			total -= it.size;
		}
	}

	std::string Mp3Path(const std::string& id) { return CacheDir() + "/" + id + ".mp3"; }
	std::string OkPath(const std::string& id) { return CacheDir() + "/" + id + ".ok"; }
	bool Cached(const std::string& id) { return FileExists(OkPath(id)) && FileExists(Mp3Path(id)); }

	// программа в папке сервера (полный путь: cmd с NoDefaultCurrentDirectoryInExePath не ищет в текущей
	// папке, а рабочая папка процесса может быть другой) или из PATH
	std::string Tool(const char* name)
	{
#ifdef _WIN32
		const std::string local = ServerPath(std::string(name) + ".exe");
#else
		const std::string local = ServerDir().empty() ? std::string("./") + name : ServerPath(name);
#endif
		return FileExists(local) ? local : std::string(name);
	}

	bool ValidId(const std::string& id)
	{
		if (id.size() != 11) return false;
		for (char c : id) if (!isalnum((unsigned char)c) && c != '-' && c != '_') return false;
		return true;
	}
	// ссылка, которую безопасно передать в командную строку в кавычках
	bool SafeUrl(const std::string& url)
	{
		if (url.size() < 10 || url.size() > 500) return false;
		if (url.compare(0, 7, "http://") != 0 && url.compare(0, 8, "https://") != 0) return false;
		for (size_t i = 0; i < url.size(); i++)
		{
			const char c = url[i];
			if (!isalnum((unsigned char)c) && !strchr(":/?&=._-~+#%@,", c)) return false;
			// % - только как %XX: иначе cmd подставит переменную окружения (%PATH% и т.п.) в ссылку
			if (c == '%' && (i + 2 >= url.size() || !isxdigit((unsigned char)url[i + 1]) || !isxdigit((unsigned char)url[i + 2]))) return false;
		}
		return true;
	}

	// cmd.exe снимает первую и последнюю кавычку строки - на Windows оборачиваем команду целиком
	FILE* OpenPipe(const std::string& cmd)
	{
#ifdef _WIN32
		return popen(("\"" + cmd + MEDIA_NULL "\"").c_str(), MEDIA_READ);
#else
		return popen((cmd + MEDIA_NULL).c_str(), MEDIA_READ);
#endif
	}

	std::string RunRead(const std::string& cmd)
	{
		std::string out;
		FILE* p = OpenPipe(cmd);
		if (!p) return out;
		char buf[512];
		size_t n;
		while ((n = fread(buf, 1, sizeof(buf), p)) > 0) out.append(buf, n);
		pclose(p);
		return out;
	}

	// одна загрузка на трек: пишет mp3 в кэш, слушатели читают файл по мере записи
	std::shared_ptr<Job> EnsureJob(const std::string& id)
	{
		std::lock_guard<std::mutex> lock(jobsMx);
		auto it = jobs.find(id);
		if (it != jobs.end()) return it->second;
		if (Cached(id)) return nullptr;
		auto job = std::make_shared<Job>();
		jobs[id] = job;
		std::thread([id, job]()
		{
			MakeCacheDir();
			const std::string path = Mp3Path(id);
			FILE* out = fopen(path.c_str(), "wb");
			long long bytes = 0;
			int status = -1;
			if (out)
			{
				const std::string cmd = "\"" + Tool("yt-dlp") + "\" -q --no-warnings --no-playlist -f bestaudio -o - \"https://www.youtube.com/watch?v=" + id + "\"" MEDIA_NULL
					" | \"" + Tool("ffmpeg") + "\" -loglevel quiet -i pipe:0 -vn -ac 2 -ar 44100 -f mp3 -b:a 128k pipe:1";
				FILE* p = OpenPipe(cmd);
				if (p)
				{
					char buf[16384];
					size_t n;
					while ((n = fread(buf, 1, sizeof(buf), p)) > 0)
					{
						fwrite(buf, 1, n, out);
						fflush(out);
						bytes += (long long)n;
					}
					status = pclose(p);
				}
				fclose(out);
			}
			const bool ok = bytes > 16 * 1024 && status == 0;
			if (ok)
			{
				FILE* f = fopen(OkPath(id).c_str(), "wb");
				if (f) fclose(f);
				DebugLog::Write("[ModGear]: Медиа: звук YouTube %s готов (%lld КБ).", id.c_str(), bytes / 1024);
				PruneCache();
			}
			else
			{
				remove(path.c_str());
				Platform::Log(Platform::LogLevel::Warning, "[ModGear]: Не удалось получить звук YouTube %s. Проверьте, что рядом с сервером есть yt-dlp и ffmpeg.", id.c_str());
			}
			job->ok = ok;
			job->done = true;
			std::lock_guard<std::mutex> lock(jobsMx);
			jobs.erase(id);
		}).detach();
		return job;
	}

	bool SendAll(mbedtls_net_context* c, const char* data, size_t len)
	{
		while (len)
		{
			int r = mbedtls_net_send(c, (const unsigned char*)data, len);
			if (r <= 0) return false;
			data += r;
			len -= (size_t)r;
		}
		return true;
	}
	bool SendAll(mbedtls_net_context* c, const std::string& s) { return SendAll(c, s.data(), s.size()); }

	void ServeCached(mbedtls_net_context* c, const std::string& id, const std::string& request)
	{
		const std::string path = Mp3Path(id);
		const long long size = FileSize(path);
		FILE* f = fopen(path.c_str(), "rb");
		if (!f || size <= 0) { if (f) fclose(f); SendAll(c, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"); return; }
		long long from = 0;
		size_t r = request.find("Range: bytes=");
		if (r == std::string::npos) r = request.find("range: bytes=");
		if (r != std::string::npos) from = atoll(request.c_str() + r + 13);
		if (from < 0 || from >= size) from = 0;
		std::string head = from > 0
			? "HTTP/1.1 206 Partial Content\r\nContent-Range: bytes " + std::to_string(from) + "-" + std::to_string(size - 1) + "/" + std::to_string(size) + "\r\n"
			: std::string("HTTP/1.1 200 OK\r\n");
		head += "Content-Type: audio/mpeg\r\nAccept-Ranges: bytes\r\nContent-Length: " + std::to_string(size - from) + "\r\nConnection: close\r\n\r\n";
		if (SendAll(c, head))
		{
			fseek(f, (long)from, SEEK_SET);
			char buf[16384];
			size_t n;
			while ((n = fread(buf, 1, sizeof(buf), f)) > 0) if (!SendAll(c, buf, n)) break;
		}
		fclose(f);
	}

	// трек ещё качается: отдаём то, что уже есть, и ждём новые данные
	void ServeLive(mbedtls_net_context* c, const std::string& id, const std::shared_ptr<Job>& job)
	{
		const std::string path = Mp3Path(id);
		FILE* f = nullptr;
		for (int i = 0; i < 600 && !(f = fopen(path.c_str(), "rb")); i++)
		{
			if (job->done) break;
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
		if (!f) { SendAll(c, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"); return; }
		if (SendAll(c, "HTTP/1.1 200 OK\r\nContent-Type: audio/mpeg\r\nConnection: close\r\n\r\n"))
		{
			char buf[16384];
			int idle = 0;
			for (;;)
			{
				size_t n = fread(buf, 1, sizeof(buf), f);
				if (n > 0)
				{
					idle = 0;
					if (!SendAll(c, buf, n)) break;
					continue;
				}
				clearerr(f);
				if (job->done && (n = fread(buf, 1, sizeof(buf), f)) == 0) break;
				if (n > 0) { if (!SendAll(c, buf, n)) break; continue; }
				if (++idle > 1200) break; // минута без новых данных
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
			}
		}
		fclose(f);
	}

	// --- Twitch (kind: 0 - живой канал, 1 - VOD, 2 - клип)

	bool ValidTwitchChannel(const std::string& s)
	{
		if (s.empty() || s.size() > 30) return false;
		for (char ch : s) if (!isalnum((unsigned char)ch) && ch != '_') return false;
		return true;
	}
	bool ValidDigits(const std::string& s)
	{
		if (s.empty() || s.size() > 20) return false;
		for (char ch : s) if (!isdigit((unsigned char)ch)) return false;
		return true;
	}
	bool ValidSlug(const std::string& s)
	{
		if (s.empty() || s.size() > 100) return false;
		for (char ch : s) if (!isalnum((unsigned char)ch) && ch != '-' && ch != '_') return false;
		return true;
	}
	bool ValidTwitch(int kind, const std::string& ident)
	{
		return kind == 0 ? ValidTwitchChannel(ident) : kind == 1 ? ValidDigits(ident) : ValidSlug(ident);
	}

	// прямой URL потока (HLS .m3u8 для канала/VOD, mp4 для клипа) через yt-dlp
	// <=720p: сегменты меньше - легче гнать через наш прокси
	std::string ResolveTwitch(int kind, const std::string& ident)
	{
		const std::string page = kind == 0 ? "https://www.twitch.tv/" + ident
			: kind == 1 ? "https://www.twitch.tv/videos/" + ident
			: "https://clips.twitch.tv/" + ident;
		const std::string out = RunRead("\"" + Tool("yt-dlp") + "\" -q --no-warnings -g -f \"best[height<=720]/best\" \"" + page + "\"");
		std::string link = out.substr(0, out.find('\n'));
		while (!link.empty() && (isspace((unsigned char)link.back()))) link.pop_back();
		return SafeUrl(link) ? link : ""; // дальше идёт в командную строку ffmpeg
	}

	std::string UrlEncode(const std::string& s)
	{
		static const char* h = "0123456789ABCDEF";
		std::string o;
		for (unsigned char c : s)
		{
			if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') o += (char)c;
			else { o += '%'; o += h[c >> 4]; o += h[c & 15]; }
		}
		return o;
	}

	// живой канал: берём токен у Twitch GQL как официальный плеер (playerType=embed) и собираем
	// usher-плейлист сами. Так Twitch отдаёт сам эфир, а не заставку "Commercial break"/рекламу
	// "" - не вышло (тогда откат на yt-dlp)
	std::string ResolveTwitchLive(const std::string& channel)
	{
		const std::string body =
			"{\"operationName\":\"PlaybackAccessToken\",\"variables\":{\"isLive\":true,\"login\":\"" + channel +
			"\",\"isVod\":false,\"vodID\":\"\",\"playerType\":\"embed\"},"
			"\"extensions\":{\"persistedQuery\":{\"version\":1,"
			"\"sha256Hash\":\"0828119ded1c13477966434e15800ff57ddacf13ba1911c129dc2200705b0712\"}}}";
		std::string resp;
		if (Http::Post("gql.twitch.tv", "/gql", "application/json", body,
			"Client-ID: kimne78kx3ncx6brgo4mv6wki5h1ko\r\n", resp) != 200)
			return "";

		std::string value, sig;
		try
		{
			auto j = nlohmann::json::parse(resp);
			auto& t = j.at("data").at("streamPlaybackAccessToken");
			value = t.value("value", "");
			sig = t.value("signature", "");
		}
		catch (...) { return ""; }
		if (value.empty() || sig.empty()) return "";

		return "https://usher.ttvnw.net/api/channel/hls/" + channel + ".m3u8?allow_source=true&fast_bread=true"
			"&player_backend=mediaplayer&playlist_include_framerate=true&reassignments_supported=true"
			"&supported_codecs=h264&cdm=wv&player_version=1.0.0&sig=" + sig + "&token=" + UrlEncode(value) +
			"&p=" + std::to_string(std::rand() % 1000000);
	}

	// ---- HLS-прокси: манифест и сегменты Twitch через наш сервер (убирает CORS в плеере)

	std::string B64UrlEncode(const std::string& in)
	{
		static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
		std::string out;
		int val = 0, bits = -6;
		for (unsigned char c : in)
		{
			val = (val << 8) + c; bits += 8;
			while (bits >= 0) { out += t[(val >> bits) & 0x3F]; bits -= 6; }
		}
		if (bits > -6) out += t[((val << 8) >> (bits + 8)) & 0x3F];
		return out;
	}
	std::string B64UrlDecode(const std::string& in)
	{
		static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
		std::string out;
		int val = 0, bits = -8;
		for (char c : in)
		{
			if (c == 0) break;
			const char* p = strchr(t, c);
			if (!p) break;
			val = (val << 6) + int(p - t); bits += 6;
			if (bits >= 0) { out += char((val >> bits) & 0xFF); bits -= 8; }
		}
		return out;
	}

	// https://host/path?query -> host, "/path?query"
	bool SplitUrl(const std::string& url, std::string& host, std::string& path)
	{
		if (url.compare(0, 8, "https://") != 0) return false;
		const size_t slash = url.find('/', 8);
		if (slash == std::string::npos) { host = url.substr(8); path = "/"; }
		else { host = url.substr(8, slash - 8); path = url.substr(slash); }
		const size_t colon = host.find(':');
		if (colon != std::string::npos) host = host.substr(0, colon);
		return !host.empty();
	}

	// хост - сам домен или его поддомен (".домен")
	// и "eviltwitch.tv" проходил как "twitch.tv"
	bool HostIs(const std::string& h, const char* domain)
	{
		const size_t n = strlen(domain);
		if (h.size() == n) return h == domain;
		return h.size() > n && h[h.size() - n - 1] == '.' && h.compare(h.size() - n, n, domain) == 0;
	}

	std::string Lower(std::string s)
	{
		for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		return s;
	}

	// CloudFront - общий CDN (поддомен там заводит кто угодно), поэтому его хосты пускаем, только если
	// они встретились в манифесте, который прокси уже получил с домена Twitch
	std::mutex learnedMx;
	std::set<std::string> learnedHosts;

	void LearnHost(const std::string& url)
	{
		std::string host, path;
		if (!SplitUrl(url, host, path)) return;
		host = Lower(host);
		if (!HostIs(host, "cloudfront.net")) return;
		std::lock_guard<std::mutex> lock(learnedMx);
		if (learnedHosts.size() < 256) learnedHosts.insert(host);
	}

	// только домены Twitch/его CDN - чтобы прокси нельзя было использовать как открытый
	bool TwitchHostAllowed(const std::string& host)
	{
		const std::string h = Lower(host);
		static const char* ok[] = { "ttvnw.net", "twitch.tv", "twitchcdn.net", "twitchsvc.net" };
		for (const char* s : ok) if (HostIs(h, s)) return true;
		std::lock_guard<std::mutex> lock(learnedMx);
		return learnedHosts.count(h) != 0;
	}

	// ссылку ref из манифеста привести к абсолютной относительно base
	std::string ResolveUrl(const std::string& base, const std::string& ref)
	{
		if (ref.empty() || ref.compare(0, 7, "http://") == 0 || ref.compare(0, 8, "https://") == 0) return ref;
		const size_t scheme = base.find("://");
		const size_t hostStart = scheme == std::string::npos ? 0 : scheme + 3;
		if (ref[0] == '/')
		{
			const size_t slash = base.find('/', hostStart);
			return base.substr(0, slash == std::string::npos ? base.size() : slash) + ref;
		}
		const std::string b = base.substr(0, base.find('?'));
		const size_t slash = b.rfind('/');
		return (slash == std::string::npos ? b : b.substr(0, slash + 1)) + ref;
	}

	// все ссылки в m3u8 завернуть на наш прокси (origin = http://host:port)
	std::string RewriteManifest(const std::string& body, const std::string& baseUrl, const std::string& origin)
	{
		std::string out;
		size_t i = 0;
		while (i < body.size())
		{
			const size_t e = body.find('\n', i);
			std::string line = body.substr(i, e == std::string::npos ? std::string::npos : e - i);
			i = e == std::string::npos ? body.size() : e + 1;
			std::string nl = "\n";
			if (!line.empty() && line.back() == '\r') { line.pop_back(); nl = "\r\n"; }
			if (line.empty()) { out += nl; continue; }
			if (line[0] == '#')
			{
				const size_t u = line.find("URI=\"");
				if (u != std::string::npos)
				{
					const size_t s = u + 5;
					const size_t en = line.find('"', s);
					if (en != std::string::npos)
					{
						const std::string abs = ResolveUrl(baseUrl, line.substr(s, en - s));
						LearnHost(abs);
						line = line.substr(0, s) + origin + "/hp?u=" + B64UrlEncode(abs) + line.substr(en);
					}
				}
				out += line + nl;
			}
			else
			{
				const std::string abs = ResolveUrl(baseUrl, line);
				LearnHost(abs);
				out += origin + "/hp?u=" + B64UrlEncode(abs) + nl;
			}
		}
		return out;
	}

	void ServeProxy(mbedtls_net_context* c, const std::string& b64)
	{
		const std::string target = B64UrlDecode(b64);
		std::string host, path;
		if (!SplitUrl(target, host, path) || !TwitchHostAllowed(host))
		{
			SendAll(c, "HTTP/1.1 403 Forbidden\r\nConnection: close\r\n\r\n");
			return;
		}
		std::string body;
		if (Http::Get(host, path, body) != 200)
		{
			SendAll(c, "HTTP/1.1 502 Bad Gateway\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n");
			return;
		}
		std::string origin;
		{
			std::lock_guard<std::mutex> lock(hostMx);
			origin = "http://" + serverHost + ":" + std::to_string(serverPort);
		}
		std::string ctype, out;
		if (body.compare(0, 7, "#EXTM3U") == 0 || target.find(".m3u8") != std::string::npos)
		{
			out = RewriteManifest(body, target, origin);
			ctype = "application/vnd.apple.mpegurl";
		}
		else
		{
			out.swap(body);
			ctype = target.find(".ts") != std::string::npos ? "video/mp2t"
				: target.find(".mp4") != std::string::npos ? "video/mp4" : "application/octet-stream";
		}
		const std::string head = "HTTP/1.1 200 OK\r\nContent-Type: " + ctype +
			"\r\nAccess-Control-Allow-Origin: *\r\nCache-Control: no-cache\r\nContent-Length: " +
			std::to_string(out.size()) + "\r\nConnection: close\r\n\r\n";
		if (SendAll(c, head)) SendAll(c, out);
	}

	// hls.js держим у себя: грузить его с внешнего CDN прямо в CEF Arizona ненадёжно
	// (внешний домен может быть заблокирован - тогда чёрный экран). Качаем один раз и отдаём с /hls.js
	std::mutex hlsMx;
	std::string hlsJs;
	bool hlsTried = false;
	// SHA-256 hls.min.js 1.5.17 (одинаковый на jsDelivr и unpkg). Файл уходит в CEF всех игроков
	// а качается без проверки сертификата - подменённый по дороге не принимаем
	constexpr char HLS_SHA256[] = "484054e8cd03d3f6d1781fb7f402bdc318d8a4c527f933a95c624e27cc9a9470";

	bool HlsGenuine(const std::string& body)
	{
		unsigned char hash[32];
		mbedtls_sha256(reinterpret_cast<const unsigned char*>(body.data()), body.size(), hash, 0);
		static const char digits[] = "0123456789abcdef";
		std::string hex;
		for (unsigned char c : hash) { hex += digits[c >> 4]; hex += digits[c & 15]; }
		return hex == HLS_SHA256;
	}

	const std::string& EnsureHlsJs()
	{
		std::lock_guard<std::mutex> lock(hlsMx);
		if (!hlsTried)
		{
			hlsTried = true;
			std::string body;
			if (Http::Get("cdn.jsdelivr.net", "/npm/hls.js@1.5.17/dist/hls.min.js", body) == 200 && HlsGenuine(body))
				hlsJs = body;
			else if (Http::Get("unpkg.com", "/hls.js@1.5.17/dist/hls.min.js", body) == 200 && HlsGenuine(body))
				hlsJs = body;
			else
				Platform::Log(Platform::LogLevel::Warning, "[ModGear]: Медиа: не удалось скачать hls.js (Twitch-видео может не играть)");
		}
		return hlsJs;
	}

	// JS-строка в кавычках
	std::string JsEscape(const std::string& s)
	{
		std::string out;
		for (char ch : s)
		{
			if (ch == '\\' || ch == '"') { out += '\\'; out += ch; }
			else if (ch == '<') out += "\\u003c";
			else if (ch == '>') out += "\\u003e";
			else if ((unsigned char)ch >= 0x20) out += ch;
		}
		return out;
	}

	// HTML-страница с hls.js: играет прямой поток Twitch, минуя его встроенный плеер
	// Страница всегда рисует фон и строку статуса - если видео не пошло, на экране видно почему
	void ServeTwitch(mbedtls_net_context* c, int kind, const std::string& ident)
	{
		std::string origin;
		{
			std::lock_guard<std::mutex> lock(hostMx);
			origin = "http://" + serverHost + ":" + std::to_string(serverPort);
		}
		const std::string u = JsEscape(origin + "/twl/" + std::to_string(kind) + "/" + ident + "?q=" + std::to_string(videoQuality));
		const std::string html =
			"<!doctype html><html><head><meta charset=\"windows-1251\">"
			"<style>html,body{margin:0;background:#101014;height:100%;overflow:hidden;font-family:sans-serif}"
			"video{position:absolute;inset:0;width:100%;height:100%;object-fit:contain;background:#000}"
			"#s{position:absolute;left:12px;top:10px;color:#9ad;font-size:20px;z-index:2;"
			"text-shadow:0 1px 3px #000}</style></head>"
			"<body><video id=\"v\" autoplay playsinline></video><div id=\"s\">ModGear: Twitch, запуск потока...</div>"
			"<script>var u=\"" + u + "\",v=document.getElementById('v'),S=document.getElementById('s');"
			"function st(t){S.textContent=t;}"
			"v.addEventListener('playing',function(){st('');});"
			"v.addEventListener('error',function(){st('Поток не получен (yt-dlp/ffmpeg на сервере?)');});"
			"v.src=u;v.muted=false;var p=v.play();if(p&&p.catch)p.catch(function(){v.muted=true;v.play();});"
			"</script></body></html>";
		const std::string head = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=windows-1251\r\n"
			"Content-Length: " + std::to_string(html.size()) + "\r\nConnection: close\r\n\r\n";
		if (SendAll(c, head)) SendAll(c, html);
	}

	// Перекодирование потока Twitch в WebM/VP8 на лету и отдача клиенту (ffmpeg тянет HLS сам
	// без CORS). height - высота кадра (480/720/1080); больше = чётче, но нагрузка на CPU выше
	void ServeTwitchStream(mbedtls_net_context* c, int kind, const std::string& ident, int height)
	{
		std::string src = kind == 0 ? ResolveTwitchLive(ident) : "";
		if (src.empty()) src = ResolveTwitch(kind, ident);
		if (src.empty())
		{
			SendAll(c, "HTTP/1.1 502 Bad Gateway\r\nConnection: close\r\n\r\n");
			Platform::Log(Platform::LogLevel::Warning, "[ModGear]: Медиа: не удалось получить поток Twitch %s (нужен yt-dlp рядом с сервером)", ident.c_str());
			return;
		}
		// чем выше качество, тем больше битрейт и тяжелее кодирование (cpu-used меньше = лучше/дороже)
		const int bitrate = height >= 1080 ? 5000 : height >= 720 ? 3000 : 1500;
		const int cpuUsed = height >= 1080 ? 4 : height >= 720 ? 5 : 8;
		DebugLog::Write("[ModGear]: Медиа: Twitch (%s) -> перекодирование в WebM %dp", ident.c_str(), height);

		char vf[160];
		snprintf(vf, sizeof(vf), " -vf scale=-2:%d -c:v libvpx -b:v %dk -deadline realtime -cpu-used %d -g 60 -auto-alt-ref 0 ",
			height, bitrate, cpuUsed);
		// -af dynaudnorm: авто-нормализация громкости (тихие стримы становятся громче, без клиппинга)
		const std::string cmd = "\"" + Tool("ffmpeg") + "\" -hide_banner -loglevel error -fflags nobuffer -i \"" + src +
			"\"" + vf + "-af dynaudnorm=f=200:g=11 -c:a libopus -b:a 160k -f webm -live 1 -flush_packets 1 pipe:1";
		FILE* p = OpenPipe(cmd);
		if (!p)
		{
			SendAll(c, "HTTP/1.1 502 Bad Gateway\r\nConnection: close\r\n\r\n");
			return;
		}
		if (SendAll(c, "HTTP/1.1 200 OK\r\nContent-Type: video/webm\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n"))
		{
			char b[32768];
			size_t n;
			while ((n = fread(b, 1, sizeof(b), p)) > 0)
				if (!SendAll(c, b, n)) break; // клиент отключился - ffmpeg закроется
		}
		pclose(p);
	}

	// ---- потоки клиентов медиасервера
	// Поток на соединение (слушатель радио/видео держит его весь трек). SA-MP - 32-битный процесс
	constexpr unsigned CLIENT_STACK = 128 * 1024;
	constexpr int MAX_CLIENTS = 1200;
	constexpr int HEADER_DEADLINE_MS = 5000;
	std::atomic<int> activeClients{ 0 };

	void HandleClient(mbedtls_net_context client);

	struct ClientArg { mbedtls_net_context client; };

	void RunClient(ClientArg* arg)
	{
		const mbedtls_net_context client = arg->client;
		delete arg;
		HandleClient(client);
		activeClients.fetch_sub(1, std::memory_order_relaxed);
	}

#ifdef _WIN32
	unsigned __stdcall ClientThread(void* p) { RunClient(static_cast<ClientArg*>(p)); return 0; }
#else
	void* ClientThread(void* p) { RunClient(static_cast<ClientArg*>(p)); return nullptr; }
#endif

	// поток со стеком CLIENT_STACK; false - создать не удалось (client не тронут)
	bool SpawnClient(const mbedtls_net_context& client)
	{
		ClientArg* arg = new ClientArg{ client };
#ifdef _WIN32
		const uintptr_t h = _beginthreadex(nullptr, CLIENT_STACK, ClientThread, arg, STACK_SIZE_PARAM_IS_A_RESERVATION, nullptr);
		if (h) { CloseHandle(reinterpret_cast<HANDLE>(h)); return true; }
#else
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setstacksize(&attr, CLIENT_STACK);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_t t;
		const bool ok = pthread_create(&t, &attr, ClientThread, arg) == 0;
		pthread_attr_destroy(&attr);
		if (ok) return true;
#endif
		delete arg;
		return false;
	}

	void RejectBusy(mbedtls_net_context& client)
	{
		static const char busy[] = "HTTP/1.1 503 Service Unavailable\r\nRetry-After: 2\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
		mbedtls_net_send(&client, reinterpret_cast<const unsigned char*>(busy), sizeof(busy) - 1);
		mbedtls_net_free(&client);
	}

	void HandleClient(mbedtls_net_context client)
	{
		mbedtls_net_context* c = &client;
		std::string request;
		unsigned char buf[2048];
		// дедлайн на весь заголовок, а не на каждый recv: клиент, шлющий по байту, держал поток
		// до 8192 x 5 с
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(HEADER_DEADLINE_MS);
		while (request.find("\r\n\r\n") == std::string::npos && request.size() < 8192)
		{
			const auto left = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - std::chrono::steady_clock::now()).count();
			if (left <= 0) break;
			int r = mbedtls_net_recv_timeout(c, buf, sizeof(buf), (uint32_t)left);
			if (r <= 0) break;
			request.append((const char*)buf, (size_t)r);
		}

		// путь-токен после префикса (до пробела/?/слеша)
		auto token = [&](const char* prefix) -> std::string
		{
			const size_t pl = strlen(prefix);
			if (request.compare(0, pl, prefix) != 0) return "";
			const size_t e = request.find_first_of(" ?/\r\n", pl);
			return request.substr(pl, e == std::string::npos ? std::string::npos : e - pl);
		};

		// WebM-поток Twitch (перекодирование): /twl/<kind>/<ident>
		if (request.compare(0, 9, "GET /twl/") == 0 && request.size() > 11 && request[10] == '/')
		{
			const int kind = request[9] - '0';
			const size_t is = 11;
			const size_t e = request.find_first_of(" ?/\r\n", is);
			const std::string ident = request.substr(is, e == std::string::npos ? std::string::npos : e - is);
			int height = 720; // по умолчанию 720p; ?q=480/720/1080 - сменить
			const size_t q = request.find("?q=", is);
			if (q != std::string::npos && q < request.find("\r\n"))
			{
				const int v = atoi(request.c_str() + q + 3);
				if (v == 480 || v == 720 || v == 1080) height = v;
			}
			if (kind >= 0 && kind <= 2 && ValidTwitch(kind, ident)) ServeTwitchStream(c, kind, ident, height);
			else SendAll(c, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
			mbedtls_net_free(c);
			return;
		}

		// HLS-прокси Twitch: /hp?u=<base64url полного URL>
		if (request.compare(0, 10, "GET /hp?u=") == 0)
		{
			const size_t s = 10;
			const size_t e = request.find_first_of(" &\r\n", s);
			ServeProxy(c, request.substr(s, e == std::string::npos ? std::string::npos : e - s));
			mbedtls_net_free(c);
			return;
		}

		// наш hls.js (чтобы не грузить с внешнего CDN в CEF)
		if (request.compare(0, 12, "GET /hls.js ") == 0)
		{
			const std::string& js = EnsureHlsJs();
			if (js.empty()) SendAll(c, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
			else
			{
				const std::string head = "HTTP/1.1 200 OK\r\nContent-Type: application/javascript\r\n"
					"Cache-Control: max-age=86400\r\nContent-Length: " + std::to_string(js.size()) + "\r\nConnection: close\r\n\r\n";
				if (SendAll(c, head)) SendAll(c, js);
			}
			mbedtls_net_free(c);
			return;
		}

		// Twitch: /tw/<канал>, /twv/<id VOD>, /twc/<слаг клипа>
		std::string t;
		if (!(t = token("GET /twv/")).empty()) { if (ValidDigits(t)) ServeTwitch(c, 1, t); else SendAll(c, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"); mbedtls_net_free(c); return; }
		if (!(t = token("GET /twc/")).empty()) { if (ValidSlug(t)) ServeTwitch(c, 2, t); else SendAll(c, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"); mbedtls_net_free(c); return; }
		if (!(t = token("GET /tw/")).empty()) { if (ValidTwitchChannel(t)) ServeTwitch(c, 0, t); else SendAll(c, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n"); mbedtls_net_free(c); return; }

		// GET /yt/<id>.mp3
		std::string id;
		if (request.compare(0, 8, "GET /yt/") == 0)
		{
			size_t end = request.find(".mp3", 8);
			if (end != std::string::npos) id = request.substr(8, end - 8);
		}
		if (!ValidId(id)) SendAll(c, "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
		else
		{
			if (Cached(id)) ServeCached(c, id, request);
			else
			{
				auto job = EnsureJob(id);
				if (job) ServeLive(c, id, job);
				else ServeCached(c, id, request);
			}
		}
		mbedtls_net_free(c);
	}

	void DetectHost()
	{
		std::string body;
		unsigned a = 0, b = 0, c = 0, d = 0;
		char tail;
		if (Http::Get("api.ipify.org", "/", body) == 200)
			while (!body.empty() && isspace((unsigned char)body.back())) body.pop_back();
		else body.clear();
		if (!body.empty() && body.size() <= 15 && sscanf(body.c_str(), "%u.%u.%u.%u%c", &a, &b, &c, &d, &tail) == 4 &&
			a < 256 && b < 256 && c < 256 && d < 256)
		{
			std::lock_guard<std::mutex> lock(hostMx);
			if (serverHost.empty()) serverHost = body;
			Platform::Log(Platform::LogLevel::Message, "[ModGear]: Медиасервер: http://%s:%d", body.c_str(), serverPort);
		}
		else Platform::Log(Platform::LogLevel::Warning, "[ModGear]: Медиасервер не смог узнать внешний IP. Укажите его в Media_Init.");
	}
}

bool Media::Start(int port, const std::string& host, int videoHeight)
{
	if (running) return true;
	char portText[16];
	snprintf(portText, sizeof(portText), "%d", port);
	mbedtls_net_init(&listenCtx);
	// Привязка явно к IPv4 (0.0.0.0): с nullptr на Windows сокет садится на IPv6 (::)
	// а он там по умолчанию не принимает IPv4, и клиент SA-MP (IPv4) до порта не достучится
	if (mbedtls_net_bind(&listenCtx, "0.0.0.0", portText, MBEDTLS_NET_PROTO_TCP) != 0)
	{
		Platform::Log(Platform::LogLevel::Warning, "[ModGear]: Медиасервер не запустился: порт %d занят.", port);
		return false;
	}
	serverPort = port;
	videoQuality = (videoHeight == 480 || videoHeight == 1080) ? videoHeight : 720;
	running = true;
	MakeCacheDir();
	PruneCache(); // при старте подчистить кэш, если разросся
	std::string known = host;
	std::string bind = Platform::GetBindAddress();
	if (bind == "localhost") bind = "127.0.0.1";
	// локальный сервер (bind 127.x) - игра на том же ПК, ссылку даём на localhost
	// (внешний IP изнутри своей сети обычно недоступен из-за NAT), иначе - внешний IP
	if (known.empty() && bind.compare(0, 4, "127.") == 0) known = "127.0.0.1";
	// bind из server.cfg задан - ссылки на тот же IP, что и у сервера ("Started server on ...")
	if (known.empty() && !bind.empty() && bind != "0.0.0.0") known = bind;
	{
		std::lock_guard<std::mutex> lock(hostMx);
		serverHost = known;
	}
	if (known.empty()) std::thread(DetectHost).detach();
	else Platform::Log(Platform::LogLevel::Message, "[ModGear]: Медиасервер: http://%s:%d", known.c_str(), port);
	// запуск внешних программ - сотни мс; в фоне, чтобы не держать OnGameModeInit (и gmx)
	std::thread([]()
	{
		if (RunRead("\"" + Tool("yt-dlp") + "\" --version").empty())
			Platform::Log(Platform::LogLevel::Warning, "[ModGear]: Не найден yt-dlp: положите yt-dlp рядом с сервером, иначе ссылки YouTube в Audio_ не заиграют.");
		if (RunRead("\"" + Tool("ffmpeg") + "\" -version").empty())
			Platform::Log(Platform::LogLevel::Warning, "[ModGear]: Не найден ffmpeg: положите ffmpeg рядом с сервером, иначе ссылки YouTube в Audio_ не заиграют.");
	}).detach();
	std::thread([]()
	{
		while (running)
		{
			mbedtls_net_context client;
			mbedtls_net_init(&client);
			if (mbedtls_net_accept(&listenCtx, &client, nullptr, 0, nullptr) != 0)
			{
				mbedtls_net_free(&client);
				if (!running) break;
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}
			// лимит соединений: сверх него - сразу 503, поток не создаём
			if (activeClients.fetch_add(1, std::memory_order_relaxed) >= MAX_CLIENTS || !SpawnClient(client))
			{
				activeClients.fetch_sub(1, std::memory_order_relaxed);
				RejectBusy(client);
			}
		}
	}).detach();
	return true;
}

void Media::Stop()
{
	if (!running) return;
	running = false;
	mbedtls_net_free(&listenCtx);
}

bool Media::Running() { return running; }

std::string Media::PublicIp()
{
	static std::string ip;
	static std::once_flag once;
	std::call_once(once, []
	{
		std::string body;
		unsigned a = 0, b = 0, c = 0, d = 0;
		char tail;
		if (Http::Get("api.ipify.org", "/", body, 5000) != 200) return;
		while (!body.empty() && isspace((unsigned char)body.back())) body.pop_back();
		if (body.size() <= 15 && sscanf(body.c_str(), "%u.%u.%u.%u%c", &a, &b, &c, &d, &tail) == 4 && a < 256 && b < 256 && c < 256 && d < 256)
			ip = body;
	});
	return ip;
}

bool Media::IsSafeUrl(const std::string& url) { return SafeUrl(url); }

std::string Media::YouTubeId(const std::string& url)
{
	static const char* keys[] = { "youtube.com/watch?v=", "youtube.com/watch?", "youtu.be/", "youtube.com/shorts/", "youtube.com/embed/", "youtube.com/live/", "video_id=" };
	for (const char* key : keys)
	{
		size_t p = url.find(key);
		if (p == std::string::npos) continue;
		p += strlen(key);
		if (!strcmp(key, "youtube.com/watch?"))
		{
			size_t v = url.find("v=", p);
			if (v == std::string::npos) continue;
			p = v + 2;
		}
		std::string id = url.substr(p, 11);
		if (ValidId(id)) return id;
	}
	return "";
}

std::string Media::AudioUrl(const std::string& url)
{
	const std::string id = YouTubeId(url);
	if (id.empty()) return url;
	std::lock_guard<std::mutex> lock(hostMx);
	if (!running || serverHost.empty()) return "";
	return "http://" + serverHost + ":" + std::to_string(serverPort) + "/yt/" + id + ".mp3";
}

std::string Media::TwitchUrl(const std::string& url)
{
	if (url.find("twitch.tv/") == std::string::npos) return "";

	// часть ссылки после ключа до разделителя
	auto after = [&](const char* key) -> std::string
	{
		size_t p = url.find(key);
		if (p == std::string::npos) return "";
		p += strlen(key);
		size_t e = url.find_first_of("&?#/", p);
		return url.substr(p, e == std::string::npos ? std::string::npos : e - p);
	};

	int kind = -1;
	std::string ident;
	std::string s;
	if (!(s = after("clips.twitch.tv/")).empty() && s != "embed") { kind = 2; ident = s; }
	else if (!(s = after("/clip/")).empty()) { kind = 2; ident = s; }
	else if (!(s = after("/videos/")).empty()) { kind = 1; ident = s; }
	else if (!(s = after("twitch.tv/")).empty() && s != "videos") { kind = 0; ident = s; }

	if (kind < 0 || !ValidTwitch(kind, ident)) return "";

	std::lock_guard<std::mutex> lock(hostMx);
	if (!running || serverHost.empty()) return "";
	const char* path = kind == 1 ? "/twv/" : kind == 2 ? "/twc/" : "/tw/";
	return "http://" + serverHost + ":" + std::to_string(serverPort) + path + ident;
}

void Media::Prefetch(const std::string& youtubeId)
{
	if (running && ValidId(youtubeId)) EnsureJob(youtubeId);
}

int Media::Duration(const std::string& url)
{
	const int ms = DurationMs(url);
	return ms > 0 ? (ms + 999) / 1000 : -1;
}

int Media::DurationMs(const std::string& url)
{
	const std::string id = YouTubeId(url);
	const std::string key = id.empty() ? url : "yt:" + id;
	{
		std::lock_guard<std::mutex> lock(durMx);
		auto it = durations.find(key);
		if (it != durations.end()) return it->second > 0 ? it->second : -1;
		if (pending.count(key)) return -1;
		if (id.empty() && !SafeUrl(url)) { durations[key] = 0; return -1; }
		pending.insert(key);
	}
	std::thread([key, id, url]()
	{
		std::string out;
		if (!id.empty())
		{
			if (Cached(id)) out = RunRead("\"" + Tool("ffprobe") + "\" -v quiet -show_entries format=duration -of csv=p=0 \"" + Mp3Path(id) + "\"");
			if (atof(out.c_str()) <= 0) out = RunRead("\"" + Tool("yt-dlp") + "\" --no-warnings --skip-download --print duration \"https://www.youtube.com/watch?v=" + id + "\"");
		}
		else out = RunRead("\"" + Tool("ffprobe") + "\" -v quiet -show_entries format=duration -of csv=p=0 \"" + url + "\"");
		double seconds = atof(out.c_str());
		std::lock_guard<std::mutex> lock(durMx);
		durations[key] = seconds > 0 ? (int)std::lround(seconds * 1000.0) : 0;
		pending.erase(key);
	}).detach();
	return -1;
}

// название видео/трека по ссылке (yt-dlp --print title). "" - ещё узнаём или нет названия
// Запрос уходит в фон при первом вызове; потом отдаётся из памяти
std::string Media::Title(const std::string& url)
{
	{
		std::lock_guard<std::mutex> lock(titleMx);
		auto it = titles.find(url);
		if (it != titles.end()) return it->second;
		if (titlePending.count(url)) return "";
		if (!SafeUrl(url)) { titles[url] = ""; return ""; }
		titlePending.insert(url);
	}
	std::thread([url]()
	{
		std::string out = RunRead("\"" + Tool("yt-dlp") + "\" --no-warnings --no-playlist --skip-download --print \"%(title)s\" \"" + url + "\"");
		std::string title = out.substr(0, out.find('\n'));
		while (!title.empty() && (title.back() == '\r' || isspace((unsigned char)title.back()))) title.pop_back();
		if (title == "NA") title.clear();
		std::lock_guard<std::mutex> lock(titleMx);
		titles[url] = title;
		titlePending.erase(url);
	}).detach();
	return "";
}
