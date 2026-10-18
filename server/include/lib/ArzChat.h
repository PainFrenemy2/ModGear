#pragma once
// Чат Arizona (_chat.asi): вкладки, ники в стиле Arizona, значки чата, цифры урона, сообщения во вкладку
// _chat.asi ловит пакеты 220
// Цвета - 0xRRGGBBAA. str8 - u8 длина + байты, str16 - u16 длина + байты (cp1251)

#include <lib/ArzExtra.h>
#include <string>

namespace ArzChat
{
	constexpr int MAX_TABS = 32;
	constexpr int MAX_BADGES = 32;
	constexpr int TAG_PARAMS = 20;

	// ---- вкладки чата игрока (220:36 / 37)
	struct Tab { bool used = false; std::string command, name; uint32_t color = 0xFFFFFFFF; bool exclusive = false; };
	inline Tab tabs[MAX_PLAYERS][MAX_TABS];

	// ---- ник игрока в стиле Arizona (220:223), видят все
	struct NameTag
	{
		bool title = false; uint32_t titleColor = 0; std::string titleText; uint8_t titleIcon = 0, titleStyle = 0xFF;
		bool status = false; std::string statusText;
		bool plate = false; uint32_t plateBg = 0, plateColor = 0; std::string plateText;
		bool paramSet[TAG_PARAMS] = {}; uint16_t param[TAG_PARAMS] = {};
	};
	inline NameTag tags[MAX_PLAYERS];
	inline bool customTags = false;         // 220:42 всем (SetNameTagStyle)
	inline bool bubbleNoDistance = false;    // 220:43

	inline void Write16(BitStream& bs, const std::string& s)
	{
		const size_t len = s.size() > 0xFFFF ? 0xFFFF : s.size();
		bs.Write((uint16_t)len);
		if (len) bs.Write(s.data(), (int)len);
	}

	// ---- вкладки
	inline void SendTab(int playerid, int id)
	{
		const Tab& t = tabs[playerid][id];
		BitStream bs;
		if (t.used)
		{
			ArzExtra::Begin(bs, 36);
			bs.Write((uint8_t)id);
			ArzExtra::WriteStr8(bs, t.command.c_str());
			bs.Write(t.color);
			ArzExtra::WriteStr8(bs, t.name.c_str());
			bs.Write((uint8_t)(t.exclusive ? 1 : 0));
		}
		else
		{
			ArzExtra::Begin(bs, 37);
			bs.Write((uint8_t)id);
		}
		ArzExtra::SendTo(playerid, bs);
	}

	// ---- ники: 220:223 [u16 игрок][u8 тип][...]
	inline void TagBegin(BitStream& bs, int playerid, uint8_t type)
	{
		ArzExtra::Begin(bs, 223);
		bs.Write((uint16_t)playerid);
		bs.Write(type);
	}

	inline void SendTitle(int playerid, int to = -1)
	{
		const NameTag& t = tags[playerid];
		BitStream bs;
		TagBegin(bs, playerid, t.title ? 1 : 2);
		if (t.title)
		{
			bs.Write(t.titleColor);
			ArzExtra::WriteStr8(bs, t.titleText.c_str());
			bs.Write(t.titleIcon);
			bs.Write(t.titleStyle);
		}
		ArzExtra::SendTo(to, bs);
	}

	inline void SendStatus(int playerid, int to = -1)
	{
		const NameTag& t = tags[playerid];
		BitStream bs;
		TagBegin(bs, playerid, t.status ? 3 : 4);
		if (t.status) Write16(bs, t.statusText);
		ArzExtra::SendTo(to, bs);
	}

	inline void SendPlate(int playerid, int to = -1)
	{
		const NameTag& t = tags[playerid];
		BitStream bs;
		TagBegin(bs, playerid, t.plate ? 9 : 10);
		if (t.plate)
		{
			bs.Write(t.plateBg);
			bs.Write(t.plateColor);
			ArzExtra::WriteStr8(bs, t.plateText.c_str());
		}
		ArzExtra::SendTo(to, bs);
	}

	inline void SendParam(int playerid, int key, int to = -1)
	{
		BitStream bs;
		TagBegin(bs, playerid, 5);
		bs.Write((uint8_t)key);
		bs.Write(tags[playerid].param[key]);
		ArzExtra::SendTo(to, bs);
	}

	inline void SendFlag(uint8_t sub, bool on, int to = -1)
	{
		BitStream bs;
		ArzExtra::Begin(bs, sub);
		bs.Write((uint8_t)(on ? 1 : 0));
		ArzExtra::SendTo(to, bs);
	}

	inline void ResetTag(int playerid)
	{
		tags[playerid] = NameTag();
		BitStream bs;
		TagBegin(bs, playerid, 12);
		ArzExtra::SendTo(-1, bs);
	}

	constexpr int MAX_PEDS = 1000;
	inline uint16_t pedGrad[MAX_PEDS][2];
	inline bool pedGradSet[MAX_PEDS];

	inline uint16_t Hsv(int hue, int sat)
	{
		hue = hue < 0 ? 0 : (hue > 359 ? 359 : hue);
		sat = sat < 0 ? 0 : (sat > 100 ? 100 : sat);
		return (uint16_t)(hue | (sat << 9));
	}

	inline void SendPedGradient(int pedid, int to = -1)
	{
		for (int n = 0; n < 2; n++)
		{
			BitStream bs;
			TagBegin(bs, 0, (uint8_t)(13 + n));
			bs.Write((uint16_t)pedid);
			bs.Write(pedGrad[pedid][n]);
			ArzExtra::SendTo(to, bs);
		}
	}

	// игрок включил лаунчер: всё, что ему положено видеть
	inline void OnLauncherEnabled(int playerid)
	{
		if (customTags) SendFlag(42, true, playerid);
		if (bubbleNoDistance) SendFlag(43, true, playerid);
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			const NameTag& t = tags[i];
			if (t.title) SendTitle(i, playerid);
			if (t.status) SendStatus(i, playerid);
			if (t.plate) SendPlate(i, playerid);
			for (int k = 0; k < TAG_PARAMS; k++) if (t.paramSet[k]) SendParam(i, k, playerid);
		}
		for (int id = 0; id < MAX_TABS; id++) if (tabs[playerid][id].used) SendTab(playerid, id);
		for (int p = 0; p < MAX_PEDS; p++) if (pedGradSet[p]) SendPedGradient(p, playerid);
	}

	inline void PlayerLeft(int playerid)
	{
		if (playerid < 0 || playerid >= MAX_PLAYERS) return;
		for (Tab& t : tabs[playerid]) t = Tab();
		ResetTag(playerid);
	}
}
