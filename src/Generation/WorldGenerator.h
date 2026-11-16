
#pragma once
#include "NoiseGenerator.h"
#include "World/Chunk.h"
#include <cstdint>

namespace World::Generation
{
    class WorldGenerator
    {
    public:
        WorldGenerator(uint64_t seed);

        void GenerateChunk(Chunk& chunk);
        bool IsForestAt(float tileX, float tileY);
        bool HasNearbyTree(const Chunk& chunk, int x, int y, int radius) const;

    private:
        Tile GenerateTile(int x, int y);
        void GenerateTrees(Chunk& chunk, int treeCount);

        uint64_t m_seed;
        NoiseGenerator m_noise;
    };
}
