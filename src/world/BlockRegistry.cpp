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

    s_blocks[AIR] = BlockType{ AIR, "air", false, false, false, allFaces(-1), 0.0f };
    s_blocks[STONE] = BlockType{ STONE, "stone", true, true, false, allFaces(0), 1.5f };
    s_blocks[DIRT] = BlockType{ DIRT, "dirt", true, true, false, allFaces(1), 0.5f };
    s_blocks[GRASS] = BlockType{ GRASS, "grass_block", true, true, false, 
        std::array<int, 6>{3, 3, 1, 2, 3, 3}, 0.6f };

    // Новые блоки: глубокий сланец и коренная порода
    s_blocks[DEEPSLATE] = BlockType{ DEEPSLATE, "deepslate", true, true, false, allFaces(5), 3.0f };
    s_blocks[BEDROCK]   = BlockType{ BEDROCK,   "bedrock",   true, true, false, allFaces(6), -1.0f };

    // Тестовые блоки для прозрачности
    s_blocks[GLASS] = BlockType{ GLASS, "glass", false, true, true, allFaces(4), 0.3f };
    s_blocks[WATER] = BlockType{ WATER, "water", false, false, true, allFaces(7), -1.0f };

    // Новые блоки: песок, бревно, листва
    s_blocks[SAND]   = BlockType{ SAND,   "sand",   true,  true, false, allFaces(8), 0.5f };
    s_blocks[LOG]    = BlockType{ LOG,    "oak_log", true,  true, false, allFaces(9), 2.0f };
    s_blocks[LEAVES] = BlockType{ LEAVES, "oak_leaves", false, true, true, allFaces(10), 0.2f };

    // Рудные жилы («шахты»)
    s_blocks[COAL_ORE] = BlockType{ COAL_ORE, "coal_ore", true, true, false, allFaces(11), 3.0f };
    s_blocks[IRON_ORE] = BlockType{ IRON_ORE, "iron_ore", true, true, false, allFaces(12), 3.0f };
    s_blocks[GOLD_ORE] = BlockType{ GOLD_ORE, "gold_ore", true, true, false, allFaces(13), 3.0f };
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