#include "World.h"
#include "VisibleTiles.h"

#include <algorithm>
#include <cmath>

namespace World
{
    void World::DrawOccluders() const{
        for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y){
            for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x){
                const Tile& tile = m_chunk.GetTile(x, y);

                if (tile.type != TileType::Tree) continue;

                DrawRectangle(
                    x * TileSize,
                    y * TileSize,
                    TileSize,
                    TileSize,
                    WHITE
                );
            }
        }
    }

    void World::DrawAutoShadows( const Tileset& tileset, Lighting::ShadowRenderer2D& renderer, Vector2 lightPosition, float shadowLength, Rectangle visibleBounds, float lightRadius, Color maskColor) const{
        // Conservative sweep bounds retain off-screen casters whose shadows enter the view.
        const auto visible = [&](Rectangle sprite, const Lighting::ShadowStyle& style) {
            if (style.mode == Lighting::ShadowMode::None) return false;
            const bool blocker = style.mode == Lighting::ShadowMode::Occlusion;
            Rectangle bounds = blocker ? sprite : Lighting::ShadowBase(sprite, style);
            float reach = blocker ? shadowLength : style.mode == Lighting::ShadowMode::Footprint ? std::max(0.0f, style.maxProjection) : 0.0f;
            Rectangle sweep{bounds.x - reach, bounds.y - reach,
                bounds.width + 2 * reach, bounds.height + 2 * reach};
            return CheckCollisionRecs(sweep, visibleBounds) &&
                CheckCollisionCircleRec(lightPosition, lightRadius, sweep);
        };
        // 1. Automatically cast shadows for trees.
        const Texture2D& treeTexture = tileset.GetTreeAtlasTexture();

        if (treeTexture.id != 0)
        {
            const float reach = std::max(shadowLength, GameConfig::Trees::Shadow.maxProjection);
            const auto bounds=VisibleTiles({lightPosition.x-lightRadius,lightPosition.y-lightRadius,
                lightRadius*2,lightRadius*2},TreeExtent()+reach);
            const int minX=bounds.left, maxX=bounds.right, minY=bounds.top, maxY=bounds.bottom;
            for (int y = minY; y < maxY; ++y)
            {
                for (int x = minX; x < maxX; ++x)
                {
                    const Tile tile = m_chunk.GetTile(x, y);

                    if (tile.type != TileType::Tree)
                        continue;

                    const Rectangle treeSource = tileset.GetTree(tile.metadata);

                    // Use the same uniform scale for drawing and shadow placement.
                    const float treeWidth = treeSource.width * GameConfig::Trees::RenderScale;
                    const float treeHeight = treeSource.height * GameConfig::Trees::RenderScale;
                    float treeFeetY =
                        y * TileSize +
                        TileSize +
                        GameConfig::Trees::AnchorOffset.y;

                        Rectangle destination = {
                            x * TileSize +
                                (TileSize - treeWidth) * 0.5f +
                                GameConfig::Trees::AnchorOffset.x,

                            treeFeetY - treeHeight,

                            treeWidth,
                            treeHeight
                        };

                    if (!visible(destination, GameConfig::Trees::Shadow)) continue;

                    renderer.DrawCaster(
                        treeTexture,
                        treeSource,
                        destination,
                        GameConfig::Trees::Shadow,
                        lightPosition,
                        shadowLength, maskColor
                    );
                }
            }
        }

        // 2. Automatically cast shadows for placed assets.
        for (const WorldObject& object : m_objects)
        {
            if (object.shadow.mode == Lighting::ShadowMode::None)continue;

            Rectangle source = {
                0.0f,
                0.0f,
                (float)object.texture.width,
                (float)object.texture.height
            };

            Rectangle destination = {
                object.position.x,
                object.position.y,
                (float)object.texture.width,
                (float)object.texture.height
            };

            if (!visible(destination, object.shadow)) continue;

            renderer.DrawCaster(
                object.texture,
                source,
                destination,
                object.shadow,
                lightPosition,
                shadowLength,
                maskColor
            );
        }
    }
}
