#pragma once
// Пакеты ядра Arizona (vorbisFile.dll), сервер -> клиент: raw 220 + u8 сабтип + данные
// Разобрано по обработчикам ядра (модули SyncDirt, DirtySampObjects, PlayParticle, ChatWindowMode

#include <lib/InfoServer.h>
#include <lib/Native.h>
#include <raknet/bitstream.h>
#include "../../Platform.h"
#include <cstring>
#include <string>
#include <vector>

namespace ArzExtra
{
	constexpr int MAX_DIRTY_OBJECTS = 1000;   // клиент принимает objectid <= 999
	constexpr int ACS_SLOTS = 10;

	inline uint8_t objDirt[MAX_DIRTY_OBJECTS];          // 0 - чистый
	inline uint8_t objDirtTex[MAX_DIRTY_OBJECTS];
	inline uint8_t acsDirt[MAX_PLAYERS][ACS_SLOTS];
	inline uint8_t acsDirtTex[MAX_PLAYERS][ACS_SLOTS];

	// 220:134 масштаб объектов (0 - не задан)
	inline float objScale[MAX_DIRTY_OBJECTS];
	inline bool objScaleCol[MAX_DIRTY_OBJECTS];

	// 220:138 машина на машине (эвакуатор/автовоз): 48 байт как читает клиент
	constexpr int VEH_ATTACH_SLOTS = 10;
	struct VehAttach
	{
		bool used = false;
		float off[3] = {}, rot[3] = {};
		uint8_t upgrades[14] = {};
		uint8_t flags = 0, paintjob = 0;
		uint16_t model = 0;
		uint8_t color1 = 0, color2 = 0;
		float health = 1000.0f;
	};
	inline VehAttach vehAttach[MAX_VEHICLES][VEH_ATTACH_SLOTS];

	// 220:192 звук клаксона (220:162 хлопки из выхлопа - SetShotsExhaust)
	inline int vehHorn[MAX_VEHICLES];          // 0 - не задан, иначе id + 1

	// 220:213 масштаб костей игрока, 220:34 трассеры пуль игрока
	struct BoneScale { uint16_t bone; float scale; };
	inline std::vector<BoneScale> bones[MAX_PLAYERS];
	struct Tracer { bool set = false; uint32_t color = 0; float time = 0.0f; bool laser = false; };
	inline Tracer tracer[MAX_PLAYERS];

	inline void Begin(BitStream& bs, uint8_t sub)
	{
		bs.Write((uint8_t)220);
		bs.Write(sub);
	}

	inline void WriteStr8(BitStream& bs, const char* s)
	{
		size_t len = s ? std::strlen(s) : 0;
		if (len > 255) len = 255;
		bs.Write((uint8_t)len);
		if (len) bs.Write(s, (int)len);
	}

	inline bool Launcher(int playerid)
	{
		return playerid >= 0 && playerid < MAX_PLAYERS && PI[playerid].pLauncherStat && Platform::IsPlayerConnected(playerid);
	}

	// одному игроку (playerid >= 0) или всем с лаунчером (-1)
	inline void SendTo(int playerid, BitStream& bs)
	{
		if (playerid >= 0) { if (Launcher(playerid)) Native::MBSendPacket(playerid, &bs, 1, 9); return; }
		for (int i = 0; i < MAX_PLAYERS; i++)
			if (Launcher(i)) Native::MBSendPacket(i, &bs, 1, 9);
	}

	// ---- 220:26 грязь и износ машины
	inline void SendVehicleDirt(int carid, int to = -1)
	{
		if (carid <= 0 || carid >= MAX_VEHICLES || vTemp[carid].vDirt < 0.0f) return;
		BitStream bs;
		Begin(bs, 26);
		bs.Write((int16_t)carid);
		bs.Write(vTemp[carid].vDirt);
		bs.Write(vTemp[carid].vDirtRate);
		bs.Write((uint8_t)vTemp[carid].vWear);
		bs.Write(false);
		Native::SendPackerPlayer(bs, -1, carid, to);
	}

	// ---- 220:133 цвет подсветки поворотников на земле (0xRRGGBBAA, AA - яркость)
	inline void SendTurnLightColor(int carid, int to = -1)
	{
		if (carid <= 0 || carid >= MAX_VEHICLES || !vTemp[carid].vTurnColorSet) return;
		const uint32_t c = vTemp[carid].vTurnColor;
		BitStream bs;
		Begin(bs, 133);
		bs.Write((uint16_t)carid);
		bs.Write((uint8_t)(c & 0xFF));          // яркость
		bs.Write((uint8_t)((c >> 8) & 0xFF));   // B
		bs.Write((uint8_t)((c >> 16) & 0xFF));  // G
		bs.Write((uint8_t)(c >> 24));           // R
		Native::SendPackerPlayer(bs, -1, carid, to);
	}

	// ---- 220:218 грязь объекта / аксессуара
	inline void SendObjectDirt(int objectid, int to = -1)
	{
		if (objectid < 0 || objectid >= MAX_DIRTY_OBJECTS) return;
		BitStream bs;
		Begin(bs, 218);
		bs.Write(false);
		bs.Write(objDirt[objectid]);
		bs.Write((uint16_t)objectid);
		bs.Write(objDirtTex[objectid]);
		SendTo(to, bs);
	}

	inline void SendAcsDirt(int playerid, int slot, int to = -1)
	{
		if (playerid < 0 || playerid >= MAX_PLAYERS || slot < 0 || slot >= ACS_SLOTS) return;
		BitStream bs;
		Begin(bs, 218);
		bs.Write(true);
		bs.Write(acsDirt[playerid][slot]);
		bs.Write((uint16_t)playerid);
		bs.Write((uint8_t)slot);
		bs.Write(acsDirtTex[playerid][slot]);
		if (to >= 0) SendTo(to, bs);
		else Native::SendPackerPlayer(bs, -1, playerid, -1, 1); // сам игрок и те, у кого он в стриме
	}

	inline void SendObjectScale(int objectid, int to = -1)
	{
		if (objectid < 0 || objectid >= MAX_DIRTY_OBJECTS) return;
		BitStream bs;
		Begin(bs, 134);
		bs.Write((uint16_t)objectid);
		bs.Write(objScale[objectid] > 0.0f ? objScale[objectid] : 1.0f);
		bs.Write(objScaleCol[objectid]);
		SendTo(to, bs);
	}

	inline void SendVehAttach(int carid, int slot, int to = -1)
	{
		const VehAttach& a = vehAttach[carid][slot];
		BitStream bs;
		Begin(bs, 138);
		bs.Write((int16_t)carid);
		bs.Write((uint8_t)slot);
		bs.Write(a.used);
		if (a.used)
		{
			for (float v : a.off) bs.Write(v);
			for (float v : a.rot) bs.Write(v);
			for (uint8_t u : a.upgrades) bs.Write(u);
			bs.Write(a.flags);
			bs.Write(a.paintjob);
			bs.Write(a.model);
			bs.Write(a.color1);
			bs.Write(a.color2);
			bs.Write(a.health);
		}
		Native::SendPackerPlayer(bs, -1, carid, to);
	}

	inline void SendHorn(int carid, int to = -1)
	{
		BitStream bs;
		Begin(bs, 192);
		bs.Write((int16_t)carid);
		bs.Write((uint8_t)(vehHorn[carid] > 0 ? vehHorn[carid] - 1 : 0xFF));
		bs.Write(true);
		Native::SendPackerPlayer(bs, -1, carid, to);
	}

	inline void SendBones(int playerid, int to = -1)
	{
		BitStream bs;
		Begin(bs, 213);
		bs.Write((uint16_t)playerid);
		bs.Write((uint16_t)bones[playerid].size());
		for (const BoneScale& b : bones[playerid]) { bs.Write(b.bone); bs.Write(b.scale); }
		if (to >= 0) SendTo(to, bs);
		else Native::SendPackerPlayer(bs, -1, playerid, -1, 1);
	}

	inline void SendTracer(int playerid, int to = -1)
	{
		const Tracer& t = tracer[playerid];
		BitStream bs;
		Begin(bs, 34);
		bs.Write((int16_t)playerid);
		if (t.set) { bs.Write(t.color); bs.Write(t.time); bs.Write(t.laser); } // без полей - сброс к стандартному
		SendTo(to, bs);
	}

	inline void ResetVehicle(int carid)
	{
		if (carid <= 0 || carid >= MAX_VEHICLES) return;
		for (int s = 0; s < VEH_ATTACH_SLOTS; s++) vehAttach[carid][s] = VehAttach();
		vehHorn[carid] = 0;
	}

	// ---- повторная отправка
	inline void OnVehicleStreamIn(int carid, int playerid)
	{
		SendVehicleDirt(carid, playerid);
		SendTurnLightColor(carid, playerid);
		for (int s = 0; s < VEH_ATTACH_SLOTS; s++) if (vehAttach[carid][s].used) SendVehAttach(carid, s, playerid);
		if (vehHorn[carid]) SendHorn(carid, playerid);
	}

	inline void OnPlayerStreamIn(int viewerid, int playerid)
	{
		for (int s = 0; s < ACS_SLOTS; s++)
			if (acsDirt[playerid][s]) SendAcsDirt(playerid, s, viewerid);
		if (!bones[playerid].empty()) SendBones(playerid, viewerid);
	}

	// игрок включил лаунчер: общее для всех (объекты без стрима, трассеры игроков)
	inline void OnLauncherEnabled(int playerid)
	{
		for (int o = 0; o < MAX_DIRTY_OBJECTS; o++)
		{
			if (objDirt[o]) SendObjectDirt(o, playerid);
			if (objScale[o] > 0.0f) SendObjectScale(o, playerid);
		}
		for (int s = 0; s < ACS_SLOTS; s++)
			if (acsDirt[playerid][s]) SendAcsDirt(playerid, s, playerid);
		if (!bones[playerid].empty()) SendBones(playerid, playerid);
		for (int i = 0; i < MAX_PLAYERS; i++) if (tracer[i].set) SendTracer(i, playerid);
	}

	inline void PlayerLeft(int playerid)
	{
		if (playerid < 0 || playerid >= MAX_PLAYERS) return;
		std::memset(acsDirt[playerid], 0, sizeof(acsDirt[playerid]));
		std::memset(acsDirtTex[playerid], 0, sizeof(acsDirtTex[playerid]));
		bones[playerid].clear();
		if (tracer[playerid].set) { tracer[playerid] = Tracer(); SendTracer(playerid); }
	}

	// водитель шлёт 220:26 {float грязь 0..100} не чаще раза в секунду, когда она меняется
	// запоминаем и раздаём остальным, кто видит машину
	inline void OnClientDirt(int playerid, float dirt)
	{
		int seat = -1;
		const int carid = Platform::GetPlayerVehicle(playerid, &seat);
		if (carid <= 0 || carid >= MAX_VEHICLES || seat != 0) return;
		if (!(dirt >= 0.0f)) dirt = 0.0f;
		if (dirt > 100.0f) dirt = 100.0f;
		vTemp[carid].vDirt = dirt;
		SendVehicleDirt(carid);
	}
}
