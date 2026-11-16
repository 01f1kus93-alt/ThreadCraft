#include "ui/Inventory.hpp"
#include "core/Config.hpp"
#include <algorithm>

namespace mc {

bool Inventory::add(BlockID id, int count) {
    if (id == BLOCK_AIR || count <= 0) return false;

    // 1) Top up existing stacks of the same id: hotbar first, then main.
    for (int pass = 0; pass < 2 && count > 0; ++pass) {
        const int start = (pass == 0) ? 0 : HOTBAR;
        const int end   = (pass == 0) ? HOTBAR : TOTAL;
        for (int i = start; i < end && count > 0; ++i) {
            ItemStack& s = m_slots[i];
            if (s.id != id || s.count >= config::STACK_MAX) continue;
            const int room = config::STACK_MAX - (int)s.count;
            const int put  = std::min(count, room);
            s.count = (uint8_t)(s.count + put);
            count  -= put;
        }
    }

    // 2) New stacks in empty slots: hotbar first, then main.
    for (int pass = 0; pass < 2 && count > 0; ++pass) {
        const int start = (pass == 0) ? 0 : HOTBAR;
        const int end   = (pass == 0) ? HOTBAR : TOTAL;
        for (int i = start; i < end && count > 0; ++i) {
            ItemStack& s = m_slots[i];
            if (!s.isEmpty()) continue;
            const int put = std::min(count, config::STACK_MAX);
            s.id    = id;
            s.count = (uint8_t)put;
            count  -= put;
        }
    }

    return count == 0;
}

int Inventory::remove(BlockID id, int count) {
    if (id == BLOCK_AIR || count <= 0) return 0;
    int removed = 0;
    for (int i = 0; i < TOTAL && removed < count; ++i) {
        ItemStack& s = m_slots[i];
        if (s.id != id) continue;
        const int takeN = std::min((int)s.count, count - removed);
        s.count = (uint8_t)(s.count - takeN);
        if (s.count == 0) s.clear();
        removed += takeN;
    }
    return removed;
}

ItemStack Inventory::take(int slot, int amount) {
    if (slot < 0 || slot >= TOTAL || amount <= 0) return {};
    ItemStack& s = m_slots[slot];
    if (s.isEmpty()) return {};
    const int takeN = std::min((int)s.count, amount);
    ItemStack out{ s.id, (uint8_t)takeN };
    s.count = (uint8_t)(s.count - takeN);
    if (s.count == 0) s.clear();
    return out;
}

int Inventory::mergeInto(ItemStack stack) {
    if (stack.isEmpty()) return 0;
    int count = (int)stack.count;
    const BlockID id = stack.id;

    // Top up existing stacks, then fill empty slots.
    for (int i = 0; i < TOTAL && count > 0; ++i) {
        ItemStack& s = m_slots[i];
        if (s.id != id || s.count >= config::STACK_MAX) continue;
        const int room = config::STACK_MAX - (int)s.count;
        const int put  = std::min(count, room);
        s.count = (uint8_t)(s.count + put);
        count  -= put;
    }
    for (int i = 0; i < TOTAL && count > 0; ++i) {
        ItemStack& s = m_slots[i];
        if (!s.isEmpty()) continue;
        const int put = std::min(count, config::STACK_MAX);
        s.id    = id;
        s.count = (uint8_t)put;
        count  -= put;
    }
    return count;
}

int Inventory::countOf(BlockID id) const noexcept {
    if (id == BLOCK_AIR) return 0;
    int total = 0;
    for (const ItemStack& s : m_slots) {
        if (s.id == id) total += (int)s.count;
    }
    return total;
}

bool Inventory::has(BlockID id, int count) const noexcept {
    return countOf(id) >= count;
}

} // namespace mc
