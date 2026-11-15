#include "world/WorldGenerator.hpp"
#include "world/BlockRegistry.hpp"
#include "core/Config.hpp"
#include <cmath>
#include <algorithm>

namespace mc {

namespace {

    inline uint32_t hash2(int x, int z, uint32_t seed) noexcept {
        uint32_t h = seed;
        h ^= (uint32_t)(x * 374761393);
        h ^= (uint32_t)(z * 668265263);
        h  = (h ^ (h >> 13)) * 1274126177u;
        return h ^ (h >> 16);
    }

    void placeTree(Chunk& chunk, int lx, int baseLy, int lz, uint32_t treeSeed) noexcept {
        const int trunkHeight = 4 + (treeSeed % 3);

        for (int i = 1; i <= trunkHeight; ++i) {
            const int ty = baseLy + i;
            if (ty >= 0 && ty < config::CHUNK_SIZE_Y) {
                chunk.set(lx, ty, lz, BlockRegistry::LOG);
            }
        }

        const int topY = baseLy + trunkHeight;

        struct CrownLayer { int dy; int radius; };
        constexpr CrownLayer layers[] = {
            { 2, 1 }, { 1, 1 }, { 0, 2 }, {-1, 2 },
        };

        for (const auto& layer : layers) {
            const int ly = topY + layer.dy;
            if (ly < 0 || ly >= config::CHUNK_SIZE_Y) continue;

            for (int dx = -layer.radius; dx <= layer.radius; ++dx) {
                for (int dz = -layer.radius; dz <= layer.radius; ++dz) {
                    if (dx == 0 && dz == 0 && layer.dy <= 0) continue;

                    const int nx = lx + dx;
                    const int nz = lz + dz;
                    if (nx < 0 || nx >= config::CHUNK_SIZE_X) continue;
                    if (nz < 0 || nz >= config::CHUNK_SIZE_Z) continue;
                    if (layer.radius == 2 && std::abs(dx) == 2 && std::abs(dz) == 2) continue;

                    if (std::abs(dx) == layer.radius || std::abs(dz) == layer.radius) {
                        uint32_t h = treeSeed
                            ^ ((uint32_t)(dx * 374761393u))
                            ^ ((uint32_t)(dz * 668265263u))
                            ^ ((uint32_t)(layer.dy * 2246822519u));
                        h = (h ^ (h >> 13)) * 1274126177u;
                        h ^= (h >> 16);
                        if ((h % 5) == 0) continue;
                    }

                    chunk.set(nx, ly, nz, BlockRegistry::LEAVES);
                }
            }
        }
    }

} // namespace

WorldGenerator::WorldGenerator(uint32_t seed)
    : m_seed(seed)
{
    m_noise2D.SetSeed(seed);
    m_noise2D.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noise2D.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noise2D.SetFractalOctaves(4);
    m_noise2D.SetFrequency(config::TERRAIN_NOISE_SCALE);

    m_noiseBedrock.SetSeed(seed + 3000);
    m_noiseBedrock.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noiseBedrock.SetFrequency(0.06f);

    // ──────── ПЕЩЕРЫ ────────
    // Добавлен Domain Warp для извилистости и мягкости
    m_noiseCaveVolume.SetSeed(seed + 1000);
    m_noiseCaveVolume.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_noiseCaveVolume.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseCaveVolume.SetFractalOctaves(3);
    m_noiseCaveVolume.SetFrequency(0.02f); 
    m_noiseCaveVolume.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
    m_noiseCaveVolume.SetDomainWarpAmp(20.0f);

    m_caveWorm1.SetSeed(seed + 2000);
    m_caveWorm1.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_caveWorm1.SetFrequency(0.025f);
    m_caveWorm1.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
    m_caveWorm1.SetDomainWarpAmp(15.0f);

    m_caveWorm2.SetSeed(seed + 4000);
    m_caveWorm2.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    m_caveWorm2.SetFrequency(0.025f);
    m_caveWorm2.SetDomainWarpType(FastNoiseLite::DomainWarpType_OpenSimplex2);
    m_caveWorm2.SetDomainWarpAmp(15.0f);

    m_noiseOre.SetSeed(seed + 5000);
    m_noiseOre.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    m_noiseOre.SetFractalType(FastNoiseLite::FractalType_FBm);
    m_noiseOre.SetFractalOctaves(3);
    m_noiseOre.SetFrequency(0.09f);
}

int WorldGenerator::heightAt(int wx, int wz) const noexcept {
    const float n = m_noise2D.GetNoise((float)wx, (float)wz);
    return config::TERRAIN_BASE_HEIGHT + (int)(n * config::TERRAIN_AMPLITUDE);
}

int WorldGenerator::bedrockTopAt(int wx, int wz) const noexcept {
    const float n = m_noiseBedrock.GetNoise((float)wx, (float)wz);
    return 1 + (int)((n * 0.5f + 0.5f) * 5.0f);
}

void WorldGenerator::generate(Chunk& chunk) const {
    const ChunkCoord c = chunk.coord();
    const int baseX = c.x * config::CHUNK_SIZE_X;
    const int baseY = c.y * config::CHUNK_SIZE_Y;
    const int baseZ = c.z * config::CHUNK_SIZE_Z;

    constexpr int TERRAIN_MAX_Y = config::TERRAIN_BASE_HEIGHT + (int)config::TERRAIN_AMPLITUDE + 1;
    if (baseY > TERRAIN_MAX_Y) return;
    if (baseY + config::CHUNK_SIZE_Y <= 0) return;

    for (int lx = 0; lx < config::CHUNK_SIZE_X; ++lx) {
        for (int lz = 0; lz < config::CHUNK_SIZE_Z; ++lz) {
            const int wx = baseX + lx;
            const int wz = baseZ + lz;
            const int h  = heightAt(wx, wz);
            const int btop = bedrockTopAt(wx, wz);

            for (int ly = 0; ly < config::CHUNK_SIZE_Y; ++ly) {
                const int wy = baseY + ly;
                if (wy < 0) continue;

                BlockID block = BLOCK_AIR;

                // Стратиграфия
                if (wy <= btop) {
                    block = BlockRegistry::BEDROCK;
                } else if (wy <= 15) {
                    block = BlockRegistry::DEEPSLATE;
                } else if (wy < h - 4) {
                    block = BlockRegistry::STONE;
                } else if (wy < h) {
                    block = BlockRegistry::DIRT;
                } else if (wy == h) {
                    block = (h > config::SEA_LEVEL) ? BlockRegistry::GRASS : BlockRegistry::SAND;
                } else if (wy > h && wy <= config::SEA_LEVEL) {
                    block = BlockRegistry::WATER;
                }

                // ───────── Пещеры ─────────
                if (block == BlockRegistry::STONE || block == BlockRegistry::DEEPSLATE) {
                    const float wx_f = (float)wx;
                    const float wy_f = (float)wy;
                    const float wz_f = (float)wz;

                    bool isSafeDepth = (wy > btop + 2);

                    if (isSafeDepth) {
                        // 1. Большие залы (Cheese)
                        float volume = m_noiseCaveVolume.GetNoise(wx_f, wy_f, wz_f);
                        volume = (volume + 1.0f) * 0.5f; 
                        
                        if (wy < h - 4 && volume > 0.68f) {
                            block = BLOCK_AIR;
                        } 
                        // 2. Тоннели (Цилиндрические и мягкие)
                        else {
                            float worm1 = m_caveWorm1.GetNoise(wx_f, wy_f, wz_f);
                            float worm2 = m_caveWorm2.GetNoise(wx_f + 1000.0f, wy_f + 2000.0f, wz_f - 500.0f);
                            
                            // Цилиндрическая формула (x^2 + y^2 < r^2) убирает квадратность
                            float r2 = 0.0025f; // Радиус тоннеля (0.05f)
                            
                            // Плавно уменьшаем радиус у поверхности, чтобы не было резких дырок
                            if (wy > h - 12) {
                                float fade = (h - wy) / 12.0f;
                                r2 *= std::max(0.0f, fade);
                            }

                            if (worm1 * worm1 + worm2 * worm2 < r2) {
                                block = BLOCK_AIR;
                            }
                        }
                    }
                }

                // ───────── Входы (Мягкие спуски на поверхности) ─────────
                if (block == BlockRegistry::GRASS || block == BlockRegistry::DIRT) {
                    float worm1 = m_caveWorm1.GetNoise((float)wx, (float)wy, (float)wz);
                    float worm2 = m_caveWorm2.GetNoise((float)wx + 1000.0f, (float)wy + 2000.0f, (float)wz - 500.0f);
                    
                    float r2 = 0.001f; // Радиус входа (0.031f)
                    if (worm1 * worm1 + worm2 * worm2 < r2) {
                        block = BLOCK_AIR;
                    }
                }

                // Руды
                if (block == BlockRegistry::STONE || block == BlockRegistry::DEEPSLATE) {
                    if (block == BlockRegistry::STONE) {
                        if (wy >= 16 && wy <= 96) {
                            const float n = m_noiseOre.GetNoise((float)wx, (float)wy, (float)wz);
                            if (n > 0.55f) block = BlockRegistry::COAL_ORE;
                        }
                        if (block == BlockRegistry::STONE && wy >= 16 && wy <= 48) {
                            const float n = m_noiseOre.GetNoise((float)wx + 913.0f, (float)wy + 527.0f, (float)wz + 271.0f);
                            if (n > 0.58f) block = BlockRegistry::IRON_ORE;
                        }
                    } else {
                        const float n = m_noiseOre.GetNoise((float)wx + 571.0f, -(float)wy, (float)wz + 439.0f);
                        if (n > 0.62f) block = BlockRegistry::GOLD_ORE;
                    }
                }

                chunk.set(lx, ly, lz, block);

                if (wy > std::max(h, config::SEA_LEVEL) + 2) break;
            }
        }
    }

    // Деревья
    constexpr int MARGIN = 2;
    for (int lx = MARGIN; lx < config::CHUNK_SIZE_X - MARGIN; ++lx) {
        for (int lz = MARGIN; lz < config::CHUNK_SIZE_Z - MARGIN; ++lz) {
            const int wx = baseX + lx;
            const int wz = baseZ + lz;
            const int h  = heightAt(wx, wz);

            if (h <= config::SEA_LEVEL || h >= config::CHUNK_SIZE_Y) continue;

            const BlockID surfaceBlock = chunk.getFast(lx, h, lz);
            if (surfaceBlock != BlockRegistry::GRASS) continue;

            if ((hash2(wx, wz, m_seed) % 100) != 0) continue;

            const uint32_t treeSeed = hash2(wx, wz, m_seed * 7 + 13);
            const int trunkHeight = 4 + (treeSeed % 3);

            if (h + trunkHeight + 2 >= config::CHUNK_SIZE_Y) continue;

            placeTree(chunk, lx, h, lz, treeSeed);
        }
    }
}

} // namespace mc