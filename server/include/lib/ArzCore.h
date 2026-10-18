#pragma once
// Пакеты модуля core.asi клиента Arizona (сервер -> клиент: raw 220 + u8 сабтип + данные)
// Форматы сняты с обработчиков core.asi (sigslot CustomPacket). Биты - RakNet bool (1 бит)
// строки - как читает клиент: str8 = u8 длина + байты, str32 = u32 длина + байты

#include <lib/ArzExtra.h>
#include <string>
#include <vector>
#include <map>

namespace ArzCore
{
	// ---- 220:216 стиль походки (группа анимаций GTA) игрока, видят все
	inline std::string walkStyle[MAX_PLAYERS];

	// [u16 игрок][str8 группа] - пустая строка возвращает обычную походку скина
	inline void SendWalkStyle(int playerid, int to = -1)
	{
		BitStream bs;
		ArzExtra::Begin(bs, 216);
		bs.Write((uint16_t)playerid);
		ArzExtra::WriteStr8(bs, walkStyle[playerid].c_str());
		if (to != -1) { ArzExtra::SendTo(to, bs); return; }
		// себе и тем, у кого игрок в зоне стрима
		ArzExtra::SendTo(playerid, bs);
		for (int i : connectedPlayers)
			if (i != playerid && Platform::IsPlayerStreamedIn(playerid, i)) ArzExtra::SendTo(i, bs);
	}

	// ---- 220:161 текстуры и цвет скина игрока (модуль PedMaterial), видят все
	// В клумпе педа ищется материал с текстурой texture, ему ставится текстура newTexture из того же TXD
	// (txd 0) или из TXD Arizona с номером txd, и цвет (0 - без цвета)
	struct SkinMaterial { std::string texture, newTexture; uint32_t color; uint8_t txd; };
	constexpr size_t MAX_SKIN_MATERIALS = 16;
	inline std::vector<SkinMaterial> skinMaterials[MAX_PLAYERS];
	inline uint8_t lightMode[MAX_PLAYERS];          // 220:180 режим света игрока (0 - выкл)

	inline void WriteStr16(BitStream& bs, const std::string& s)
	{
		const uint16_t len = (uint16_t)(s.size() > 0x8F ? 0x8F : s.size()); // клиент читает в буфер 0x90
		bs.Write(len);
		if (len) bs.Write(s.data(), len);
	}

	// [u16 игрок][u8 n] n x {str16 текстура, u32 цвет (A, B, G, R), str16 новая текстура, u8 txd}; n = 0 - сброс
	inline void SendSkinMaterials(int playerid, int to = -1)
	{
		BitStream bs;
		ArzExtra::Begin(bs, 161);
		bs.Write((uint16_t)playerid);
		const auto& list = skinMaterials[playerid];
		bs.Write((uint8_t)list.size());
		for (const SkinMaterial& m : list)
		{
			WriteStr16(bs, m.texture);
			bs.Write((uint8_t)(m.color & 0xFF));
			bs.Write((uint8_t)(m.color >> 8));
			bs.Write((uint8_t)(m.color >> 16));
			bs.Write((uint8_t)(m.color >> 24));
			WriteStr16(bs, m.newTexture);
			bs.Write(m.txd);
		}
		if (to != -1) { ArzExtra::SendTo(to, bs); return; }
		ArzExtra::SendTo(playerid, bs);
		for (int i : connectedPlayers)
			if (i != playerid && Platform::IsPlayerStreamedIn(playerid, i)) ArzExtra::SendTo(i, bs);
	}

	// ---- порталы (модуль Portal): пара сторон 1 и 2, вход в одну - выход из другой
	// 220:169 [u16 id][u8 сторона 1/2][3 x float позиция][3 x float направление] - создать/сдвинуть сторону
	// 220:170 [u16 id][u8 сторона] - убрать сторону; 220:166 [bit] - порталы работают у игрока
	constexpr int MAX_PORTALS = 1004;
	struct PortalSide { float pos[3], dir[3]; };
	inline std::map<int, PortalSide> portals;            // ключ: id * 2 + (сторона - 1)

	inline void SendPortalSide(int key, int to)
	{
		const PortalSide& p = portals[key];
		BitStream bs;
		ArzExtra::Begin(bs, 169);
		bs.Write((uint16_t)(key / 2));
		bs.Write((uint8_t)(key % 2 + 1));
		for (float v : p.pos) bs.Write(v);
		for (float v : p.dir) bs.Write(v);
		ArzExtra::SendTo(to, bs);
	}

	inline void SendPortalRemove(int id, int side, int to)
	{
		BitStream bs;
		ArzExtra::Begin(bs, 170);
		bs.Write((uint16_t)id);
		bs.Write((uint8_t)side);
		ArzExtra::SendTo(to, bs);
	}

	// 220:180 [u16 игрок][u8 режим] - видят все
	inline void SendLightMode(int playerid, int to = -1)
	{
		BitStream bs;
		ArzExtra::Begin(bs, 180);
		bs.Write((uint16_t)playerid);
		bs.Write(lightMode[playerid]);
		if (to != -1) { ArzExtra::SendTo(to, bs); return; }
		// и самому игроку
		for (int i : connectedPlayers)
			if (i == playerid || Platform::IsPlayerStreamedIn(playerid, i)) ArzExtra::SendTo(i, bs);
	}

	// ---- режимы "для всех" (playerid -1): игрок, включивший лаунчер позже, получает их тоже
	// вызовы для одного игрока не запоминаются (мод сам ставит их при входе)
	struct Global { std::vector<uint8_t> data; int bits = 0; };
	inline std::map<uint8_t, Global> globals;
	inline std::vector<std::string> globalMaps;  // LoadPlayerMap(-1, ...) - карт может быть несколько

	inline void RememberGlobal(uint8_t sub, BitStream& bs)
	{
		Global& g = globals[sub];
		g.data.assign(bs.GetData(), bs.GetData() + bs.GetNumberOfBytesUsed());
		g.bits = bs.GetNumberOfBitsUsed();
	}

	inline void SendMap(int playerid, const std::string& file)
	{
		BitStream bs;
		ArzExtra::Begin(bs, 165);
		ArzExtra::WriteStr8(bs, file.c_str());
		ArzExtra::SendTo(playerid, bs);
	}

	inline void OnPlayerStreamIn(int viewerid, int playerid)
	{
		if (lightMode[playerid]) SendLightMode(playerid, viewerid);
		if (!walkStyle[playerid].empty()) SendWalkStyle(playerid, viewerid);
		if (!skinMaterials[playerid].empty()) SendSkinMaterials(playerid, viewerid);
	}

	inline void OnLauncherEnabled(int playerid)
	{
		// режимы, включённые всем до его входа
		for (auto& [sub, g] : globals)
		{
			BitStream bs(g.data.data(), (unsigned int)g.data.size(), true);
			bs.SetWriteOffset(g.bits);
			ArzExtra::SendTo(playerid, bs);
		}
		for (const std::string& file : globalMaps) SendMap(playerid, file);
		for (const auto& [key, side] : portals) SendPortalSide(key, playerid);
		if (!walkStyle[playerid].empty()) SendWalkStyle(playerid, playerid);
		if (!skinMaterials[playerid].empty()) SendSkinMaterials(playerid, playerid);
	}

	inline void PlayerLeft(int playerid)
	{
		if (playerid < 0 || playerid >= MAX_PLAYERS) return;
		walkStyle[playerid].clear();
		skinMaterials[playerid].clear();
		lightMode[playerid] = 0;
	}

	// ---- нитро Arizona на машине (видят все, кто видит машину)
	struct NitroRefill { int carid, playerid; int64_t at; };
	inline std::vector<NitroRefill> nitroRefill;

	// деталь нитро x10 (ScmEvent 96, событие 2 - как AddVehicleComponent), когда подошло время
	inline void RefillNitro()
	{
		if (nitroRefill.empty()) return;
		const int64_t now = Native::SteadyMs();
		for (size_t i = 0; i < nitroRefill.size();)
		{
			const NitroRefill r = nitroRefill[i];
			if (r.at > now) { i++; continue; }
			nitroRefill.erase(nitroRefill.begin() + i);
			if (!vTemp[r.carid].vNitro || !Platform::IsPlayerConnected(r.playerid)) continue;
			BitStream bs;
			bs.Write((uint16_t)0xFFFF);
			bs.Write((uint32_t)2);
			bs.Write((uint32_t)r.carid);
			bs.Write((uint32_t)1010);
			bs.Write((uint32_t)0);
			Native::MBSendRPC(r.playerid, 96, &bs, 1, 9);
		}
	}
	// 220:156 [u16 машина][bit] - нитро-система вкл; 220:151 [u16 машина][u8 0..3] - цвет пламени
	inline void SendVehicleNitro(int carid, int to = -1)
	{
		BitStream on;
		ArzExtra::Begin(on, 156);
		on.Write((uint16_t)carid);
		on.Write(vTemp[carid].vNitro);
		BitStream color;
		const bool hasColor = vTemp[carid].vNitroColor >= 0;
		if (hasColor)
		{
			ArzExtra::Begin(color, 151);
			color.Write((uint16_t)carid);
			color.Write((uint8_t)vTemp[carid].vNitroColor);
		}
		// 156 у клиента обнуляет заряды нитро GTA, а без них нитро не включается (ни обычное, ни цветное)
		// Поэтому через полсекунды снова ставим деталь нитро x10 (RefillNitro): пакет 220 и RPC идут разными
		// потоками, и деталь, отправленная сразу, приходила
		auto send = [&](int i)
		{
			ArzExtra::SendTo(i, on);
			if (vTemp[carid].vNitro) nitroRefill.push_back({ carid, i, Native::SteadyMs() + 500 });
			if (hasColor) ArzExtra::SendTo(i, color);
		};
		if (to != -1) { send(to); return; }
		for (int i : connectedPlayers)
			if (Platform::IsVehicleStreamedIn(carid, i)) send(i);
	}

	// 220:150 [u16 машина][bit] - двигатель без звука (остальные звуки машины остаются)
	inline void SendSilentEngine(int carid, int to = -1)
	{
		BitStream bs;
		ArzExtra::Begin(bs, 150);
		bs.Write((uint16_t)carid);
		bs.Write(vTemp[carid].vSilentEngine);
		if (to != -1) { ArzExtra::SendTo(to, bs); return; }
		for (int i : connectedPlayers)
			if (Platform::IsVehicleStreamedIn(carid, i)) ArzExtra::SendTo(i, bs);
	}

	// 220:193 [u16 машина][str8 текстура] - текстура фар (модуль VehicleHeadlightTexture), пусто - стандартная
	inline void SendHeadlightTexture(int carid, int to = -1)
	{
		BitStream bs;
		ArzExtra::Begin(bs, 193);
		bs.Write((uint16_t)carid);
		ArzExtra::WriteStr8(bs, vTemp[carid].vHeadlightTexture.c_str());
		if (to != -1) { ArzExtra::SendTo(to, bs); return; }
		for (int i : connectedPlayers)
			if (Platform::IsVehicleStreamedIn(carid, i)) ArzExtra::SendTo(i, bs);
	}

	// ---- пакеты одному игроку (playerid -1 - всем, и запомнить для новых)
	inline void SendBit(int playerid, uint8_t sub, bool value)
	{
		BitStream bs;
		ArzExtra::Begin(bs, sub);
		bs.Write(value);
		if (playerid == -1) RememberGlobal(sub, bs);
		ArzExtra::SendTo(playerid, bs);
	}

	inline void SendU8(int playerid, uint8_t sub, uint8_t value)
	{
		BitStream bs;
		ArzExtra::Begin(bs, sub);
		bs.Write(value);
		if (playerid == -1) RememberGlobal(sub, bs);
		ArzExtra::SendTo(playerid, bs);
	}


	inline void WriteStr32(BitStream& bs, const std::string& s)
	{
		bs.Write((uint32_t)s.size());
		if (!s.empty()) bs.Write(s.data(), (int)s.size());
	}
}
