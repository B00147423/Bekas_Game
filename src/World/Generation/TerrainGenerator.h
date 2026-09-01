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


    private:

        bool HasNearbyTree(const Chunk& chunk, int x, int y, int radius) const;
    };
}
