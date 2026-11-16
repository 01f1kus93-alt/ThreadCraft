#pragma once
#include <cstdint>
#include <cstddef>
#include <functional>
#include <raylib.h>

namespace mc {

// ─────────────── BlockID ───────────────
using BlockID = uint8_t;
inline constexpr BlockID BLOCK_AIR = 0;

// ─────────────── Координаты ───────────────
struct ChunkCoord {
    int32_t x{0}, y{0}, z{0};
    bool operator==(const ChunkCoord&) const = default;
};

struct BlockCoord {
    int x{0}, y{0}, z{0};
};

// ─────────────── Режим игры ───────────────
enum class GameMode : uint8_t { Survival, Creative };

// ─────────────── Грани блока ───────────────
enum class Face : uint8_t {
    NegX = 0, PosX = 1,
    NegY = 2, PosY = 3,
    NegZ = 4, PosZ = 5,
};

inline constexpr Face ALL_FACES[6] = {
    Face::NegX, Face::PosX,
    Face::NegY, Face::PosY,
    Face::NegZ, Face::PosZ,
};

inline Vector3 faceNormal(Face f) noexcept {
    switch (f) {
        case Face::NegX: return { -1.0f,  0.0f,  0.0f };
        case Face::PosX: return {  1.0f,  0.0f,  0.0f };
        case Face::NegY: return {  0.0f, -1.0f,  0.0f };
        case Face::PosY: return {  0.0f,  1.0f,  0.0f };
        case Face::NegZ: return {  0.0f,  0.0f, -1.0f };
        case Face::PosZ: return {  0.0f,  0.0f,  1.0f };
    }
    return { 0.0f, 0.0f, 0.0f };
}

} // namespace mc

// ─────────────── std::hash<ChunkCoord> ───────────────
// Позволяет использовать ChunkCoord как ключ в unordered_map
namespace std {
template<>
struct hash<mc::ChunkCoord> {
    size_t operator()(const mc::ChunkCoord& c) const noexcept {
        constexpr int64_t MASK = 0x1FFFFF; // 21 бит на ось = ±1M чанков
        int64_t v = ((int64_t)(c.x & MASK) << 42)
                  | ((int64_t)(c.y & MASK) << 21)
                  |  (int64_t)(c.z & MASK);
        return size_t(v);
    }
};
} // namespace std