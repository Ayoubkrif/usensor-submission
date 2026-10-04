#include "Event.hpp"
#include "types.hpp"
#include "little_endian.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

namespace
{

constexpr size_t	HEADER_SIZE = 16;
constexpr size_t	RECORD_SIZE = 32;
//constexpr size_t	CHUNK_SIZE = 1024;	// batch de 32 records de 32 octets

// checksum func
u32	fnv1a(const u8 *data, size_t size)
{
	u32	h = 2166136261u;

	for (size_t i = 0; i < size; ++i)
		h = (h ^ data[i]) * 16777619u;
	return h;
}

// Header:
// 
// |"USENS001"	|record_size|version|checksum	|
// |8 u8		|u16		|u16	|u32		|
bool	isValidHeader(const u8 *h)
{
	return (
		std::memcmp(h, "USENS001", 8) == 0
		&& getU16(h + 8) == RECORD_SIZE
		&& getU16(h + 10) == 1
		&& getU32(h + 12) == fnv1a(h, 12)
	);
}

// nullptr si mauvais checksum OU flag invalide OU capteur inconnu
// remplis le minimum syndical pour le moment
std::unique_ptr<Event>	parseRecord(const u8 *r)
{
	if (getU32(r + 28) != fnv1a(r, 28) || !(r[13] & 1))
		return nullptr;

	const u64	ts = getU64(r);

	// capture le sensor ID
	switch (static_cast<EventType>(r[12]))
	{
	case EventType::Camera: return std::make_unique<CameraEvent>(ts);
	case EventType::Imu: return std::make_unique<ImuEvent>(ts);
	case EventType::Gps: return std::make_unique<GpsEvent>(ts);
	case EventType::Temp: return std::make_unique<TempEvent>(ts);
	case EventType::Button: return std::make_unique<ButtonEvent>(ts);
	}
	return nullptr;
}

} // namespace

int	main(int argc, char **argv)
{
	(void)argc, (void)argv;
	u8	header[HEADER_SIZE];
	if (std::fread(header, 1, HEADER_SIZE, stdin) != HEADER_SIZE || !isValidHeader(header))
	{
		std::cerr << "checker: invalid header\n";
		return 1;
	}
	return (0);
}
