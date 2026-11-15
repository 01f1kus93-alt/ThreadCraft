#pragma once
#include "core/Config.hpp"
#include "core/Types.hpp"
#include <cmath>
#include <cstdint>

namespace mc::math {

// ─────────────── floor-деление для отрицательных координат ───────────────
// Стандартный / в C++ округляет к нулю, а нам нужно к -inf.
inline int floorDiv(int a, int b) noexcept {
    int q = a / b;
    int r = a % b;
    if (r != 0 && ((r < 0) != (b < 0))) --q;
    return q;
}

// ─────────────── Мир → чанк ───────────────
inline ChunkCoord worldToChunk(int wx, int wy, int wz) noexcept {
    return {
        floorDiv(wx, config::CHUNK_SIZE_X),
        floorDiv(wy, config::CHUNK_SIZE_Y),
        floorDiv(wz, config::CHUNK_SIZE_Z)
    };
}

inline ChunkCoord worldToChunk(const Vector3& p) noexcept {
    return worldToChunk(
        (int)std::floor(p.x),
        (int)std::floor(p.y),
        (int)std::floor(p.z)
    );
}

// ─────────────── Мир → локальные координаты внутри чанка ───────────────
inline BlockCoord worldToLocal(int wx, int wy, int wz) noexcept {
    return {
        ((wx % config::CHUNK_SIZE_X) + config::CHUNK_SIZE_X) % config::CHUNK_SIZE_X,
        ((wy % config::CHUNK_SIZE_Y) + config::CHUNK_SIZE_Y) % config::CHUNK_SIZE_Y,
        ((wz % config::CHUNK_SIZE_Z) + config::CHUNK_SIZE_Z) % config::CHUNK_SIZE_Z
    };
}

// ─────────────── Локальная координата → плоский индекс ───────────────
inline int localIndex(int x, int y, int z) noexcept {
    return (y * config::CHUNK_SIZE_Z + z) * config::CHUNK_SIZE_X + x;
}

// ─────────────── Упаковка ChunkCoord в int64 (для unordered_map<int64,...>) ───────────────
inline int64_t chunkKey(const ChunkCoord& c) noexcept {
    constexpr int64_t MASK = 0x1FFFFF;
    return ((int64_t)(c.x & MASK) << 42)
         | ((int64_t)(c.y & MASK) << 21)
         |  int64_t(c.z & MASK);
}

// ─────────────── Квадрат расстояния между чанками (для приоритезации задач) ───────────────
inline int chunkDistanceSq(const ChunkCoord& a, const ChunkCoord& b) noexcept {
    int dx = a.x - b.x;
    int dy = a.y - b.y;
    int dz = a.z - b.z;
    return dx*dx + dy*dy + dz*dz;
}

} // namespace mc::math