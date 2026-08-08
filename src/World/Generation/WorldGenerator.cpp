#include "WorldGenerator.h"

namespace World::Generation
{
    WorldGenerator::WorldGenerator(uint64_t seed)
        : 
          m_noise(seed)
          //m_heightMapGenerator(seed),
    {
    }

    void WorldGenerator::GenerateChunk(Chunk& chunk)
    {
        // Top-down flat map. HeightMapGenerator stays available if we need hills later.
        m_terrainGenerator.GenerateTopDown(chunk, m_noise);
    }
}
