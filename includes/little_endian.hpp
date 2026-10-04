#include "types.hpp"
#pragma once
// get little-endian
inline u16	getU16(const u8 *p) {
	return static_cast<u16>(p[0] | p[1] << 8);
}

inline u32	getU32(const u8 *p) {
	return (
		static_cast<u32>(p[0])
		| static_cast<u32>(p[1]) << 8
		| static_cast<u32>(p[2]) << 16
		| static_cast<u32>(p[3]) << 24
	);
}

inline u64	getU64(const u8 *p) {
	return (
		static_cast<u64>(getU32(p))
		| static_cast<u64>(getU32(p + 4)) << 32
	);
}

