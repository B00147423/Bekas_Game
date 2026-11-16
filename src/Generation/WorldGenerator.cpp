
#include "WorldGenerator.h"
#include "ForestPlacement.h"
#include "GameConfig.h"
#include <algorithm>
#include <random>
#include <vector>

namespace World::Generation
{
    WorldGenerator::WorldGenerator(uint64_t seed)
        : m_noise(seed), m_seed(seed)
    {
    }

    void WorldGenerator::GenerateChunk(Chunk& chunk)
    {
        if constexpr (Chunk::CHUNK_WIDTH * Chunk::CHUNK_HEIGHT > 65536) {
            chunk.SetGenerator([this](int x,int y) { return GenerateTile(x,y); });
            return;
        }
        chunk.SetGenerator({});
        constexpr float biomeFrequency = 0.06f;

        for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x) {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y) {
                const float biome = m_noise.FractalBrownianMotion(
                    x * biomeFrequency, y * biomeFrequency, 3, 0.5f
                );

                Tile tile{};
                tile.type = TileType::Grass;
                tile.solid = false;
                tile.biome = biome > 0.0f ? BiomeType::Forest : BiomeType::Plains;
                tile.metadata = static_cast<uint8_t>(x * 13 + y * 37);
                chunk.SetTile(x, y, tile);
            }
        }

        GenerateTrees(chunk, GameConfig::World::StartingTreeCount);
    }


Tile WorldGenerator::GenerateTile(int x,int y) {
    Tile tile{}; tile.type=TileType::Grass;
    const float forest=m_noise.FractalBrownianMotion(x*0.06f,y*0.06f,3,0.5f);
    tile.biome=forest>0 ? BiomeType::Forest : BiomeType::Plains;
    tile.metadata=static_cast<uint8_t>(x*13+y*37);
    const int sx=int((GameConfig::Player::SpawnPosition.x+GameConfig::Player::BodyOffset.x)/GameConfig::World::TileSize);
    const int sy=int((GameConfig::Player::SpawnPosition.y+GameConfig::Player::BodyOffset.y)/GameConfig::World::TileSize);
    if((x-sx)*(x-sx)+(y-sy)*(y-sy)<=GameConfig::World::SpawnClearRadius*GameConfig::World::SpawnClearRadius) return tile;
    // Position-based randomness makes page load order irrelevant.
    uint64_t value=m_seed ^ (uint64_t(x)*0x9e3779b97f4a7c15ULL) ^ (uint64_t(y)*0xbf58476d1ce4e5b9ULL);
    value=(value^(value>>30))*0xbf58476d1ce4e5b9ULL;
    value=(value^(value>>27))*0x94d049bb133111ebULL; value^=value>>31;
    const float density=m_noise.FractalBrownianMotion(x*0.10f+100,y*0.10f+100,3,0.5f);
    const float probability=forest>0 ? std::clamp(0.30f+density*0.25f,0.05f,0.65f) : 0.02f;
    if(float(value>>40)/16777216.0f<probability) {
        tile.type=TileType::Tree;tile.solid=true;tile.metadata=static_cast<uint8_t>(value%11);
    }
    return tile;
}

void WorldGenerator::GenerateTrees(Chunk& chunk, int treeCount)
{
    const float tileSize = GameConfig::World::TileSize;

    const Vector2 spawn = {
        GameConfig::Player::SpawnPosition.x + GameConfig::Player::BodyOffset.x,
        GameConfig::Player::SpawnPosition.y + GameConfig::Player::BodyOffset.y
    };

    const int spawnX = static_cast<int>(spawn.x / tileSize);
    const int spawnY = static_cast<int>(spawn.y / tileSize);
    const int clearRadius = GameConfig::World::SpawnClearRadius;

    std::seed_seq seedSequence{
        static_cast<uint32_t>(m_seed),
        static_cast<uint32_t>(m_seed >> 32)
    };

    std::mt19937 rng(seedSequence);
    std::uniform_real_distribution<float> chance(0.0f, 1.0f);
    std::uniform_int_distribution<int> treeVariant(0, 10);

    constexpr float forestFrequency = 0.06f;
    constexpr float densityFrequency = 0.10f;

    for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y)
    {
        for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x)
        {
            const int dx = x - spawnX;
            const int dy = y - spawnY;

            if (dx * dx + dy * dy <= clearRadius * clearRadius)
                continue;

            Tile tile = chunk.GetTile(x, y);

            if (tile.type != TileType::Grass || tile.solid)
                continue;

            const float forestNoise = m_noise.FractalBrownianMotion(
                x * forestFrequency,
                y * forestFrequency,
                3,
                0.5f
            );

            const float densityNoise = m_noise.FractalBrownianMotion(
                x * densityFrequency + 100.0f,
                y * densityFrequency + 100.0f,
                3,
                0.5f
            );

            float probability = 0.02f;

            if (forestNoise > 0.0f)
            {
                probability = std::clamp(
                    0.30f + densityNoise * 0.25f,
                    0.05f,
                    0.65f
                );
            }

            if (chance(rng) >= probability)
                continue;

            tile.type = TileType::Tree;
            tile.solid = true;
            tile.metadata = static_cast<uint8_t>(treeVariant(rng));

            chunk.SetTile(x, y, tile);
        }
    }
}

bool WorldGenerator::HasNearbyTree(const Chunk& chunk, int x, int y, int radius) const{
        for (int ny = y - radius; ny <= y + radius; ++ny) {
            for (int nx = x - radius; nx <= x + radius; ++nx) {
                if (nx < 0 || nx >= Chunk::CHUNK_WIDTH ||
                    ny < 0 || ny >= Chunk::CHUNK_HEIGHT)
                {
                    continue;
                }

                if (nx == x && ny == y)
                    continue;

                if (chunk.GetTile(nx, ny).type == TileType::Tree)
                    return true;
            }
        }

        return false;
    }

    bool WorldGenerator::IsForestAt(float tileX, float tileY)
    {
        constexpr float biomeFrequency = 0.06f;

        const float biome = m_noise.FractalBrownianMotion(
            tileX * biomeFrequency, tileY * biomeFrequency, 3, 0.5f
        );

        return biome > 0.0f;
    }
}
