#pragma once

#include "World/Chunk.h"
#include "NoiseGenerator.h"
#include <cmath>

namespace World::Generation
{
    inline uint8_t GroveTreeType(NoiseGenerator& noise, int x, int y, std::mt19937& rng)
    {
        // Broadleaf pockets are less common; neighbouring pines share a palette.
        const float grove = noise.SimplexNoise2D(x * 0.065f + 71.0f, y * 0.065f + 43.0f);
        if (grove > 0.35f)
            return static_cast<uint8_t>(8 + rng() % 3);
        const int family = grove < -0.2f ? 0 : 4;
        return static_cast<uint8_t>(family + rng() % 4);
    }

    inline bool HasCanopySpace(const Chunk& chunk, int x, int y, uint8_t type)
    {
        const Rectangle source = GameConfig::Trees::SpriteSourcesTree[type];
        const float scale = GameConfig::Trees::RenderScale;
        const int radius = static_cast<int>(std::ceil(96.0f * scale / GameConfig::World::TileSize));
        for (int ny = std::max(0, y - radius); ny <= std::min(Chunk::CHUNK_HEIGHT - 1, y + radius); ++ny)
        {
            for (int nx = std::max(0, x - radius); nx <= std::min(Chunk::CHUNK_WIDTH - 1, x + radius); ++nx)
            {
                const Tile other = chunk.GetTile(nx, ny);
                if (other.type != TileType::Tree) continue;
                const Rectangle otherSource = GameConfig::Trees::SpriteSourcesTree[other.metadata % 11];
                // Allow some crown overlap, with closer spacing front-to-back.
                const float spacingX = (source.width + otherSource.width) * scale * 0.4f;
                const float spacingY = (source.height + otherSource.height) * scale * 0.22f;
                const float dx = (x - nx) * GameConfig::World::TileSize / spacingX;
                const float dy = (y - ny) * GameConfig::World::TileSize / spacingY;
                if (dx * dx + dy * dy < 1.0f) return false;
            }
        }
        return true;
    }
}
