#include "ui/MemoryHud.hpp"
#include <cstdio>
#include <cstring>
#include <cstddef>

namespace mc {

namespace {

constexpr float REFRESH_INTERVAL = 1.0f;

// Resident set size of this process in bytes; -1 on failure (Linux /proc).
long long processRssBytes() noexcept {
#ifdef __linux__
    FILE* f = std::fopen("/proc/self/status", "r");
    if (!f) return -1;
    long long kb = -1;
    char line[256];
    while (std::fgets(line, sizeof(line), f)) {
        if (std::strncmp(line, "VmRSS:", 6) == 0) {
            std::sscanf(line + 6, "%lld", &kb);
            break;
        }
    }
    std::fclose(f);
    return kb > 0 ? kb * 1024 : -1;
#else
    return -1;
#endif
}

// Total system RAM in bytes; -1 on failure (Linux /proc).
long long systemTotalBytes() noexcept {
#ifdef __linux__
    FILE* f = std::fopen("/proc/meminfo", "r");
    if (!f) return -1;
    long long kb = -1;
    char line[256];
    while (std::fgets(line, sizeof(line), f)) {
        if (std::strncmp(line, "MemTotal:", 9) == 0) {
            std::sscanf(line + 9, "%lld", &kb);
            break;
        }
    }
    std::fclose(f);
    return kb > 0 ? kb * 1024 : -1;
#else
    return -1;
#endif
}

void formatBytes(long long bytes, char* out, std::size_t outSize) noexcept {
    if (bytes < 0) {
        std::snprintf(out, outSize, "N/A");
        return;
    }
    constexpr long long GB = 1024LL * 1024LL * 1024LL;
    constexpr long long MB = 1024LL * 1024LL;
    if (bytes >= GB) {
        std::snprintf(out, outSize, "%.1f GB", (double)bytes / (double)GB);
    } else {
        std::snprintf(out, outSize, "%lld MB", bytes / MB);
    }
}

} // namespace

void MemoryHud::update(float dt) {
    m_accum += dt;
    // First call refreshes immediately; afterwards at most once per second.
    if (m_accum < REFRESH_INTERVAL && m_valid) return;
    m_accum = 0.0f;

    const long long proc  = processRssBytes();
    const long long total = systemTotalBytes();
    if (proc < 0 || total <= 0) {
        m_valid = false;
        return;
    }

    char procBuf[32]  = {};
    char totalBuf[32] = {};
    formatBytes(proc,  procBuf,  sizeof(procBuf));
    formatBytes(total, totalBuf, sizeof(totalBuf));
    std::snprintf(m_text, sizeof(m_text), "Mem: %s / %s", procBuf, totalBuf);
    m_valid = true;
}

void MemoryHud::draw() const {
    DrawText(m_valid ? m_text : "Mem: N/A", 10, 10, 20, DARKGRAY);
}

} // namespace mc
