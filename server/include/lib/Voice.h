#pragma once
// Голосовые каналы Arizona (RPC/PKT 252), разобрано по логам клиента
//   252:3  клиент->сервер (03 00 1C 09 00 ..) - рукопожатие после загрузки CEF, сервер отвечает 03 00 1A 09 00 00 00 00 00

#include <lib/Native.h>
#include "../../Platform.h"
#include "../../Afk.h"
#include <bitset>
#include <map>
#include <string>
#include <vector>

namespace Voice
{
    constexpr int MAX_CHANNELS = 250;          // id канала в пакетах - u8

    struct Member
    {
        int zone = 0;                          // uid зоны этого игрока в пакетах 252:10
        std::bitset<MAX_PLAYERS> shown;        // кому зона сейчас отправлена (кто его слышит)
    };

    struct Channel
    {
        bool used = false;
        std::string name;
        float radius = 20.0f;                  // 0 - общий канал (рация)
        std::map<int, Member> members;         // playerid -> зона
        int globalZone = 0;                    // рация: глобальная зона (тип 1), из неё играет голос
        std::bitset<MAX_PLAYERS> globalShown;  // кому глобальная зона отправлена
        void* owner = nullptr;                 // скрипт, который создал канал: выгрузили (gmx) - канал удаляется
    };

    inline std::vector<Channel> channels;     // uid канала = индекс + 1 = id канала в пакетах
    inline std::bitset<MAX_PLAYERS> muted;
    inline std::bitset<MAX_PLAYERS> inited;    // игроку уже отправлен 252:9 + своя зона
    inline std::bitset<MAX_PLAYERS> selfSent;  // игроку уже отправлены его зоны + категории + 252:19
    inline int selfZone[MAX_PLAYERS][2];       // 2 зоны на самом игроке (как у Arizona)
    constexpr int CHANNEL_BASE = 0x5A;         // id первого канала в пакетах - как "Локальный" у Arizona (90)
    inline int nextZone = 40000;               // uid голосовых зон, отдельно от аудио (там индекс + 1)
    inline int format = 0;                     // пересылка голоса: 0 - вкл (формат AZVoice), -1 - выкл
    inline bool echo = false;                  // слать говорящему его же голос (тест в одиночку)

    inline int PacketId(int uid) { return CHANNEL_BASE + uid - 1; }

    inline Channel* Get(int uid)
    {
        return uid >= 1 && uid <= (int)channels.size() && channels[uid - 1].used ? &channels[uid - 1] : nullptr;
    }

    inline bool ValidPlayer(int playerid)
    {
        return playerid >= 0 && playerid < MAX_PLAYERS && Platform::IsPlayerConnected(playerid);
    }

    inline int NewZone()
    {
        const int z = nextZone++;
        if (nextZone > 65000) nextZone = 40000;
        return z;
    }

    inline void SendPlayerZone(int zone, int target, int to)
    {
        BitStream bs;
        bs.Write((uint8_t)10);
        bs.Write((uint16_t)zone);
        bs.Write((uint32_t)0);
        bs.Write((uint16_t)0);
        bs.Write((uint8_t)2);                  // тип 2 - зона на игроке
        bs.Write((uint16_t)target);
        bs.Write((uint32_t)0);
        Native::MBSendRPC(to, 252, &bs, 1, 9);
    }

    // 252:10 тип 1 - глобальная зона (AudioChannelGlobal в AZVoice): звук без позиции, слышно везде
    // [u16 uid][u16 длина + название][4 байта][u8 1][u32 0] - так Arizona шлёт свои именованные зоны
    inline void SendGlobalZone(int zone, const std::string& name, int to)
    {
        static const uint8_t style[] = { 0xFF, 0xDA, 0xA2, 0xC2 }; // как у зон Arizona
        BitStream bs;
        bs.Write((uint8_t)10);
        bs.Write((uint16_t)zone);
        bs.Write((uint16_t)name.size());
        if (!name.empty()) bs.Write(name.c_str(), (unsigned int)name.size());
        bs.Write((const char*)style, sizeof(style));
        bs.Write((uint8_t)1);
        bs.Write((uint32_t)0);
        Native::MBSendRPC(to, 252, &bs, 1, 9);
    }

    inline void SendDestroyZone(int zone, int to)
    {
        BitStream bs;
        bs.Write((uint8_t)11);
        bs.Write((uint16_t)zone);
        Native::MBSendRPC(to, 252, &bs, 1, 9);
    }

    // 252:3 - версия серверной части голоса (u32) + u16 0. Клиент AZVoice сравнивает её с 0x01010300
    inline void SendHandshake(int to)
    {
        BitStream bs;
        bs.Write((uint8_t)3);
        const uint8_t reply[] = { 0x00, 0x03, 0x01, 0x01, 0x00, 0x00 };
        bs.Write((const char*)reply, sizeof(reply));
        Native::MBSendRPC(to, 252, &bs, 1, 9);
    }

    inline void SendInit(int to)
    {
        if (inited.test(to)) return;
        BitStream bs;
        bs.Write((uint8_t)9);
        bs.Write((uint16_t)12000);
        bs.Write((uint16_t)0);
        Native::MBSendRPC(to, 252, &bs, 1, 9);
        inited.set(to);
    }

    // категории звука, которые Arizona шлёт между двумя зонами на игроке (байты как у Arizona)
    // Уведомления, Озвучка и фразы, Атмосферная музыка и звуки #1/#2, AZPotify
    inline void SendCategories(int to)
    {
        static const std::vector<std::vector<uint8_t>> cats = {
            { 0x0B,0x00, 0xD3,0xE2,0xE5,0xE4,0xEE,0xEC,0xEB,0xE5,0xED,0xE8,0xFF },
            { 0x0F,0x00, 0xCE,0xE7,0xE2,0xF3,0xF7,0xEA,0xE0,0x20,0xE8,0x20,0xF4,0xF0,0xE0,0xE7,0xFB },
            { 0x1D,0x00, 0xC0,0xF2,0xEC,0xEE,0xF1,0xF4,0xE5,0xF0,0xED,0xE0,0xFF,0x20,0xEC,0xF3,0xE7,0xFB,0xEA,0xE0,0x20,0xE8,0x20,0xE7,0xE2,0xF3,0xEA,0xE8,0x20,0x23,0x31 },
            { 0x1D,0x00, 0xC0,0xF2,0xEC,0xEE,0xF1,0xF4,0xE5,0xF0,0xED,0xE0,0xFF,0x20,0xEC,0xF3,0xE7,0xFB,0xEA,0xE0,0x20,0xE8,0x20,0xE7,0xE2,0xF3,0xEA,0xE8,0x20,0x23,0x32 },
            { 0x08,0x00, 0x41,0x5A,0x50,0x6F,0x74,0x69,0x66,0x79 },
        };
        static const uint8_t tail[] = { 0xFF, 0xDA, 0xA2, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x00 };
        int uid = 39990;
        for (const auto& c : cats)
        {
            BitStream bs;
            bs.Write((uint8_t)10);
            bs.Write((uint16_t)uid++);
            bs.Write((const char*)c.data(), (unsigned int)c.size());
            bs.Write((const char*)tail, sizeof(tail));
            Native::MBSendRPC(to, 252, &bs, 1, 9);
        }
    }

    // своя зона игрока, как у Arizona: зона на нём, категории звука, вторая зона на нём, 252:19
    inline void SendSelf(int to)
    {
        if (!selfZone[to][0]) { selfZone[to][0] = NewZone(); selfZone[to][1] = NewZone(); }
        SendPlayerZone(selfZone[to][0], to, to);
        SendCategories(to);
        SendPlayerZone(selfZone[to][1], to, to);
        selfSent.set(to);
        BitStream bs;
        bs.Write((uint8_t)19);
        bs.Write((uint16_t)selfZone[to][0]);
        bs.Write((uint8_t)0);
        Native::MBSendRPC(to, 252, &bs, 1, 9);
    }

    inline void SendChannel(int uid, const Channel& c, int to)
    {
        BitStream bs;
        bs.Write((uint8_t)6);
        bs.Write((uint8_t)PacketId(uid));
        bs.Write((uint16_t)c.name.size());
        bs.Write(c.name.c_str(), (unsigned int)c.name.size());
        Native::MBSendRPC(to, 252, &bs, 1, 9);
    }

    inline void SendZone(int member, Member& m, int to)
    {
        SendPlayerZone(m.zone, member, to);
        m.shown.set(to);
    }

    inline void SendDestroy(Member& m, int to)
    {
        BitStream bs;
        bs.Write((uint8_t)11);
        bs.Write((uint16_t)m.zone);
        Native::MBSendRPC(to, 252, &bs, 1, 9);
        m.shown.reset(to);
    }

    inline bool Near(int a, int b, float radius)
    {
        float ax, ay, az, bx, by, bz;
        if (!Platform::GetPlayerPos(a, ax, ay, az) || !Platform::GetPlayerPos(b, bx, by, bz)) return false;
        const float dx = ax - bx, dy = ay - by, dz = az - bz;
        return dx * dx + dy * dy + dz * dz <= radius * radius;
    }

    // разослать/убрать зоны так, чтобы каждый слышал ровно тех, кого должен
    inline void Update(Channel& c)
    {
        // рация: каждому участнику с лаунчером - глобальная зона канала
        if (c.radius <= 0.0f)
        {
            if (!c.globalZone) c.globalZone = NewZone();
            for (auto& [member, unused] : c.members)
                if (PI[member].pLauncherStat && !c.globalShown.test(member)) { SendGlobalZone(c.globalZone, c.name, member); c.globalShown.set(member); }
        }
        else if (c.globalZone)
        {
            for (int i = 0; i < MAX_PLAYERS; i++)
                if (c.globalShown.test(i)) SendDestroyZone(c.globalZone, i);
            c.globalShown.reset();
        }
        // позиции участников - один раз за обновление
        // пару: на канале из N игроков это 2*N*N запросов в основном потоке каждую секунду
        struct Pos { int id; Member* m; float x, y, z; bool ok, launcher; };
        std::vector<Pos> pos;
        pos.reserve(c.members.size());
        const bool radio = c.radius <= 0.0f;
        for (auto& [id, m] : c.members)
        {
            Pos p{ id, &m, 0.0f, 0.0f, 0.0f, false, PI[id].pLauncherStat != 0 };
            if (!radio) p.ok = Platform::GetPlayerPos(id, p.x, p.y, p.z);
            pos.push_back(p);
        }
        const float r2 = c.radius * c.radius;
        for (const Pos& a : pos)
        {
            const bool silent = muted.test(a.id);
            for (const Pos& b : pos)
            {
                bool should = a.id != b.id && !silent && b.launcher;
                if (should && !radio)
                {
                    const float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z;
                    should = a.ok && b.ok && dx * dx + dy * dy + dz * dz <= r2;
                }
                if (should && !a.m->shown.test(b.id)) SendZone(a.id, *a.m, b.id);
                else if (!should && a.m->shown.test(b.id)) SendDestroy(*a.m, b.id);
            }
        }
    }

    inline void UpdateAll()
    {
        for (auto& c : channels)
            if (c.used) Update(c);
    }

    // PKT 252 от игрока: переслать тем, кто его слышит
    // От клиента:  FC | u16 номер | u8 канал | звук
    inline void Relay(int speaker, const uint8_t* data, unsigned int len)
    {
        if (format < 0 || len < 4 + 3 || len - 4 > 512 || speaker < 0 || speaker >= MAX_PLAYERS || muted.test(speaker)) return;
        Channel* c = Get(data[3] - CHANNEL_BASE + 1);
        if (!c) return;
        auto it = c->members.find(speaker);
        if (it == c->members.end()) return;
        Member& m = it->second;
        const bool radio = c->radius <= 0.0f;
        for (auto& [listener, unused] : c->members)
        {
            const bool self = echo && listener == speaker && PI[listener].pLauncherStat;
            // кто кого слышит, уже посчитано в Update (раз в секунду): зона отправлена - слышит
            if (!self && listener == speaker) continue;
            // рация: голос играет из глобальной зоны канала (без позиции). Зона на говорящем привязана
            // к его педу, а когда говорящий далеко (не в прорисовке), педа у слушателя нет - голоса не было
            int zone;
            if (self) zone = selfZone[speaker][0];
            else if (radio) zone = c->globalShown.test(listener) ? c->globalZone : 0;
            else zone = m.shown.test(listener) ? m.zone : 0;
            if (!zone || Afk::Away(listener)) continue; // свёрнутому голос не копим - он живой
            BitStream bs;
            bs.Write((uint8_t)0xFC);
            bs.Write((uint16_t)speaker);
            bs.Write((const char*)data + 1, 2);                // номер пакета как у клиента
            bs.Write((uint16_t)1);
            bs.Write((uint16_t)zone);
            bs.Write((const char*)data + 4, len - 4);          // звук
            Platform::SendPacket(listener, &bs);
        }
    }

    inline int Create(const std::string& name, float radius, void* owner = nullptr)
    {
        Channel c;
        c.used = true;
        c.owner = owner;
        // название уходит в CEF клиента: без управляющих символов, кавычек и \ (мусор ломал меню голоса)
        for (char ch : name)
            if ((unsigned char)ch >= 0x20 && ch != '"' && ch != '\'' && ch != '\\') c.name += ch;
        if (c.name.empty()) c.name = "Локальный";
        c.radius = radius < 0.0f ? 0.0f : radius;
        for (size_t i = 0; i < channels.size(); i++)
            if (!channels[i].used) { channels[i] = c; return (int)i + 1; }
        if ((int)channels.size() >= MAX_CHANNELS) return 0;
        channels.push_back(c);
        return (int)channels.size();
    }

    inline bool AddPlayer(int uid, int playerid)
    {
        Channel* c = Get(uid);
        if (!c || !ValidPlayer(playerid) || c->members.count(playerid)) return false;
        Member m;
        m.zone = NewZone();
        c->members[playerid] = m;
        if (PI[playerid].pLauncherStat)
        {
            SendInit(playerid);
            SendChannel(uid, *c, playerid);
            if (!selfSent.test(playerid)) SendSelf(playerid);
        }
        Update(*c);
        return true;
    }

    inline bool RemovePlayer(int uid, int playerid)
    {
        Channel* c = Get(uid);
        if (!c) return false;
        auto it = c->members.find(playerid);
        if (it == c->members.end()) return false;
        // его больше никто не слышит
        for (int i = 0; i < MAX_PLAYERS; i++)
            if (it->second.shown.test(i)) SendDestroy(it->second, i);
        // и он больше никого из канала не слышит
        for (auto& [member, m] : c->members)
            if (m.shown.test(playerid)) SendDestroy(m, playerid);
        if (c->globalShown.test(playerid)) { SendDestroyZone(c->globalZone, playerid); c->globalShown.reset(playerid); }
        c->members.erase(it);
        return true;
    }

    inline bool IsInChannel(int uid, int playerid)
    {
        Channel* c = Get(uid);
        return c && c->members.count(playerid) > 0;
    }

    // в канале и реально в зоне: общий канал - всегда, локальный - рядом есть хотя бы один участник
    inline bool IsInZone(int uid, int playerid)
    {
        Channel* c = Get(uid);
        if (!c || !c->members.count(playerid)) return false;
        if (c->radius <= 0.0f) return true;
        for (auto& [member, m] : c->members)
            if (member != playerid && Near(member, playerid, c->radius)) return true;
        return false;
    }

    inline bool SetRadius(int uid, float radius)
    {
        Channel* c = Get(uid);
        if (!c) return false;
        c->radius = radius < 0.0f ? 0.0f : radius;
        Update(*c);
        return true;
    }

    inline bool SetName(int uid, const std::string& name)
    {
        Channel* c = Get(uid);
        if (!c || name.empty()) return false;
        c->name = name;
        for (auto& [member, m] : c->members)
            if (PI[member].pLauncherStat) SendChannel(uid, *c, member);
        // рация: зона с новым названием (по нему клиент подписывает говорящих)
        if (c->globalZone)
        {
            for (int i = 0; i < MAX_PLAYERS; i++)
                if (c->globalShown.test(i)) SendDestroyZone(c->globalZone, i);
            c->globalShown.reset();
            Update(*c);
        }
        return true;
    }

    inline bool Destroy(int uid)
    {
        Channel* c = Get(uid);
        if (!c) return false;
        for (auto& [member, m] : c->members)
            for (int i = 0; i < MAX_PLAYERS; i++)
                if (m.shown.test(i)) SendDestroy(m, i);
        for (int i = 0; i < MAX_PLAYERS; i++)
            if (c->globalShown.test(i)) SendDestroyZone(c->globalZone, i);
        *c = Channel();
        return true;
    }

    // скрипт выгружен (gmx): его каналы удаляются. Иначе новый мод создавал "Локальный" под новым id
    // а клавиша у клиента была привязана к старому - в подсказке HUD писало undefined вместо клавиши
    inline void FreeOwner(void* owner)
    {
        for (size_t i = 0; i < channels.size(); i++)
            if (channels[i].used && channels[i].owner == owner) Destroy((int)i + 1);
    }

    // клиент прислал 252:3 (CEF загрузился / перезагрузился): ответить и заново выдать голос
    inline void Handshake(int playerid)
    {
        if (playerid < 0 || playerid >= MAX_PLAYERS) return;
        SendHandshake(playerid);
        inited.reset(playerid);
        for (size_t i = 0; i < channels.size(); i++)
        {
            Channel& c = channels[i];
            if (!c.used || !c.members.count(playerid) || !PI[playerid].pLauncherStat) continue;
            SendInit(playerid);
            SendChannel((int)i + 1, c, playerid);
        }
        if (inited.test(playerid)) SendSelf(playerid);
        else selfSent.reset(playerid);
        // зоны других заново (клиент мог их потерять)
        for (auto& c : channels)
        {
            for (auto& [member, m] : c.members)
                m.shown.reset(playerid);
            c.globalShown.reset(playerid);
        }
        UpdateAll();
    }

    // подключение игрока с лаунчером - как Arizona: ответ 03, 09, канал "Локальный" (id 0x5A)
    inline void Connect(int playerid)
    {
        if (playerid < 0 || playerid >= MAX_PLAYERS) return;
        SendHandshake(playerid);
        inited.reset(playerid);
        SendInit(playerid);
        Channel* first = Get(1);
        Channel local;
        local.name = "Локальный";
        SendChannel(1, first ? *first : local, playerid);
    }

    inline bool Mute(int playerid, bool toggle)
    {
        if (!ValidPlayer(playerid)) return false;
        muted.set(playerid, toggle);
        UpdateAll();
        return true;
    }

    inline void PlayerLeft(int playerid)
    {
        if (playerid < 0 || playerid >= MAX_PLAYERS) return;
        for (size_t i = 0; i < channels.size(); i++)
            if (channels[i].used) RemovePlayer((int)i + 1, playerid);
        muted.reset(playerid);
        inited.reset(playerid);
        selfSent.reset(playerid);
        selfZone[playerid][0] = selfZone[playerid][1] = 0;
    }
}
