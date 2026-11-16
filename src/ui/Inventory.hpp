#pragma once
#include "ui/ItemStack.hpp"
#include <algorithm>

namespace mc {

// 36 slots: hotbar 0..8, main 9..35.
class Inventory {
public:
    static constexpr int HOTBAR = 9;
    static constexpr int MAIN   = 27;
    static constexpr int TOTAL  = HOTBAR + MAIN;

    ItemStack&       hotbar(int i)       noexcept { return m_slots[std::clamp(i, 0, HOTBAR - 1)]; }
    const ItemStack& hotbar(int i) const noexcept { return m_slots[std::clamp(i, 0, HOTBAR - 1)]; }
    ItemStack&       main(int i)         noexcept { return m_slots[HOTBAR + std::clamp(i, 0, MAIN - 1)]; }
    const ItemStack& main(int i)   const noexcept { return m_slots[HOTBAR + std::clamp(i, 0, MAIN - 1)]; }

    int  selected() const noexcept { return m_selected; }
    void select(int i) noexcept    { m_selected = std::clamp(i, 0, HOTBAR - 1); }

    // Positive delta (raylib wheel-up) selects the previous slot, like MC.
    void scrollSelect(int delta) noexcept {
        if (delta == 0) return;
        m_selected = ((m_selected - delta) % HOTBAR + HOTBAR) % HOTBAR;
    }

    bool add(BlockID id, int count = 1);
    int  remove(BlockID id, int count = 1);
    ItemStack take(int slot, int amount = 1);   // slot: absolute index 0..TOTAL-1
    int  mergeInto(ItemStack stack);            // returns leftover count (0 = fully merged)
    int  countOf(BlockID id) const noexcept;
    bool has(BlockID id, int count = 1) const noexcept;

private:
    ItemStack m_slots[TOTAL]{};
    int       m_selected = 0;
};

} // namespace mc
