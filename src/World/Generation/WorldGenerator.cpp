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

    bool WorldGenerator::IsForestAt(float tileX, float tileY)
    {
        constexpr float biomeFrequency = 0.02f;

        const float biome = m_noise.FractalBrownianMotion(
            tileX * biomeFrequency,
            tileY * biomeFrequency,
            3,
            0.5f
        );

        return biome > 0.0f;
    }
}
