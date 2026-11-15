#pragma once
#include <cstdint>

namespace mc::config {

// ─────────────── Chunk ───────────────
inline constexpr int CHUNK_SIZE_X = 16;
inline constexpr int CHUNK_SIZE_Y = 256;
inline constexpr int CHUNK_SIZE_Z = 16;
inline constexpr int CHUNK_VOLUME = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

// ─────────────── World ───────────────
inline constexpr int  RENDER_DISTANCE        = 24;
inline constexpr int  GENERATION_DISTANCE    = 24;
inline constexpr int  RENDER_DISTANCE_UNLOAD = 24;
inline constexpr int  SEED                   = 1337;

// ─────────────── Terrain ───────────────
inline constexpr int   TERRAIN_BASE_HEIGHT = 64;
inline constexpr float TERRAIN_AMPLITUDE   = 32.0f;
inline constexpr float TERRAIN_NOISE_SCALE = 0.02f;
inline constexpr int   SEA_LEVEL           = 55;

// ─────────────── Threading ───────────────
inline constexpr int MAX_WORKER_THREADS = 20;

// ─────────────── Rendering / Uploads ───────────────
inline constexpr int   MAX_UPLOADS_PER_FRAME = 64;
inline constexpr float MAX_UPLOAD_TIME_MS    = 2.0f;
inline constexpr float UPDATE_INTERVAL_SEC   = 0.2f;

// ─────────────── Meshing ───────────────
inline constexpr int MAX_MERGE = 128;  // Максимальный размер слияния граней в greedy meshing.
                                      // Меньшие значения = меньше вершин, но больше draw calls.
                                      // Большие значения = меньше полигонов, но могут создать
                                      // видимые швы на стыках чанков. Не более 255 (unsigned char).

// ─────────────── Texture Atlas ───────────────
inline constexpr int ATLAS_TILE_SIZE = 16;   // px per tile
inline constexpr int ATLAS_COLUMNS   = 16;   // tiles per row
inline constexpr int ATLAS_ROWS      = 16;   // tiles per column
inline constexpr int ATLAS_TILE_PAD  = 8;    // Texture-bleeding padding (px). Pitch = 16+2*8=32 is a
                                              // power of two, so glGenerateMipmap's box filter never straddles
                                              // tile boundaries, preventing edge-mirror artefacts up to
                                              // mip level 3 (2×2 px tile → 1 px pad → 0.5 px sub-texel).

} // namespace mc::config