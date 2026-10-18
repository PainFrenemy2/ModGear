#pragma once
// Хендлинг машин как у Arizona: RPC 161, kind 0 - список (u8 параметр, float значение)
// Пакет: u16 машина, u8 0, u8 1, u8 0 (kind), u8 count, count x (u8 id, float)
// Параметры шлются по одному - handling.cfg у клиента свой, сервер его не читает
#include <lib/Native.h>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cctype>

namespace Handling
{
	// столбцы handling.cfg (после имени машины) и id параметра в RPC 161 Arizona (-1 - Arizona его не шлёт / не найден)
	struct Field { const char* name; const char* alias; int arz; };
	inline constexpr Field fields[] =
	{
		{ "fMass", "mass", -1 },
		{ "fTurnMass", "turnmass", -1 },
		{ "fDragMult", "drag", -1 },
		{ "fCentreOfMassX", "comx", -1 },
		{ "fCentreOfMassY", "comy", -1 },
		{ "fCentreOfMassZ", "comz", -1 },
		{ "nPercentSubmerged", "submerged", -1 },
		{ "fTractionMultiplier", "traction", 0x08 },
		{ "fTractionLoss", "tractionloss", 0x07 },
		{ "fTractionBias", "tractionbias", 0x09 },
		{ "nNumberOfGears", "gears", -1 },
		{ "fMaxVelocity", "maxspeed", 0x0A },
		{ "fEngineAcceleration", "acceleration", 0x0B },
		{ "fEngineInertia", "inertia", 0x0C },
		{ "nDriveType", "drive", -1 },
		{ "nEngineType", "engine", -1 },
		{ "fBrakeDeceleration", "brake", 0x0D },
		{ "fBrakeBias", "brakebias", 0x0E },
		{ "bABS", "abs", -1 },
		{ "fSteeringLock", "steering", 0x0F },
		{ "fSuspensionForceLevel", "suspforce", 0x10 },
		{ "fSuspensionDampingLevel", "suspdamping", 0x11 },
		{ "fSuspensionHighSpdComDamp", "susphighspd", 0x12 },
		{ "fSuspensionUpperLimit", "suspupper", 0x13 },
		{ "fSuspensionLowerLimit", "susplower", 0x14 },
		{ "fSuspensionBias", "suspbias", 0x15 },
		{ "fSuspensionAntiDive", "antidive", 0x16 },
		{ "fSeatOffsetDistance", "seatoffset", -1 },
		{ "fCollisionDamageMultiplier", "damage", -1 },
		{ "nMonetaryValue", "price", -1 },
		{ "modelFlags", "modelflags", -1 },
		{ "handlingFlags", "handlingflags", -1 },
		{ "nFrontLights", "frontlights", -1 },
		{ "nRearLights", "rearlights", -1 },
		{ "nAnimGroup", "animgroup", -1 },
	};
	inline constexpr int FIELDS = sizeof(fields) / sizeof(fields[0]);
	// параметр, который не идёт из cfg: высота подвески (его же шлёт SetVehicleHidraulics)
	inline constexpr int ARZ_SUSPENSION = 0x10;

	inline bool Same(const char* a, const char* b)
	{
		for (; *a && *b; a++, b++) if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return false;
		return *a == *b;
	}
	// индекс столбца cfg по имени ("fMaxVelocity" или "maxspeed"), иначе -1
	inline int FieldIndex(const char* name)
	{
		for (int i = 0; i < FIELDS; i++) if (Same(name, fields[i].name) || Same(name, fields[i].alias)) return i;
		return -1;
	}
	// id параметра Arizona: по имени поля, "suspension", или число ("0x0A", "10")
	inline int ArzId(const char* name)
	{
		if (Same(name, "suspension")) return ARZ_SUSPENSION;
		int f = FieldIndex(name);
		if (f != -1) return fields[f].arz;
		char* end = nullptr;
		long v = strtol(name, &end, 0);
		if (end != name && !*end && v >= 0 && v < 64) return (int)v;
		return -1;
	}
	// ---- хранение на машине и отправка
	inline void Send(int carid, const std::vector<std::pair<int, float>>& list, int playerid = -1)
	{
		if (list.empty()) return;
		BitStream arz;
		arz.Write((uint16_t)carid);
		arz.Write((uint8_t)0);
		arz.Write((uint8_t)1);
		arz.Write((uint8_t)0);
		arz.Write((uint8_t)list.size());
		for (auto& p : list) { arz.Write((uint8_t)p.first); arz.Write(p.second); }
		Native::SendPackerPlayer(arz, 161, carid, playerid);
	}
	inline void Set(int carid, const std::vector<std::pair<int, float>>& list)
	{
		for (auto& p : list) { vTemp[carid].vHandling[p.first] = p.second; vTemp[carid].vHandlingMask |= 1ULL << p.first; }
		Send(carid, list);
	}
	inline std::vector<std::pair<int, float>> Stored(int carid)
	{
		std::vector<std::pair<int, float>> list;
		for (int i = 0; i < 64; i++) if (vTemp[carid].vHandlingMask >> i & 1) list.push_back({ i, vTemp[carid].vHandling[i] });
		return list;
	}
}
