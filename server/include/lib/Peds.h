#pragma once
// Педы Arizona (libPED в libcef.asi): охранники, NPC. Сырой пакет 221: [0xDD][u16 rpc][u16 pedId][поля]
// Как это работает у клиента: позицию педа сервер НЕ рассылает - каждый клиент сам ведёт своего педа

#include <lib/InfoServer.h>
#include <lib/Native.h>
#include <raknet/bitstream.h>
#include "../../Platform.h"
#include "../../Plugin.h"
#include "../../DebugLog.h"
#include "../../Afk.h"
#include <bitset>
#include <cmath>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

namespace Peds
{
	constexpr int MAX_PEDS = 1000;
	constexpr int ATTACH_SLOTS = 10;
	constexpr uint8_t PKT = 221;

	enum Rpc : uint16_t
	{
		RPC_ADD = 50, RPC_REMOVE = 51, RPC_ONFOOT_SYNC = 52, RPC_RECEIVE_SYNC = 53, RPC_COLOR = 54,
		RPC_INVULNERABLE = 56, RPC_NAME = 57, RPC_SKIN = 64, RPC_WEAPON = 65, RPC_POS = 66, RPC_MOVE = 67,
		RPC_SHOOT_POS = 68, RPC_ANIM = 69, RPC_SHOOT_PLAYER = 72, RPC_DAMAGE = 73, RPC_ATTACK_PLAYER = 80,
		RPC_ENTER_VEHICLE = 81, RPC_PASSENGER = 82, RPC_DRIVER = 83, RPC_EXIT_VEHICLE = 84, RPC_BUBBLE = 85,
		RPC_ATTACH = 86, RPC_DETACH = 87, RPC_ANGLE = 89, RPC_STOP = 96, RPC_SHOOT_PED = 97, RPC_ANIM_GROUP = 98,
		RPC_ATTACK_PED = 101, RPC_COLLISION = 102, RPC_HEALTH = 105, RPC_ARMOUR = 112, RPC_SETTINGS = 113
	};

	struct Attach
	{
		bool used = false;
		int model = 0, bone = 0;
		float off[3] = {}, rot[3] = {}, scale[3] = { 1.0f, 1.0f, 1.0f };
		uint32_t color1 = 0, color2 = 0;
	};

	struct Ped
	{
		bool used = false;
		void* script = nullptr;                      // скрипт, который создал педа: выгрузили (gmx) - пед удаляется
		int model = 0;
		float x = 0, y = 0, z = 0, angle = 0;         // угол в градусах
		float health = 100.0f, maxHealth = 100.0f, armour = 0.0f, maxArmour = 100.0f;
		bool invulnerable = false, dead = false, collision = true;
		int weapon = 0;
		uint32_t color = 0xFFFFFFFF, subColor = 0xFFFFFFFF;
		std::string name, subName, animGroup;
		int vehicle = 0, seat = -1;                   // vehicle 0 - пешком; seat как в SA-MP: 0 водитель, 1 передний пассажир, 2-3 задние
		int16_t clientSeat = 0;                       // место, отправленное клиенту (-1 - первое свободное пассажирское)
		bool entering = false;                        // садится с анимацией (ещё не внутри)
		uint32_t enterEnd = 0;                        // когда посадка точно кончилась (мс)
		float enterX = 0, enterY = 0, enterZ = 0;     // где стояла машина в начале посадки
		bool autoVehicle = false;                     // сел сам за хозяином - выйдет, когда выйдет хозяин
		int ownerVehicle = 0;                         // в какой машине последний раз был хозяин (SetPedFollowPlayer)
		bool stayOut = false;                         // высадили PedExitVehicle, пока хозяин в машине - не садиться снова
		int vehicleMode = 1;                          // PED_VEHICLE_*: садиться ли за хозяином (SetPedFollowVehicle)
		int enterTime = 8000;                         // сколько даётся на посадку с анимацией, потом - сразу на место
		float accuracy = 40.0f;                       // % попаданий по игроку на средней дистанции (SetPedAccuracy)
		uint32_t nextShot = 0;                        // когда следующий выстрел по игроку (мс)
		int world = 0, interior = 0;
		float streamDistance = 0.0f;                  // 0 - общая (SetPedStreamDistance)
		Attach attach[ATTACH_SLOTS];

		int owner = -1;                               // чьи позиции принимаем
		std::bitset<MAX_PLAYERS> streamed;
		std::unordered_map<int, uint32_t> lastFix;    // игрок -> время последней поправки (мс)

		bool moving = false;
		float tx = 0, ty = 0, tz = 0;                 // цель MovePedToPos
		int moveType = 6;

		int follow = -1;                              // охранник: за кем идти
		// текущая атака: 0 - нет, 1 - игрока, 2 - педа. Хранится, чтобы дослать тем, у кого пед появился позже
		int attackType = 0, attackTarget = -1, attackTime = -1;
		float followDistance = 3.0f;
		uint32_t lastOwnerSync = 0;
	};

	inline Ped peds[MAX_PEDS];
	inline float streamDistance = 150.0f;
	inline int syncRate = 100;                        // мс между 53 от клиента (SetSettings)
	inline std::bitset<MAX_PLAYERS> settingsSent;
	inline uint32_t clockMs = 0;                      // время от таймера (мс)
	inline int count = 0;                             // педов на сервере (0 - модуль спит)

	// игроки с лаунчером на этот тик: позиция/мир/интерьер берём один раз, а не на каждого педа
	struct Viewer { int id; float x, y, z; int world, interior; };
	inline std::vector<Viewer> viewers;

	inline bool Valid(int id) { return id >= 0 && id < MAX_PEDS && peds[id].used; }
	inline float Dist(float ax, float ay, float az, float bx, float by, float bz)
	{
		const float dx = ax - bx, dy = ay - by, dz = az - bz;
		return std::sqrt(dx * dx + dy * dy + dz * dz);
	}

	// ---- запись пакетов
	inline void Head(BitStream& bs, uint16_t rpc, uint16_t pedid)
	{
		bs.Write(PKT);
		bs.Write(rpc);
		bs.Write(pedid);
	}
	inline void Str32(BitStream& bs, const std::string& s)
	{
		bs.Write((uint32_t)s.size());
		if (!s.empty()) bs.Write(s.c_str(), (int)s.size());
	}
	inline void Str8(BitStream& bs, const std::string& s)
	{
		const size_t n = s.size() > 255 ? 255 : s.size();
		bs.Write((uint8_t)n);
		if (n) bs.Write(s.c_str(), (int)n);
	}
	inline void Send(int playerid, BitStream& bs) { Native::MBSendPacket(playerid, &bs, 1, 9); }
	// всем, у кого пед в стриме (except - кроме этого игрока)
	inline void SendStreamed(int id, BitStream& bs, int except = -1)
	{
		const Ped& p = peds[id];
		for (int i = 0; i < MAX_PLAYERS; i++)
			if (p.streamed.test(i) && i != except) Send(i, bs);
	}

	// место SA-MP (0 водитель, 1 передний пассажир, 2-3 задние) -> место клиента: 0 водитель, иначе пассажирское место GTA (с 0)
	// переднее пассажирское (GTA 0) клиенту не передать - шлём -1 (первое свободное пассажирское: GTA сама знает
	// сколько мест у модели, поэтому в 2-местной машине пед не сядет на несуществующее заднее место и не пропадёт)
	inline int16_t ClientSeat(int seat) { return (int16_t)(seat <= 0 ? 0 : seat == 1 ? -1 : seat - 1); }

	inline void WriteAdd(BitStream& bs, int id)
	{
		const Ped& p = peds[id];
		Head(bs, RPC_ADD, (uint16_t)id);
		bs.Write((uint16_t)p.model);
		bs.Write(p.x); bs.Write(p.y); bs.Write(p.z);
		bs.Write(p.angle);
		bs.Write(p.invulnerable);
		bs.Write(p.dead ? 0.0f : p.health);
		bs.Write(p.armour);
		bs.Write((uint16_t)p.weapon);
		bs.Write((uint8_t)0);
		bs.Write(p.color);
		Str32(bs, p.name);
		bs.Write((uint8_t)0);
		bs.Write(p.subColor);
		Str32(bs, p.subName);
		bs.Write((uint16_t)(p.vehicle > 0 ? p.vehicle : 0));
		bs.Write((uint16_t)(p.vehicle > 0 ? p.clientSeat : 0xFFFF));
		bs.Write(p.collision);
		Str8(bs, p.animGroup);
	}

	inline void SendSettings(int playerid)
	{
		BitStream bs;
		bs.Write(PKT);
		bs.Write((uint16_t)RPC_SETTINGS);
		bs.Write((uint16_t)syncRate);
		Send(playerid, bs);
		settingsSent.set(playerid);
	}

	inline void SendMove(int id, int to)
	{
		const Ped& p = peds[id];
		BitStream bs;
		Head(bs, RPC_MOVE, (uint16_t)id);
		bs.Write(p.tx); bs.Write(p.ty); bs.Write(p.tz);
		bs.Write((uint16_t)p.moveType);
		bs.Write((int32_t)-1);
		if (to >= 0) Send(to, bs); else SendStreamed(id, bs);
	}

	inline void SendAttack(int id, int to)
	{
		const Ped& p = peds[id];
		if (!p.attackType) return;
		BitStream bs;
		Head(bs, p.attackType == 1 ? RPC_ATTACK_PLAYER : RPC_ATTACK_PED, (uint16_t)id);
		bs.Write((uint16_t)p.attackTarget);
		bs.Write((int32_t)p.attackTime);
		if (to >= 0) Send(to, bs); else SendStreamed(id, bs);
	}

	inline void ClearAttack(int id) { peds[id].attackType = 0; peds[id].attackTarget = -1; }

	// атака кончилась сама (цель ушла): остановить педа у клиентов - охранник дальше идёт за хозяином
	inline void EndAttack(int id)
	{
		if (!peds[id].attackType) return;
		ClearAttack(id);
		BitStream bs;
		Head(bs, RPC_STOP, (uint16_t)id);
		SendStreamed(id, bs);
	}

	inline void SendAttach(int id, int slot, int to)
	{
		const Attach& a = peds[id].attach[slot];
		BitStream bs;
		if (!a.used)
		{
			Head(bs, RPC_DETACH, (uint16_t)id);
			bs.Write((uint16_t)slot);
		}
		else
		{
			Head(bs, RPC_ATTACH, (uint16_t)id);
			bs.Write((uint16_t)slot);
			bs.Write((uint32_t)a.model);
			bs.Write((uint16_t)a.bone);
			for (float v : a.off) bs.Write(v);
			for (float v : a.rot) bs.Write(v);
			for (float v : a.scale) bs.Write(v);
			bs.Write(a.color1);
			bs.Write(a.color2);
		}
		if (to >= 0) Send(to, bs); else SendStreamed(id, bs);
	}

	// ---- машины
	// место занято игроком или другим педом
	inline bool SeatTaken(int vehicle, int seat, int exceptPed)
	{
		for (int i : connectedPlayers) // только подключённые, а не все 1000 слотов
		{
			int s = -1;
			if (Platform::GetPlayerVehicle(i, &s) == vehicle && s == seat) return true;
		}
		for (int id = 0; id < MAX_PEDS; id++)
			if (id != exceptPed && peds[id].used && !peds[id].dead && peds[id].vehicle == vehicle && peds[id].seat == seat) return true;
		return false;
	}
	// первое свободное пассажирское место (1..3) или -1. Сколько мест у модели, сервер не знает
	// клиенту в этом случае уходит "любое свободное", а здесь - номер для GetPedVehicle
	inline int FreeSeat(int vehicle, int exceptPed)
	{
		for (int s = 1; s <= 3; s++) if (!SeatTaken(vehicle, s, exceptPed)) return s;
		return -1;
	}
	inline void SendEnter(int id, bool warp, int to)
	{
		const Ped& p = peds[id];
		BitStream bs;
		Head(bs, RPC_ENTER_VEHICLE, (uint16_t)id);
		bs.Write((uint16_t)p.vehicle);
		bs.Write(p.clientSeat);
		bs.Write((int32_t)(warp ? 0 : p.enterTime)); // 0 - сразу на место, иначе с анимацией (по таймауту клиент сажает сам)
		if (to >= 0) Send(to, bs); else SendStreamed(id, bs);
	}
	inline void EnterVehicle(int id, int vehicle, int seat, bool anySeat, bool warp, bool autoV)
	{
		Ped& p = peds[id];
		p.vehicle = vehicle;
		p.seat = seat;
		p.clientSeat = anySeat && seat > 0 ? (int16_t)-1 : ClientSeat(seat);
		p.moving = false;
		p.autoVehicle = autoV;
		p.stayOut = false;
		ClearAttack(id);
		float vx = p.x, vy = p.y, vz = p.z;
		Platform::GetVehiclePos(vehicle, vx, vy, vz);
		// далеко от машины - бежать долго, машина уедет: сразу на место
		if (Dist(vx, vy, vz, p.x, p.y, p.z) > 25.0f) warp = true;
		p.entering = !warp;
		p.enterX = vx; p.enterY = vy; p.enterZ = vz;
		p.enterEnd = clockMs + (uint32_t)p.enterTime + 400;
		SendEnter(id, warp, -1);
		if (warp) Plugin::CallPublic("OnPedEnterVehicle", { id, vehicle, seat });
	}
	inline void ExitVehicle(int id, bool notify = true)
	{
		Ped& p = peds[id];
		if (p.vehicle <= 0) return;
		const int vehicle = p.vehicle;
		p.vehicle = 0; p.seat = -1; p.clientSeat = 0;
		p.entering = false; p.autoVehicle = false;
		BitStream bs;
		Head(bs, RPC_EXIT_VEHICLE, (uint16_t)id);
		SendStreamed(id, bs);
		if (notify) Plugin::CallPublic("OnPedExitVehicle", { id, vehicle });
	}

	// ---- стрим
	inline void StreamIn(int id, int playerid)
	{
		Ped& p = peds[id];
		if (!settingsSent.test(playerid)) SendSettings(playerid);
		BitStream bs;
		WriteAdd(bs, id);
		Send(playerid, bs);
		for (int s = 0; s < ATTACH_SLOTS; s++) if (p.attach[s].used) SendAttach(id, s, playerid);
		if (p.moving && p.vehicle <= 0) SendMove(id, playerid);
		p.streamed.set(playerid);
		if (p.attackType && !p.dead) SendAttack(id, playerid);
		Plugin::CallPublic("OnPedStreamIn", { id, playerid });
	}

	inline void StreamOut(int id, int playerid, bool notify = true)
	{
		Ped& p = peds[id];
		if (!p.streamed.test(playerid)) return;
		p.streamed.reset(playerid);
		p.lastFix.erase(playerid);
		if (p.owner == playerid) p.owner = -1;
		if (notify && Platform::IsPlayerConnected(playerid))
		{
			BitStream bs;
			Head(bs, RPC_REMOVE, (uint16_t)id);
			Send(playerid, bs);
		}
		Plugin::CallPublic("OnPedStreamOut", { id, playerid });
	}

	inline bool ShouldSee(const Ped& p, const Viewer& v)
	{
		if (v.world != p.world || v.interior != p.interior) return false;
		const float limit = p.streamDistance > 0.0f ? p.streamDistance : streamDistance;
		return Dist(v.x, v.y, v.z, p.x, p.y, p.z) <= limit;
	}

	// ближайший игрок из тех, кто видит педа - он "владелец" синхронизации
	inline void PickOwner(int id)
	{
		Ped& p = peds[id];
		int best = -1;
		float bestDist = 1e9f, ownerDist = 1e9f;
		for (const Viewer& v : viewers)
		{
			if (!p.streamed.test(v.id)) continue;
			const float d = Dist(v.x, v.y, v.z, p.x, p.y, p.z);
			if (v.id == p.owner) ownerDist = d;
			if (d < bestDist) { bestDist = d; best = v.id; }
		}
		// не дёргать владельца: меняем, если новый ближе хотя бы на 10 м или старого нет
		if (p.owner < 0 || !p.streamed.test(p.owner) || bestDist + 10.0f < ownerDist) p.owner = best;
	}

	// ---- создание / удаление
	inline int Create(int model, float x, float y, float z, float angle, int world, int interior, void* script = nullptr)
	{
		for (int id = 0; id < MAX_PEDS; id++)
		{
			if (peds[id].used) continue;
			peds[id] = Ped();
			Ped& p = peds[id];
			p.used = true;
			p.script = script;
			p.model = model;
			p.x = x; p.y = y; p.z = z; p.angle = angle;
			p.world = world; p.interior = interior;
			count++;
			return id;
		}
		return -1;
	}

	inline void Destroy(int id)
	{
		if (!Valid(id)) return;
		for (int i = 0; i < MAX_PLAYERS; i++) if (peds[id].streamed.test(i)) StreamOut(id, i);
		peds[id] = Ped();
		count--;
	}

	// скрипт выгружен (gmx): его педы убираются у всех - иначе после перезапуска мода боты удваивались
	inline void FreeScript(void* script)
	{
		for (int id = 0; id < MAX_PEDS; id++)
			if (peds[id].used && peds[id].script == script) Destroy(id);
	}

	inline void ShootPlayer(int id); // ниже, в разделе урона

	// ---- таймер (200 мс): стрим, владельцы, охранники
	inline void Update()
	{
		clockMs += 200;
		if (count <= 0) return;
		viewers.clear();
		std::bitset<MAX_PLAYERS> isViewer;
		for (int i = 0; i < MAX_PLAYERS; i++)
		{
			if (!PI[i].pLauncherStat || !Platform::IsPlayerConnected(i)) continue;
			// свёрнутый игрок педов не видит: иначе их команды копились в очереди и после возврата приходили
			// пачкой устаревших. Вернётся - педы появятся у него заново, сразу в нынешнем состоянии
			if (Afk::Away(i)) continue;
			Viewer v{ i };
			if (!Platform::GetPlayerPos(i, v.x, v.y, v.z)) continue;
			v.world = Platform::GetPlayerVirtualWorld(i);
			v.interior = Platform::GetPlayerInterior(i);
			viewers.push_back(v);
			isViewer.set(i);
		}
		for (int id = 0; id < MAX_PEDS; id++)
		{
			Ped& p = peds[id];
			if (!p.used) continue;
			// в машине клиенты позицию педа не шлют (53 только пешком) - пед там, где машина
			// Без этого он оставался у места посадки и пропадал из стрима, когда машина отъезжала
			if (p.vehicle > 0)
			{
				float vx, vy, vz;
				if (!Platform::GetVehiclePos(p.vehicle, vx, vy, vz)) { p.vehicle = 0; p.seat = -1; p.entering = false; p.autoVehicle = false; } // машину удалили
				else if (p.entering)
				{
					// садится с анимацией: машина тронулась - сразу на место (иначе пед бежит за уехавшей машиной)
					// время вышло - клиент уже посадил сам
					const bool moved = Dist(vx, vy, vz, p.enterX, p.enterY, p.enterZ) > 3.0f;
					if (moved || clockMs >= p.enterEnd)
					{
						p.entering = false;
						p.x = vx; p.y = vy; p.z = vz;
						if (moved) SendEnter(id, true, -1);
						Plugin::CallPublic("OnPedEnterVehicle", { id, p.vehicle, p.seat });
					}
				}
				else { p.x = vx; p.y = vy; p.z = vz; }
			}
			for (const Viewer& v : viewers)
			{
				const bool see = ShouldSee(p, v);
				if (see && !p.streamed.test(v.id)) StreamIn(id, v.id);
				else if (!see && p.streamed.test(v.id)) StreamOut(id, v.id);
			}
			// ушли с сервера / выключили лаунчер
			if ((p.streamed & ~isViewer).any())
				for (int i = 0; i < MAX_PLAYERS; i++)
					if (p.streamed.test(i) && !isViewer.test(i)) StreamOut(id, i, Platform::IsPlayerConnected(i));
			PickOwner(id);

			// охранник: идти за игроком, держаться на followDistance
			// цель атаки ушла с сервера - атака кончилась, охранник снова идёт за хозяином
			if (p.attackType == 1 && !Platform::IsPlayerConnected(p.attackTarget)) EndAttack(id);
			if (p.attackType == 2 && (!Valid(p.attackTarget) || peds[p.attackTarget].dead)) EndAttack(id);
			if (p.attackType == 1 && !p.dead && p.vehicle <= 0) ShootPlayer(id);
			// хозяин сел в машину - пед садится к нему (SetPedFollowVehicle), вышел - выходит
			if (p.follow >= 0 && !p.dead && p.vehicleMode && Platform::IsPlayerConnected(p.follow))
			{
				const int fveh = Platform::GetPlayerVehicle(p.follow);
				if (fveh > 0) p.ownerVehicle = fveh;
				if (fveh <= 0) p.stayOut = false;
				if (fveh > 0 && p.vehicle != fveh && !p.stayOut)
				{
					const int seat = FreeSeat(fveh, id);
					if (seat > 0)
					{
						if (p.vehicle > 0) ExitVehicle(id);
						EnterVehicle(id, fveh, seat, true, p.vehicleMode == 2, true);
					}
				}
				else if (fveh <= 0 && p.vehicle > 0 && (p.autoVehicle || p.vehicle == p.ownerVehicle)) ExitVehicle(id);
			}
			if (p.follow >= 0 && !p.dead && p.vehicle <= 0 && !p.attackType)
			{
				float fx, fy, fz;
				if (!Platform::IsPlayerConnected(p.follow) || !Platform::GetPlayerPos(p.follow, fx, fy, fz)) { p.follow = -1; continue; }
				const float d = Dist(fx, fy, fz, p.x, p.y, p.z);
				const int fworld = Platform::GetPlayerVirtualWorld(p.follow), finterior = Platform::GetPlayerInterior(p.follow);
				if (d > 80.0f || fworld != p.world || finterior != p.interior)
				{
					// хозяин уехал / телепортировался - охранник за ним не успеет, переносим рядом
					p.world = fworld; p.interior = finterior;
					p.x = fx + 1.5f; p.y = fy + 1.5f; p.z = fz;
					p.moving = false;
					BitStream bs;
					Head(bs, RPC_POS, (uint16_t)id);
					bs.Write(p.x); bs.Write(p.y); bs.Write(p.z);
					SendStreamed(id, bs);
					continue; // стрим по новой позиции - на следующем тике
				}
				if (d > p.followDistance + 1.0f)
				{
					const float mt = d > 15.0f ? 7.0f : (d > 6.0f ? 6.0f : 4.0f);
					// не спамить одной и той же целью: только если игрок сместился
					if (!p.moving || Dist(fx, fy, fz, p.tx, p.ty, p.tz) > 1.5f || p.moveType != (int)mt)
					{
						p.tx = fx; p.ty = fy; p.tz = fz; p.moveType = (int)mt; p.moving = true;
						SendMove(id, -1);
					}
				}
				else if (p.moving)
				{
					p.moving = false;
					BitStream bs;
					Head(bs, RPC_STOP, (uint16_t)id);
					SendStreamed(id, bs);
				}
			}
		}
	}

	inline void PlayerLeft(int playerid)
	{
		if (playerid < 0 || playerid >= MAX_PLAYERS) return;
		settingsSent.reset(playerid);
		for (int id = 0; id < MAX_PEDS; id++)
		{
			if (!peds[id].used) continue;
			if (peds[id].streamed.test(playerid)) StreamOut(id, playerid, false);
			if (peds[id].follow == playerid) peds[id].follow = -1;
		}
	}

	// ---- урон
	inline int FloatBits(float f) { int v; std::memcpy(&v, &f, 4); return v; }

	// Выстрелы педа по игроку. SA-MP на клиенте пропускает просчёт попадания (CWeapon::FireInstantHit) только
	// для своего игрока - пули педа по игроку не попадают, и клиент о них ничего не шлёт (пед по педу - шлёт)
	// Поэтому попадания по игроку считает сервер: по оружию, темпу стрельбы, дистанции и точности педа
	struct GunInfo { int weapon; float damage, range; int interval; };
	inline const GunInfo* Gun(int weapon)
	{
		static const GunInfo guns[] = {
			{ 22, 8.25f, 35.0f, 400 }, { 23, 13.2f, 35.0f, 500 }, { 24, 46.2f, 35.0f, 900 },
			{ 25, 30.0f, 40.0f, 1100 }, { 26, 30.0f, 35.0f, 400 }, { 27, 30.0f, 40.0f, 500 },
			{ 28, 6.6f, 35.0f, 120 }, { 29, 8.25f, 45.0f, 120 }, { 30, 9.9f, 70.0f, 150 }, { 31, 9.9f, 90.0f, 150 },
			{ 32, 6.6f, 35.0f, 120 }, { 33, 24.75f, 100.0f, 1100 }, { 34, 41.25f, 100.0f, 1600 }, { 38, 46.2f, 75.0f, 60 },
		};
		for (const GunInfo& g : guns) if (g.weapon == weapon) return &g;
		return nullptr;
	}
	inline uint32_t rng = 2463534242u;
	inline float Rand01() { rng ^= rng << 13; rng ^= rng >> 17; rng ^= rng << 5; return (rng & 0xFFFFFF) / 16777216.0f; }

	inline void ShootPlayer(int id)
	{
		Ped& p = peds[id];
		const int target = p.attackTarget;
		const GunInfo* g = Gun(p.weapon);
		if (!g || p.accuracy <= 0.0f || !p.streamed.test(target) || clockMs < p.nextShot) return;
		float x, y, z;
		if (!Platform::GetPlayerPos(target, x, y, z) || Platform::GetPlayerVehicle(target) > 0) return; // в машине - пули в машину
		if (Platform::GetPlayerVirtualWorld(target) != p.world || Platform::GetPlayerInterior(target) != p.interior) return;
		const float d = Dist(x, y, z, p.x, p.y, p.z);
		if (d > g->range) return;
		p.nextShot = clockMs + (uint32_t)g->interval;
		// рядом попадает чаще, у края дальности - вдвое реже
		float chance = p.accuracy / 100.0f * (1.25f - 0.75f * d / g->range);
		if (chance > 0.95f) chance = 0.95f;
		if (Rand01() >= chance) return;
		const int bodypart = Rand01() < 0.1f ? 9 : 3; // 9 - голова, 3 - торс
		Plugin::CallPublic("OnPlayerDamageByPed", { target, id, FloatBits(g->damage), p.weapon, bodypart });
	}

	inline void Kill(int id, int killer, bool killerIsPed, int weapon)
	{
		Ped& p = peds[id];
		if (p.dead) return;
		p.dead = true;
		p.health = 0.0f;
		p.moving = false;
		p.follow = -1;
		ClearAttack(id);
		BitStream bs;
		Head(bs, RPC_HEALTH, (uint16_t)id);
		bs.Write((uint32_t)0);
		bs.Write(0.0f);
		bs.Write(p.maxHealth);
		SendStreamed(id, bs);
		Plugin::CallPublic("OnPedDeath", { id, killer, killerIsPed ? 1 : 0, weapon });
	}

	// здоровье / броня педа всем, кто его видит (полоска над головой)
	inline void SendHealthAll(int id, bool armour)
	{
		const Ped& p = peds[id];
		BitStream bs;
		Head(bs, armour ? RPC_ARMOUR : RPC_HEALTH, (uint16_t)id);
		bs.Write((uint32_t)0);
		bs.Write(armour ? p.armour : p.health);
		bs.Write(armour ? p.maxArmour : p.maxHealth);
		SendStreamed(id, bs);
	}

	inline void ApplyDamage(int id, int issuer, bool issuerIsPed, float amount, int weapon, int bodypart)
	{
		Ped& p = peds[id];
		if (p.dead || p.invulnerable || !(amount > 0.0f) || amount > 500.0f) return;
		if (!Plugin::CallPublicRet("OnPedDamage", { id, issuer, issuerIsPed ? 1 : 0, FloatBits(amount), weapon, bodypart })) return;
		const bool armourHit = p.armour > 0.0f; // броня приняла часть урона
		if (armourHit)
		{
			const float a = amount < p.armour ? amount : p.armour;
			p.armour -= a;
			amount -= a;
		}
		p.health -= amount;
		if (p.health <= 0.0f) { Kill(id, issuer, issuerIsPed, weapon); return; }
		if (armourHit) SendHealthAll(id, true);
		SendHealthAll(id, false);
	}

	// ---- пакеты от клиента
	inline void OnPacket(int playerid, BitStream& bs)
	{
		if (playerid < 0 || playerid >= MAX_PLAYERS || !PI[playerid].pLauncherStat) return;
		uint16_t rpc = 0;
		if (!bs.Read(rpc)) return;
		if (rpc == RPC_RECEIVE_SYNC)
		{
			uint16_t id = 0, aimed = 0;
			float x, y, z, heading;
			uint32_t token = 0;
			bool los = false;
			if (!bs.Read(id) || !bs.Read(x) || !bs.Read(y) || !bs.Read(z) || !bs.Read(token) || !bs.Read(los) || !bs.Read(heading)) return;
			bs.Read(aimed);
			if (!Valid(id) || !peds[id].streamed.test(playerid) || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z)) return;
			Ped& p = peds[id];
			if (p.dead || p.vehicle > 0) return;
			const float d = Dist(x, y, z, p.x, p.y, p.z);
			if (playerid == p.owner)
			{
				// владелец: принимаем позицию, если она правдоподобна (не больше ~40 м/с с прошлого раза)
				const uint32_t dt = clockMs - p.lastOwnerSync;
				const float maxMove = 5.0f + 40.0f * (dt > 2000 ? 2.0f : dt / 1000.0f);
				p.lastOwnerSync = clockMs;
				if (d > maxMove && d > 8.0f)
				{
					BitStream fix; Head(fix, RPC_POS, id); fix.Write(p.x); fix.Write(p.y); fix.Write(p.z);
					Send(playerid, fix);
					return;
				}
				p.x = x; p.y = y; p.z = z;
				if (std::isfinite(heading)) p.angle = heading * 57.29578f;
				if (p.moving && p.follow < 0 && Dist(x, y, z, p.tx, p.ty, p.tz) < 1.5f)
				{
					p.moving = false;
					Plugin::CallPublic("OnPedReachDestination", id);
				}
			}
			else if (d > 6.0f && clockMs - p.lastFix[playerid] > 1000)
			{
				// у этого клиента пед ушёл далеко от "правильного" - телепорт к позиции владельца
				p.lastFix[playerid] = clockMs;
				BitStream fix; Head(fix, RPC_POS, id); fix.Write(p.x); fix.Write(p.y); fix.Write(p.z);
				Send(playerid, fix);
				if (p.moving) SendMove(id, playerid);
			}
			return;
		}
		if (rpc == RPC_DAMAGE)
		{
			bool isTake = false;
			uint16_t id = 0, otherIsPed = 0, other = 0;
			float amount = 0.0f;
			uint32_t weapon = 0, bodypart = 0;
			if (!bs.Read(isTake) || !bs.Read(id) || !bs.Read(amount) || !bs.Read(weapon) || !bs.Read(bodypart) || !bs.Read(otherIsPed) || !bs.Read(other)) return;
			if (!Valid(id) || !peds[id].streamed.test(playerid)) return;
			if (!otherIsPed)
			{
				// урон от/по игроку засчитываем только с клиента этого игрока (остальные видят тот же удар)
				if (other != playerid) return;
				if (isTake) ApplyDamage(id, playerid, false, amount, (int)weapon, (int)bodypart);
				else Plugin::CallPublic("OnPlayerDamageByPed", { playerid, (int)id, FloatBits(amount), (int)weapon, (int)bodypart });
			}
			else if (isTake && playerid == peds[id].owner && Valid(other))
				ApplyDamage(id, other, true, amount, (int)weapon, (int)bodypart); // пед по педу - со слов владельца жертвы
		}
	}
}
