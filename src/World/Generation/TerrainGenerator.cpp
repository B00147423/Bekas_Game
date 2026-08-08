#include "TerrainGenerator.h"

namespace World::Generation
{
    void TerrainGenerator::GenerateTopDown(Chunk& chunk, NoiseGenerator& noise)
    {
        constexpr float treeFrequency = 0.10f;

        for (int x = 0; x < Chunk::CHUNK_WIDTH; x++)
        {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; y++)
            {
                Tile tile{};
                tile.type = TileType::Grass;
                tile.solid = false;
                // Grass variant id — World/Tileset wrap this with GetGrass()
                // so every variant can appear across the whole map
                tile.metadata = static_cast<uint8_t>(x * 13 + y * 37);

                const float trees = noise.FractalBrownianMotion(
                    static_cast<float>(x) * treeFrequency + 40.0f,
                    static_cast<float>(y) * treeFrequency + 40.0f,
                    3,
                    0.5f
                );

                if (trees > 0.32f && ((x * 3 + y * 5) % 4) != 0)
                {
                    tile.type = TileType::Tree;
                    tile.solid = true;
                }

                chunk.SetTile(x, y, tile);
            }
        }
    }

}
