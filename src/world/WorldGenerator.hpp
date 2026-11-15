#pragma once
#include "world/Chunk.hpp"
#include "utils/FastNoiseLite.h"
#include <cstdint>

namespace mc {

class WorldGenerator {
public:
    explicit WorldGenerator(uint32_t seed);

    void generate(Chunk& chunk) const;
    int heightAt(int wx, int wz) const noexcept;
    int bedrockTopAt(int wx, int wz) const noexcept;

private:
    uint32_t m_seed;
    
    // Рельеф
    FastNoiseLite m_noise2D;
    FastNoiseLite m_noiseBedrock;
    
    // Пещеры
    FastNoiseLite m_noiseCaveVolume;
    FastNoiseLite m_caveWorm1;
    FastNoiseLite m_caveWorm2;
    
    // Руды
    FastNoiseLite m_noiseOre;
};

} // namespace mc