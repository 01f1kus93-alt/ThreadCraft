#pragma once
#include "core/Types.hpp"
#include <array>
#include <string_view>

namespace mc {

struct BlockType {
    BlockID          id;
    std::string_view name;
    bool             isOpaque;      // блокирует ли грани соседей (для Face Culling)
    bool             isSolid;       // имеет ли коллизию (для будущей физики)
    bool             isTransparent; // требует ли отрисовки в отдельном проходе
    std::array<int, 6> faceTiles;
    float            hardness = 0.0f; // seconds to break (vanilla); < 0 = unbreakable

    bool isAir() const noexcept { return id == BLOCK_AIR; }
};

} // namespace mc