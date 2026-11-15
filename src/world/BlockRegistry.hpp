#pragma once
#include "world/Block.hpp"
#include <array>

namespace mc {

class BlockRegistry {
public:
    static void init();
    static const BlockType& get(BlockID id) noexcept;
    static BlockID idByName(std::string_view name) noexcept;

    static constexpr BlockID AIR       = 0;
    static constexpr BlockID STONE     = 1;
    static constexpr BlockID DIRT      = 2;
    static constexpr BlockID GRASS     = 3;
    static constexpr BlockID GLASS     = 4;
    static constexpr BlockID DEEPSLATE = 5;
    static constexpr BlockID BEDROCK   = 6;
    static constexpr BlockID WATER     = 7;
    static constexpr BlockID SAND      = 8;
    static constexpr BlockID LOG       = 9;
    static constexpr BlockID LEAVES    = 10;
    static constexpr BlockID COAL_ORE  = 11;
    static constexpr BlockID IRON_ORE  = 12;
    static constexpr BlockID GOLD_ORE  = 13;

private:
    static std::array<BlockType, 256> s_blocks;
    static bool s_initialized;
};

} // namespace mc