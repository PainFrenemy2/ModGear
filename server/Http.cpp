#include "Http.h"

#include <cstring>
#include <cstdlib>
#include <cctype>

#include <mbedtls/net_sockets.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/x509_crt.h>

#include <chrono>
#include <mutex>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "Platform.h"

namespace
{
	struct Tls
	{
		mbedtls_net_context net;
		mbedtls_ssl_context ssl;
		mbedtls_ssl_config conf;
		mbedtls_entropy_context entropy;
		mbedtls_ctr_drbg_context drbg;

		Tls()
		{
			mbedtls_net_init(&net);
			mbedtls_ssl_init(&ssl);
			mbedtls_ssl_config_init(&conf);
			mbedtls_entropy_init(&entropy);
			mbedtls_ctr_drbg_init(&drbg);
		}
		~Tls()
		{
			mbedtls_ssl_close_notify(&ssl);
			mbedtls_net_free(&net);
			mbedtls_ssl_free(&ssl);
			mbedtls_ssl_config_free(&conf);
			mbedtls_ctr_drbg_free(&drbg);
			mbedtls_entropy_free(&entropy);
		}
	};

#ifdef _WIN32
	using Socket = SOCKET;
	const Socket BAD_SOCKET = INVALID_SOCKET;
	void CloseSocket(Socket s) { closesocket(s); }
	bool NetInit()
	{
		static const bool ok = [] { WSADATA wsa; return WSAStartup(MAKEWORD(2, 2), &wsa) == 0; }();
		return ok;
	}
	bool SetBlocking(Socket s, bool blocking) { u_long mode = blocking ? 0 : 1; return ioctlsocket(s, FIONBIO, &mode) == 0; }
	bool ConnectPending() { return WSAGetLastError() == WSAEWOULDBLOCK; }
#else
	using Socket = int;
	const Socket BAD_SOCKET = -1;
	void CloseSocket(Socket s) { close(s); }
	bool NetInit() { return true; }
	bool SetBlocking(Socket s, bool blocking)
	{
		const int flags = fcntl(s, F_GETFL, 0);
		if (flags < 0) return false;
		return fcntl(s, F_SETFL, blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK)) == 0;
	}
	bool ConnectPending() { return errno == EINPROGRESS; }
#endif

	bool ParseV4(const std::string& ip, sockaddr_in& addr)
	{
		std::memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		return inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) == 1;
	}

	// TCP по IPv4 к host:443 с адреса source (пусто - любой), ожидание не дольше timeoutMs
	// -2 - адрес source не принадлежит этой машине
	long long ConnectV4(const std::string& host, const std::string& source, int timeoutMs)
	{
		if (!NetInit()) return -1;
		sockaddr_in src;
		if (!source.empty() && !ParseV4(source, src)) return -2;

		addrinfo hints;
		std::memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		addrinfo* list = nullptr;
		if (getaddrinfo(host.c_str(), "443", &hints, &list) != 0) return -1;

		long long result = -1;
		for (addrinfo* a = list; a && result == -1; a = a->ai_next)
		{
			Socket s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (s == BAD_SOCKET) continue;
			if (!source.empty() && bind(s, (sockaddr*)&src, sizeof(src)) != 0)
			{
				CloseSocket(s);
				result = -2;
				break;
			}
			bool ok = false;
			if (SetBlocking(s, false))
			{
				if (connect(s, a->ai_addr, (int)a->ai_addrlen) == 0) ok = true;
				else if (ConnectPending())
				{
					fd_set w;
					FD_ZERO(&w);
					FD_SET(s, &w);
					timeval tv{ timeoutMs / 1000, (timeoutMs % 1000) * 1000 };
					if (select((int)s + 1, nullptr, &w, nullptr, &tv) == 1)
					{
						int err = 0;
						socklen_t len = sizeof(err);
						ok = getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&err, &len) == 0 && err == 0;
					}
				}
			}
			if (ok && SetBlocking(s, true)) result = (long long)s;
			else CloseSocket(s);
		}
		freeaddrinfo(list);
		return result;
	}

	// в строке запроса и заголовках - без управляющих символов (\r\n в path/host добавили бы свои заголовки)
	bool SafeField(const std::string& s)
	{
		for (unsigned char c : s) if (c < 0x20 || c == 0x7F) return false;
		return true;
	}

	using Clock = std::chrono::steady_clock;
	constexpr int EXCHANGE_LIMIT_MS = 60000; // весь обмен, вместе с рукопожатием
	constexpr int READ_TIMEOUT_MS = 10000;   // одно чтение

	// предел: свой (timeoutMs > 0) или обычный, но не больше обычного
	int Limit(int timeoutMs, int normal) { return timeoutMs > 0 && timeoutMs < normal ? timeoutMs : normal; }

	// TLS поверх уже открытого соединения: рукопожатие, запрос, разбор ответа. timeoutMs - на весь обмен (0 - обычный)
	int Exchange(Tls& tls, const std::string& host, const std::string& path, std::string& body, int timeoutMs,
		const char* method = "GET", const std::string& reqBody = "", const std::string& extraHeaders = "")
	{
		if (host.empty() || !SafeField(host) || !SafeField(path) || !SafeField(method)) return 0;
		const auto deadline = Clock::now() + std::chrono::milliseconds(Limit(timeoutMs, EXCHANGE_LIMIT_MS));
		static const char pers[] = "modgear";
		if (mbedtls_ctr_drbg_seed(&tls.drbg, mbedtls_entropy_func, &tls.entropy, (const unsigned char*)pers, sizeof(pers) - 1) != 0) return 0;
		if (mbedtls_ssl_config_defaults(&tls.conf, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT) != 0) return 0;

		mbedtls_ssl_conf_authmode(&tls.conf, MBEDTLS_SSL_VERIFY_NONE);
		mbedtls_ssl_conf_rng(&tls.conf, mbedtls_ctr_drbg_random, &tls.drbg);
		mbedtls_ssl_conf_read_timeout(&tls.conf, (uint32_t)Limit(timeoutMs, READ_TIMEOUT_MS));
		if (mbedtls_ssl_setup(&tls.ssl, &tls.conf) != 0) return 0;
		if (mbedtls_ssl_set_hostname(&tls.ssl, host.c_str()) != 0) return 0;
		mbedtls_ssl_set_bio(&tls.ssl, &tls.net, mbedtls_net_send, nullptr, mbedtls_net_recv_timeout);

		int ret;
		while ((ret = mbedtls_ssl_handshake(&tls.ssl)) != 0)
		{
			if (ret != MBEDTLS_ERR_SSL_WANT_READ && ret != MBEDTLS_ERR_SSL_WANT_WRITE) return 0;
			if (Clock::now() > deadline) return 0;
		}

		std::string request =
			std::string(method) + " " + path + " HTTP/1.1\r\n"
			"Host: " + host + "\r\n"
			"User-Agent: ModGear\r\n"
			"Accept: */*\r\n"
			"Cache-Control: no-cache\r\n" + extraHeaders;
		if (!reqBody.empty()) request += "Content-Length: " + std::to_string(reqBody.size()) + "\r\n";
		request += "Connection: close\r\n\r\n" + reqBody;
		size_t sent = 0;
		while (sent < request.size())
		{
			ret = mbedtls_ssl_write(&tls.ssl, (const unsigned char*)request.data() + sent, request.size() - sent);
			if (Clock::now() > deadline) return 0;
			if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
			if (ret <= 0) return 0;
			sent += ret;
		}

		std::string response;
		unsigned char buffer[4096];
		for (;;)
		{
			ret = mbedtls_ssl_read(&tls.ssl, buffer, sizeof(buffer));
			if (Clock::now() > deadline) return 0;
			if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) continue;
			if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY || ret == 0) break;
			if (ret < 0) break;
			response.append((const char*)buffer, ret);
			if (response.size() > 24 * 1024 * 1024) return 0; // хватает и под HLS-сегменты Twitch
		}

		// "HTTP/1.1 200 OK\r\n...\r\n\r\nbody"
		if (response.compare(0, 5, "HTTP/") != 0) return 0;
		const size_t space = response.find(' ');
		const size_t headerEnd = response.find("\r\n\r\n");
		if (space == std::string::npos || headerEnd == std::string::npos) return 0;
		const int status = std::atoi(response.c_str() + space + 1);
		std::string headers = response.substr(0, headerEnd);
		for (char& c : headers) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
		body = response.substr(headerEnd + 4);

		if (headers.find("transfer-encoding: chunked") != std::string::npos)
		{
			std::string decoded;
			size_t pos = 0;
			for (;;)
			{
				const size_t lineEnd = body.find("\r\n", pos);
				if (lineEnd == std::string::npos) return 0;
				char* end = nullptr;
				const unsigned long size = std::strtoul(body.c_str() + pos, &end, 16);
				if (end == body.c_str() + pos) return 0; // не hex
				pos = lineEnd + 2;
				if (size == 0) break;
				if (pos > body.size() || size > body.size() - pos) return 0; // без переполнения pos + size
				decoded.append(body, pos, size);
				pos += size + 2;
			}
			body.swap(decoded);
		}
		return status;
	}
}

// оставшееся время на обмен после соединения (0 - без общего предела)
static int Remaining(int timeoutMs, std::chrono::steady_clock::time_point start)
{
	if (timeoutMs <= 0) return 0;
	const long long left = timeoutMs - std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
	return left < 1 ? 1 : (int)left;
}

int Http::Get(const std::string& host, const std::string& path, std::string& body, int timeoutMs)
{
	body.clear();
	const auto start = std::chrono::steady_clock::now();
	const long long s = ConnectV4(host, "", Limit(timeoutMs, 10000));
	if (s < 0) return 0;
	Tls tls;
	tls.net.fd = (int)s;
	return Exchange(tls, host, path, body, Remaining(timeoutMs, start));
}

int Http::Post(const std::string& host, const std::string& path, const std::string& contentType,
	const std::string& reqBody, const std::string& extraHeaders, std::string& body)
{
	body.clear();
	if (!SafeField(contentType)) return 0;
	const long long s = ConnectV4(host, "", 10000);
	if (s < 0) return 0;
	Tls tls;
	tls.net.fd = (int)s;
	const std::string headers = "Content-Type: " + contentType + "\r\n" + extraHeaders;
	return Exchange(tls, host, path, body, 0, "POST", reqBody, headers);
}
