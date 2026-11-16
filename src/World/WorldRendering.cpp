#include "World.h"
#include "VisibleTiles.h"

namespace World
{

    std::vector<Vector2> World::GetWaterTiles() const{
        return m_chunk.GetWaterTiles();
    }

    void World::DrawGrassTile(const Tileset& tileset, int tileX, int tileY, uint8_t metadata, const Rectangle& dest, Color tint) const{
        unsigned hash = static_cast<unsigned>(tileX);
        hash ^= static_cast<unsigned>(tileY) + 0x9e3779b9u + (hash << 6) + (hash >> 2);

        hash ^= hash >> 16;
        hash *= 0x7feb352du;
        hash ^= hash >> 15;
        hash *= 0x846ca68bu;
        hash ^= hash >> 16;
        const Rectangle src = tileset.GetGrass(static_cast<int>(hash & 0x7fffffffu));

        DrawTexturePro(
            tileset.GetGrassTexture(),
            src,
            dest,
            { 0.0f, 0.0f },
            0.0f,
            tint
        );

        (void)metadata;
    }

    bool World::isForest(int tileX, int tileY) const{
        if (tileX < 0 || tileX >= Chunk::CHUNK_WIDTH ||
            tileY < 0 || tileY >= Chunk::CHUNK_HEIGHT)
        {
            return false;
        }
        const Tile tile = m_chunk.GetTile(tileX, tileY);
        return tile.type != TileType::Water && tile.biome == BiomeType::Forest;
    }

    uint8_t World::GetForestMask(int tileX, int tileY) const{
        const bool north = isForest(tileX, tileY-1);
        const bool east  = isForest(tileX+1, tileY);
        const bool south = isForest(tileX, tileY+1);
        const bool west = isForest(tileX-1, tileY);

        uint8_t mask = 0;

        if (north) mask |= 1;
        if (east)  mask |= 4;
        if (south) mask |= 16;
        if (west)  mask |= 64;

        if (north && east &&isForest(tileX + 1, tileY - 1))
        {
            mask |= 2;
        }

        if (east && south &&isForest(tileX + 1, tileY + 1))
        {
            mask |= 8;
        }

        if (south && west &&isForest(tileX - 1, tileY + 1))
        {
            mask |= 32;
        }

        if (west && north &&isForest(tileX - 1, tileY - 1))
        {
        mask |= 128;
        }
        return mask;
    }



    void World::DrawGround(const Tileset& tileset, Rectangle visibleBounds) const {
        constexpr float tileSize = TileSize;
        constexpr int width = Chunk::CHUNK_WIDTH;
        constexpr int height = Chunk::CHUNK_HEIGHT;

        // Include partial edge tiles. Neighbour masks still read the full chunk.
        const int left = std::clamp(int(std::floor(visibleBounds.x / tileSize)), 0, width);
        const int top = std::clamp(int(std::floor(visibleBounds.y / tileSize)), 0, height);
        const int right = std::clamp(int(std::ceil((visibleBounds.x + visibleBounds.width) / tileSize)), 0, width);
        const int bottom = std::clamp(int(std::ceil((visibleBounds.y + visibleBounds.height) / tileSize)), 0, height);
        for (int x = left; x < right; ++x) {
            for (int y = top; y < bottom; ++y) {
                const Tile tile = m_chunk.GetTile(x, y);

                if (tile.type == TileType::Air) continue;

                const Rectangle dest = {
                    static_cast<float>(x) * tileSize,
                    static_cast<float>(y) * tileSize,
                    tileSize,
                    tileSize
                };

                if (tile.type == TileType::Water) {
                    DrawGrassTile(tileset, x, y, tile.metadata, dest, WHITE);
                    continue;
                }

                const bool canDrawForest =
                    tile.biome == BiomeType::Forest &&
                    tileset.GetForestTexture().id != 0;

                if (canDrawForest) {
                    const uint8_t mask = GetForestMask(x, y);

                    DrawTexturePro(
                        tileset.GetForestTexture(),
                        tileset.GetForest(mask),
                        dest,
                        {0.0f, 0.0f},
                        0.0f,
                        WHITE
                    );
                } else {
                    DrawGrassTile(tileset, x, y, tile.metadata, dest, WHITE);
                }
            }
        }
    }




    void World::DrawTrees( const Tileset& tileset, float playerFeetY, bool behindPlayer, Color tint, Rectangle visibleBounds) const
    {
        constexpr float tileSize = TileSize;

        const auto bounds = VisibleTiles(visibleBounds, TreeExtent());
        for (int y = bounds.top; y < bounds.bottom; ++y)
        {
            for (int x = bounds.left; x < bounds.right; ++x)
            {
                const Tile tile = m_chunk.GetTile(x, y);

                if (tile.type != TileType::Tree)
                    continue;

                const Rectangle treeSource = tileset.GetTree(tile.metadata);

                const float treeFeetY =
                    static_cast<float>(y) * tileSize +
                    tileSize +
                    GameConfig::Trees::AnchorOffset.y;

                const bool isBehind = treeFeetY < playerFeetY;

                if (isBehind != behindPlayer)
                    continue;

                // Use the same uniform scale for drawing and shadow placement.
                const float treeWidth = treeSource.width * GameConfig::Trees::RenderScale;
                const float treeHeight = treeSource.height * GameConfig::Trees::RenderScale;

                const Rectangle dest = {
                    static_cast<float>(x) * tileSize +
                        (tileSize - treeWidth) * 0.5f +
                        GameConfig::Trees::AnchorOffset.x,
                    treeFeetY - treeHeight,
                    treeWidth,
                    treeHeight
                };

                DrawTexturePro(
                    tileset.GetTreeAtlasTexture(),
                    treeSource,
                    dest,
                    {0.0f, 0.0f},
                    0.0f,
                    tint
                );
            }
        }
    }
    void World::DrawObjects(  float playerFeetY, bool behindPlayer, Vector2 mouseWorld, Shader hoverShader
    ) const
    {
        for (const WorldObject& object : m_objects)
        {
            const float objectFeetY =
                object.position.y + object.texture.height;
            if ((objectFeetY < playerFeetY) != behindPlayer)
                continue;

            const Rectangle bounds{
                object.position.x,
                object.position.y,
                static_cast<float>(object.texture.width),
                static_cast<float>(object.texture.height)
            };

            const bool hovered =
                CheckCollisionPointRec(mouseWorld, bounds) &&
                IsShaderValid(hoverShader);

            if (hovered)
                BeginShaderMode(hoverShader);

            DrawTextureV(object.texture, object.position, WHITE);

            if (hovered)
                EndShaderMode();
        }
    }
}
