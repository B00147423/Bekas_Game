#pragma once

#include <vector>
#include "../Chunk.h"
#include "NoiseGenerator.h"

namespace World::Generation
{
    class TerrainGenerator
    {
    public:
        TerrainGenerator() = default;

        // Top-down survival builder: flat ground with grass / dirt / stone / water patches
        void GenerateTopDown(Chunk& chunk, NoiseGenerator& noise);

        // // Side-view / height-map path — kept for later, not used by WorldGenerator yet
        // void GenerateFromHeightMap(Chunk& chunk, const std::vector<int>& heightMap);

    private:
        // void PlaceSurface(Chunk& chunk, int x, int surfaceY);
        // void FillGround(Chunk& chunk, int x, int surfaceY);
    };
}
