#pragma once
#include "core/Types.hpp"

namespace mc {

struct ItemStack {
    BlockID id    = BLOCK_AIR;
    uint8_t count = 0;

    bool isEmpty() const noexcept { return id == BLOCK_AIR || count == 0; }
    void clear()          noexcept { id = BLOCK_AIR; count = 0; }
};

} // namespace mc
