#pragma once
// Детали тюнинга кастомных машин Arizona (simple-модели, ScmEvent 5/6) и их слоты

#include <cstdint>

// слоты SetVehicleTuningPart (VEHICLE_PART_* в ModGear_Vehicle.inc)
enum VehiclePartSlot
{
	PART_AUTO = 0,          // определить по детали
	PART_HOOD = 1,
	PART_SPOILER = 2,
	PART_EXHAUST = 3,
	PART_WHEELS = 4,        // обычная деталь GTA (диски), не simple
	PART_FRONT_BUMPER = 5,
	PART_REAR_BUMPER = 6,
	PART_ROOF = 7,
	PART_WING_LEFT = 8,
	PART_WING_RIGHT = 9,
	PART_HOOD_LEFT = 10,
	PART_HOOD_RIGHT = 11,
	PART_SLOTS = 12
};

struct SimplePart { uint16_t id; uint8_t slot; };
extern const SimplePart simpleParts[];
extern const int simplePartsCount;

// слот simple-детали по её id (NamedModels.json), 0 - такой детали в клиенте нет
inline int SimplePartSlot(int id)
{
	int lo = 0, hi = simplePartsCount - 1;
	while (lo <= hi)
	{
		const int mid = (lo + hi) / 2;
		if (simpleParts[mid].id == id) return simpleParts[mid].slot;
		if (simpleParts[mid].id < id) lo = mid + 1; else hi = mid - 1;
	}
	return 0;
}
