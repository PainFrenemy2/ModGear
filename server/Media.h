#pragma once

// Медиасервер внутри плагина
//  - YouTube -> mp3 для Audio_: yt-dlp + ffmpeg на машине сервера, mp3 раздаётся игрокам по HTTP

#include <string>

namespace Media
{
	// запустить HTTP на порту; host - внешний IP для ссылок (пусто - определить самому)
	// videoHeight - качество видео Twitch (480/720/1080), больше = чётче, но выше нагрузка на CPU
	bool Start(int port, const std::string& host, int videoHeight = 720);
	void Stop();
	bool Running();

	// внешний IP сервера (api.ipify.org), запрашивается один раз; "" - не удалось
	std::string PublicIp();

	// ссылка без мусора: http(s)://, 10..500 символов, без пробелов, кавычек и управляющих символов
	bool IsSafeUrl(const std::string& url);

	// id видео YouTube из любой ссылки (watch, youtu.be, shorts, embed, music) или "" если это не YouTube
	std::string YouTubeId(const std::string& url);

	// ссылка, которую можно отдать клиенту в Audio_: YouTube -> наш mp3, остальное как есть
	// "" - YouTube, но медиасервер не запущен
	std::string AudioUrl(const std::string& url);

	// Twitch-ссылка (канал / VOD / клип) -> страница нашего медиасервера с HLS-плеером
	// которая играет прямой поток Twitch в обход встроенного плеера (его CEF не тянет -> Error #4000)
	// "" - это не Twitch или медиасервер не запущен (тогда видео падает обратно на прямой плеер)
	std::string TwitchUrl(const std::string& url);

	// длительность в секундах: -1 - ещё считается или неизвестна (запрос уходит в фон при первом вызове)
	int Duration(const std::string& url);
	// то же в миллисекундах (для точного конца трека и повтора): -1 - ещё считается или неизвестна
	int DurationMs(const std::string& url);

	// название видео/трека по ссылке (YouTube/Twitch/прямая): "" - ещё узнаём или нет названия
	std::string Title(const std::string& url);

	// начать качать трек YouTube в кэш заранее
	void Prefetch(const std::string& youtubeId);
}
