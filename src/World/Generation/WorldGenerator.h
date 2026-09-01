#pragma once

#include "HeightMapGenerator.h"
#include "TerrainGenerator.h"
#include "NoiseGenerator.h"
#include "../Chunk.h"

namespace World::Generation
{
    class WorldGenerator
    {
    public:
        WorldGenerator(uint64_t seed);

        void GenerateChunk(Chunk& chunk);
        bool IsForestAt(float tileX, float tileY);

    private:
        // Kept for later (hills / side-view). Not used by top-down GenerateChunk yet.
        // HeightMapGenerator m_heightMapGenerator;
        TerrainGenerator m_terrainGenerator;
        NoiseGenerator m_noise;
    };
}
