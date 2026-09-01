#include "TerrainGenerator.h"

namespace World::Generation
{

    void TerrainGenerator::GenerateTopDown(Chunk& chunk, NoiseGenerator& noise)
    {
        constexpr float treeFrequency = 0.10f;
        constexpr float biomeFrequency = 0.02f;
        constexpr float treeThreshold = 0.32f;

        for (int x = 0; x < Chunk::CHUNK_WIDTH; x++)
        {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; y++)
            {
                const float biome = noise.FractalBrownianMotion(
                    static_cast<float>(x) * biomeFrequency,
                    static_cast<float>(y) * biomeFrequency,
                    3,
                    0.5f
                );

                bool isForest = biome > 0.0f;

                Tile tile{};
                tile.type = TileType::Grass;
                tile.solid = false;
                tile.biome = isForest ? BiomeType::Forest : BiomeType::Plains;
                tile.metadata = static_cast<uint8_t>(x * 13 + y * 37);

                const float trees = noise.FractalBrownianMotion(
                    static_cast<float>(x) * treeFrequency + 40.0f,
                    static_cast<float>(y) * treeFrequency + 40.0f,
                    3,
                    0.5f
                );

                if (tile.biome == BiomeType::Forest &&
                    trees > treeThreshold &&
                    ((x * 3 + y * 5) % 4) != 0)
                {
                    tile.type = TileType::Tree;
                    tile.solid = true;
                }

                chunk.SetTile(x, y, tile);
            }
        }
    } // GenerateTopDown ENDS HERE


    bool TerrainGenerator::HasNearbyTree( const Chunk& chunk, int x, int y, int radius) const
    {
        for(int offsetX =- radius; offsetX <= radius; offsetX ++){
            for (int offsetY =- radius; offsetY <= radius; offsetY++){


                int checkX = x + offsetX;
                int checkY  =x + offsetY;

                if(offsetX == 0 && offsetY == 0)
                    continue;
                // Don't go outside the chunk
                if (checkX < 0 || checkX >= Chunk::CHUNK_WIDTH ||
                    checkY < 0 || checkY >= Chunk::CHUNK_HEIGHT)
                    continue;

                if (chunk.GetTile(checkX, checkY).type == TileType::Tree)
                {
                    return true;
                }
            }
        }

        return false;
    }

}
