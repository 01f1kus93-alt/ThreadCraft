#pragma once
#include <cstdint>

namespace mc::config {

// ─────────────── Chunk ───────────────
inline constexpr int CHUNK_SIZE_X = 16;
inline constexpr int CHUNK_SIZE_Y = 256;
inline constexpr int CHUNK_SIZE_Z = 16;
inline constexpr int CHUNK_VOLUME = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

// ─────────────── Subchunks (16×16×16) ───────────────
// Чанк хранится как массив из CHUNK_SUBSECTIONS секций; секция, целиком
// состоящая из воздуха, не выделяется вовсе (nullptr), что экономит ОЗУ
// для верхних пустых слоёв чанка.
inline constexpr int SUBSECTION_SIZE_Y = 16;
inline constexpr int CHUNK_SUBSECTIONS = CHUNK_SIZE_Y / SUBSECTION_SIZE_Y; // 16
inline constexpr int SUBSECTION_VOLUME = CHUNK_SIZE_X * SUBSECTION_SIZE_Y * CHUNK_SIZE_Z; // 4096

// ─────────────── World ───────────────
inline constexpr int  RENDER_DISTANCE        = 6;   // Значение по умолчанию; меняется рантайм-слайдером
inline constexpr int  GENERATION_DISTANCE    = RENDER_DISTANCE + 1;   // legacy, заменено m_renderDistance
inline constexpr int  RENDER_DISTANCE_UNLOAD = RENDER_DISTANCE;       // legacy, заменено m_renderDistance + 2
inline constexpr int  RENDER_DISTANCE_MIN    = 4;
inline constexpr int  RENDER_DISTANCE_MAX    = 96;
inline constexpr int  SEED                   = 1337;

// ─────────────── Terrain ───────────────
inline constexpr int   TERRAIN_BASE_HEIGHT = 64;
inline constexpr float TERRAIN_AMPLITUDE   = 16.0f;
inline constexpr float TERRAIN_NOISE_SCALE = 0.02f;
inline constexpr int   SEA_LEVEL           = 55;

// Минимальная высота рельефа (округлённая вниз после нормализации FBm-шума).
// Используется для culling'а подземных чанков на дальних дистанциях.
inline constexpr int   TERRAIN_MIN_HEIGHT  = TERRAIN_BASE_HEIGHT - (int)(TERRAIN_AMPLITUDE * 1.6f); // 64 - 25 = 39

// ─────────────── Threading ───────────────
inline constexpr int MAX_WORKER_THREADS = 96;

// ─────────────── Лимиты очередей (защита ОЗУ при быстром полёте) ───────────────
inline constexpr int MAX_PENDING_GENERATION = 512; // чанков в генерации одновременно
inline constexpr int MAX_PENDING_MESHES     = 384; // задач мешинга в очереди
inline constexpr int MAX_UPLOAD_QUEUE       = 1024; // мешей, ожидающих аплоада на GPU

// ─────────────── Rendering / Uploads ───────────────
inline constexpr int   MAX_UPLOADS_PER_FRAME = 256;
inline constexpr float MAX_UPLOAD_TIME_MS    = 4.0f;
inline constexpr float UPDATE_INTERVAL_SEC   = 0.2f;
inline constexpr float FOG_START_FRACTION    = 0.5f;

// Свыше этой дистанции (в чанках) листва рендерится плоским непрозрачным
// зелёным цветом вместо текстуры с alpha-discard — это убирает лаги от листвы.
inline constexpr int LEAVES_LOD_DISTANCE_CHUNKS = 16;

// Сколько слоёв листвы от внешней границы кроны остаются «дырявыми»
// (с alpha-discard). Более глубокие слои рисуются непрозрачным, чуть более
// тёмным зелёным — так крона перестаёт просвечивать насквозь.
inline constexpr int LEAVES_VISIBLE_LAYERS_MIN     = 1;
inline constexpr int LEAVES_VISIBLE_LAYERS_MAX     = 8;
inline constexpr int LEAVES_VISIBLE_LAYERS_DEFAULT = 1;

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

// ─────────────── Inventory ───────────────
inline constexpr int STACK_MAX = 64;   // max items per stack

// ─────────────── Block breaking ───────────────
inline constexpr float BREAK_TIME_MULT = 1.2f;   // break time = hardness * BREAK_TIME_MULT (seconds)

// ─────────────── Particles ───────────────
inline constexpr float PARTICLE_GRAVITY = -18.0f;
inline constexpr int   MAX_PARTICLES    = 2048;

// ─────────────── Item entities ───────────────
inline constexpr float ITEM_GRAVITY      = -20.0f;
inline constexpr float ITEM_PICKUP_R     = 1.5f;
inline constexpr float ITEM_PICKUP_DELAY = 0.35f;
inline constexpr int   MAX_ITEM_ENTITIES = 256;

// ─────────────── Hotbar ───────────────
inline constexpr int HOTBAR_SLOTS = 9;

} // namespace mc::config