#include "world/Chunk.hpp"

namespace mc {

Chunk::Chunk(ChunkCoord coord) : m_coord(coord) {
    // Секции (m_sections) выделяются лениво в set(); пустые секции не занимают память.
    columnTop.fill(-1);

    // Явное приведение к float для устранения warning: narrowing conversion
    boxMin = {
        static_cast<float>(coord.x * config::CHUNK_SIZE_X),
        static_cast<float>(coord.y * config::CHUNK_SIZE_Y),
        static_cast<float>(coord.z * config::CHUNK_SIZE_Z)
    };
    boxMax = {
        boxMin.x + static_cast<float>(config::CHUNK_SIZE_X),
        boxMin.y + static_cast<float>(config::CHUNK_SIZE_Y),
        boxMin.z + static_cast<float>(config::CHUNK_SIZE_Z)
    };
}

} // namespace mc