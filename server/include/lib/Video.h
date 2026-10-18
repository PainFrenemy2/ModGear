#pragma once
// Видео на экране-объекте, как кинотеатр Arizona
// 220:12 - показать: u32 ширина, u32 высота, u32 0, u32 0, строка ссылка, строка user-agent

#include <lib/Native.h>
#include <lib/StrCompress.h>
#include "../../Platform.h"
#include "../../Plugin.h"
#include "../../Media.h"
#include <string>
#include <vector>
#include <bitset>
#include <chrono>
#include <cstring>

namespace Video
{
    struct Screen
    {
        bool used = false;
        int objectid = 0, model = 0;
        float dist = 40.0f;
        float x = 0, y = 0, z = 0;             // позиция экрана (плагин берёт сам по objectid)
        bool hasPos = false;                   // позиция известна - сервер сам гасит стрим далёким
        std::string source;                    // ссылка, как её дал скрипт
        std::string url;                       // ссылка для клиента
        bool playing = false;
        bool loop = true;                      // по кругу: в конце начать заново (false - остановить)
        int startAt = 0;                       // секунда, с которой включили
        int duration = 0;                      // длина в секундах: -1 - ещё считается, 0 - без конца
        std::chrono::steady_clock::time_point started;
        std::bitset<MAX_PLAYERS> shown;        // кому сейчас показано
        void* owner = nullptr;                 // скрипт, который создал экран: выгрузили (gmx) - экран удаляется
    };

    inline std::vector<Screen> screens;       // id видео = индекс + 1

    inline const char* USER_AGENT = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/152.0.0.0 Safari/537.36";
    inline const char* ARIZONA_YOUTUBE = "https://server-api.arizona.games/client/youtube/play?video_id=";

    inline Screen* Get(int id)
    {
        return id >= 1 && id <= (int)screens.size() && screens[id - 1].used ? &screens[id - 1] : nullptr;
    }

    inline std::string Param(const std::string& url, const char* key)
    {
        size_t p = url.find(key);
        if (p == std::string::npos) return "";

        p += strlen(key);
        size_t e = url.find_first_of("&?#/", p);

        return url.substr(p, e == std::string::npos ? std::string::npos : e - p);
    }

    // Twitch требует параметр parent = домену, где открыт плеер. Внутри CEF Arizona этот домен
    // заранее не известен, поэтому передаём сразу несколько parent - плеер запускается, если
    inline const char* TWITCH_TAIL =
        "&parent=twitch.tv&parent=www.twitch.tv&parent=localhost&parent=127.0.0.1"
        "&parent=arizona-rp.com&parent=arizona.games&autoplay=true&muted=false";

    // twitch.tv/канал | /videos/<id> (VOD) | clips.twitch.tv/<slug> или /clip/<slug> -> плеер Twitch
    inline std::string TwitchEmbed(const std::string& url)
    {
        if (url.find("twitch.tv/") == std::string::npos)
            return url;

        // уже готовый плеер - не трогаем
        if (url.find("player.twitch.tv") != std::string::npos ||
            url.find("clips.twitch.tv/embed") != std::string::npos)
            return url;

        // клип
        std::string clip = Param(url, "clips.twitch.tv/");
        if (clip.empty()) clip = Param(url, "/clip/");
        if (!clip.empty() && clip != "embed")
            return std::string("https://clips.twitch.tv/embed?clip=") + clip + TWITCH_TAIL;

        // VOD (запись трансляции)
        std::string vod = Param(url, "/videos/");
        if (!vod.empty())
            return std::string("https://player.twitch.tv/?video=") + vod + TWITCH_TAIL;

        // живой канал
        std::string channel = Param(url, "twitch.tv/");
        if (!channel.empty() && channel != "videos")
            return std::string("https://player.twitch.tv/?channel=") + channel + TWITCH_TAIL;

        return url;
    }

    // YouTube -> плеер Arizona; Twitch -> наш HLS-плеер (минуя сломанный плеер Twitch)
    // если медиасервер не запущен - запасной вариант через прямой плеер Twitch; остальное как есть
    inline std::string Normalize(const std::string& url)
    {
        std::string id = Media::YouTubeId(url);
        if (!id.empty())
            return ARIZONA_YOUTUBE + id;

        std::string twitch = Media::TwitchUrl(url);
        if (!twitch.empty())
            return twitch;

        return TwitchEmbed(url);
    }

    inline int Elapsed(const Screen& s)
    {
        return s.startAt +
               (int)std::chrono::duration_cast<std::chrono::seconds>(
                   std::chrono::steady_clock::now() - s.started
               ).count();
    }

    inline void WriteString(BitStream& bs, const std::string& text)
    {
        bs.Write((uint16_t)text.size());
        bs.Write((uint8_t)1);
        StrCompress::Write(bs, text);
    }

    inline void SendShow(int id, int playerid)
    {
        Screen& s = screens[id - 1];

        std::string url = s.url;
        int at = Elapsed(s);

        if (at > 0 && url.compare(0, strlen(ARIZONA_YOUTUBE), ARIZONA_YOUTUBE) == 0)
            url += "&start=" + std::to_string(at);

        BitStream bs;

        bs.Write((uint8_t)220);
        bs.Write((uint8_t)12);
        bs.Write((uint32_t)1440);
        bs.Write((uint32_t)810);
        bs.Write((uint32_t)0);
        bs.Write((uint32_t)0);

        WriteString(bs, url);
        WriteString(bs, USER_AGENT);

        bs.Write((uint16_t)s.objectid);
        bs.Write((uint16_t)s.model);
        bs.Write(s.dist);
        bs.Write((uint32_t)id);

        Native::MBSendPacket(playerid, &bs, 1, 9);

        s.shown.set(playerid);
    }

    inline void SendStop(int id, int playerid)
    {
        BitStream bs;

        bs.Write((uint8_t)220);
        bs.Write((uint8_t)14);
        bs.Write((uint32_t)id);

        Native::MBSendPacket(playerid, &bs, 1, 9);

        screens[id - 1].shown.reset(playerid);
    }

    inline void StopAll(int id)
    {
        Screen& s = screens[id - 1];

        for (int i : connectedPlayers)
        {
            if (i >= 0 && i < MAX_PLAYERS && s.shown.test(i))
                SendStop(id, i);
        }

        s.shown.reset();
        s.playing = false;
    }

    inline void Play(int id, const std::string& source, int start, bool loop)
    {
        Screen& s = screens[id - 1];

        StopAll(id);
        s.source = source;
        s.url = Normalize(source);
        s.loop = loop;
        s.startAt = start;
        s.started = std::chrono::steady_clock::now();
        s.duration = Media::YouTubeId(source).empty() ? 0 : Media::Duration(source); // 0 - без конца (стрим, страница)
        s.playing = true;
    }

    // перемотка: тем, кто смотрит, видео перезапускается с новой секунды
    inline void Seek(int id, int seconds)
    {
        Screen& s = screens[id - 1];

        s.startAt = seconds;
        s.started = std::chrono::steady_clock::now();

        for (int i : connectedPlayers)
        {
            if (i >= 0 && i < MAX_PLAYERS && s.shown.test(i))
            {
                SendStop(id, i);
                SendShow(id, i);
            }
        }
    }

    // позиции игроков на текущий тик: заполняются один раз в Update и переиспользуются
    // всеми экранами (без повторного GetPlayerPos на каждую пару экран-игрок)
    struct Pos { float x, y, z; bool ok; };
    inline Pos tickPos[MAX_PLAYERS];

    inline bool InRange(const Screen& s, int playerid)
    {
        const Pos& p = tickPos[playerid];
        if (!p.ok)
            return false;

        float dx = p.x - s.x;
        float dy = p.y - s.y;
        float dz = p.z - s.z;

        return dx * dx + dy * dy + dz * dz <= s.dist * s.dist;
    }

    // раз в секунду: включить тем, кто в радиусе, выключить (стоп 220:14) тем, кто вышел - чтобы у
    // далёкого игрока видео не стримилось в фоне. Если позиция экрана неизвестна (не нашли объект
    // напр. streamer) - шлём один раз всем с лаунчером, а радиус считает клиент по dist
    inline void Update()
    {
        // позиции нужны только если есть хоть один экран с известной позицией
        bool needPos = false;
        for (const Screen& s : screens)
            if (s.used && s.playing && s.hasPos) { needPos = true; break; }

        if (needPos)
            for (int i : connectedPlayers)
            {
                if (i < 0 || i >= MAX_PLAYERS) continue;
                Pos& p = tickPos[i];
                p.ok = PI[i].pLauncherStat && Platform::GetPlayerPos(i, p.x, p.y, p.z);
            }

        for (int id = 1; id <= (int)screens.size(); id++)
        {
            Screen& s = screens[id - 1];

            if (!s.used || !s.playing)
                continue;

            if (s.duration < 0)
                s.duration = Media::Duration(s.source);

            if (s.duration > 0 && Elapsed(s) >= s.duration)
            {
                if (s.loop)
                {
                    // по кругу: клиент сам ролик не повторяет (у смотрящих он просто заканчивался)
                    // перезапускаем с 0 у всех зрителей: сначала стоп, потом показ, чтобы не было второго
                    Seek(id, 0);
                }
                else
                {
                    StopAll(id);
                    Plugin::CallPublic("OnVideoFinished", id);
                }
                continue;
            }

            for (int i : connectedPlayers)
            {
                if (i < 0 || i >= MAX_PLAYERS)
                    continue;

                if (!PI[i].pLauncherStat)
                    continue;

                if (!s.hasPos)
                {
                    if (!s.shown.test(i))
                        SendShow(id, i);
                    continue;
                }

                bool isNear = InRange(s, i);

                if (isNear && !s.shown.test(i))
                    SendShow(id, i);
                else if (!isNear && s.shown.test(i))
                    SendStop(id, i);
            }
        }
    }

    // скрипт выгружен (gmx, unloadfs): его экраны гасятся у всех и удаляются - иначе старое видео
    // оставалось на экране у игроков, а новый мод создавал второй экран поверх
    inline void FreeOwner(void* owner)
    {
        for (int id = 1; id <= (int)screens.size(); id++)
        {
            if (!screens[id - 1].used || screens[id - 1].owner != owner)
                continue;

            StopAll(id);
            screens[id - 1] = Screen();
        }
    }

    inline void PlayerLeft(int playerid)
    {
        if (playerid < 0 || playerid >= MAX_PLAYERS)
            return;

        for (auto& s : screens)
            s.shown.reset(playerid);
    }
}
