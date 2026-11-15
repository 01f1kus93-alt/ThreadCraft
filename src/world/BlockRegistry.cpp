#include "world/BlockRegistry.hpp"
#include <cassert>

namespace mc {

std::array<BlockType, 256> BlockRegistry::s_blocks{};
bool BlockRegistry::s_initialized = false;

namespace {
    std::array<int, 6> allFaces(int tile) noexcept {
        return { tile, tile, tile, tile, tile, tile };
    }
}

void BlockRegistry::init() {
    if (s_initialized) return;
    s_initialized = true;

    s_blocks[AIR] = BlockType{ AIR, "air", false, false, false, allFaces(-1) };
    s_blocks[STONE] = BlockType{ STONE, "stone", true, true, false, allFaces(0) };
    s_blocks[DIRT] = BlockType{ DIRT, "dirt", true, true, false, allFaces(1) };
    s_blocks[GRASS] = BlockType{ GRASS, "grass_block", true, true, false, 
        std::array<int, 6>{3, 3, 1, 2, 3, 3} };

    // Новые блоки: глубокий сланец и коренная порода
    s_blocks[DEEPSLATE] = BlockType{ DEEPSLATE, "deepslate", true, true, false, allFaces(5) };
    s_blocks[BEDROCK]   = BlockType{ BEDROCK,   "bedrock",   true, true, false, allFaces(6) };

    // Тестовые блоки для прозрачности
    s_blocks[GLASS] = BlockType{ GLASS, "glass", false, true, true, allFaces(4) };
    s_blocks[WATER] = BlockType{ WATER, "water", false, false, true, allFaces(7) };

    // Новые блоки: песок, бревно, листва
    s_blocks[SAND]   = BlockType{ SAND,   "sand",   true,  true, false, allFaces(8) };
    s_blocks[LOG]    = BlockType{ LOG,    "oak_log", true,  true, false, allFaces(9) };
    s_blocks[LEAVES] = BlockType{ LEAVES, "oak_leaves", false, true, true, allFaces(10) };

    // Рудные жилы («шахты»)
    s_blocks[COAL_ORE] = BlockType{ COAL_ORE, "coal_ore", true, true, false, allFaces(11) };
    s_blocks[IRON_ORE] = BlockType{ IRON_ORE, "iron_ore", true, true, false, allFaces(12) };
    s_blocks[GOLD_ORE] = BlockType{ GOLD_ORE, "gold_ore", true, true, false, allFaces(13) };
}

const BlockType& BlockRegistry::get(BlockID id) noexcept {
    assert(s_initialized && "BlockRegistry::init() must be called before get()");
    return s_blocks[id];
}

BlockID BlockRegistry::idByName(std::string_view name) noexcept {
    for (uint32_t i = 0; i < s_blocks.size(); ++i) {
        if (s_blocks[i].name == name) return BlockID(i);
    }
    return BLOCK_AIR;
}

} // namespace mc