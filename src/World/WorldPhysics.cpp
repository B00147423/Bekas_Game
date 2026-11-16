#include "World.h"
#include "VisibleTiles.h"

#include <cmath>

namespace World
{
    void World::ClearColliders()
    {
        // Do not dereference PhysicsWorld if its simulation has been destroyed.
        // Checking the full world ID also distinguishes a reused Box2D world slot.
        if (b2World_IsValid(m_physicsWorldId)) {
            for (b2BodyId body : m_colliders) m_physicsWorld.DestroyBody(body);
            for (auto [key,body] : m_treeColliders) m_physicsWorld.DestroyBody(body);
        }
        m_colliders.clear();
        m_treeColliders.clear();
        m_activeBucketX = m_activeBucketY = -10000;
    }

    void World::CreateTreeColliders()
    {
        m_activeBucketX = m_activeBucketY = -10000;
        UpdateActiveArea(m_activePosition);
    }

    void World::UpdateActiveArea(Vector2 position) {
        m_activePosition=position;
        const int bx=int(std::floor(position.x/(TileSize*8))), by=int(std::floor(position.y/(TileSize*8)));
        if(bx==m_activeBucketX && by==m_activeBucketY) return;
        m_activeBucketX=bx;m_activeBucketY=by;
        const auto bounds=VisibleTiles({position.x,position.y,0,0},TileSize*24);
        for(auto it=m_treeColliders.begin();it!=m_treeColliders.end();) {
            const int x=it->first%Chunk::CHUNK_WIDTH,y=it->first/Chunk::CHUNK_WIDTH;
            if(x<bounds.left || x>=bounds.right || y<bounds.top || y>=bounds.bottom) {
                m_physicsWorld.DestroyBody(it->second);it=m_treeColliders.erase(it);
            } else ++it;
        }
        for (int y = bounds.top; y < bounds.bottom; y++) {
            for (int x = bounds.left; x < bounds.right; x++){
                const int key=y*Chunk::CHUNK_WIDTH+x;
                if(m_treeColliders.contains(key)) continue;
                const Tile tile = m_chunk.GetTile(x, y);
                if (tile.type != TileType::Tree)
                    continue;

                const Vector2 stumpSize = GameConfig::Trees::ColliderSizes[
                    tile.metadata % GameConfig::Trees::ColliderSizes.size()];

                // Stump at tree feet (bottom of tile), not the full canopy
                const float stumpX = static_cast<float>(x) * TileSize + TileSize * 0.5f + GameConfig::Trees::AnchorOffset.x + GameConfig::Trees::ColliderOffset.x;
                const float stumpY = static_cast<float>(y) * TileSize + TileSize - stumpSize.y * 0.5f + GameConfig::Trees::AnchorOffset.y + GameConfig::Trees::ColliderOffset.y;

                m_treeColliders.emplace(key,m_physicsWorld.CreateStaticBox({ stumpX, stumpY }, stumpSize));
            }
        }
    }

        bool World::CanPlaceObject(Vector2 position, Vector2 size) const
        {
            const int minX = static_cast<int>(std::floor(position.x / TileSize));
            const int minY = static_cast<int>(std::floor(position.y / TileSize));

            const int maxX = static_cast<int>(std::floor((position.x + size.x - 0.001f) / TileSize));
            const int maxY = static_cast<int>(std::floor((position.y + size.y - 0.001f) / TileSize));

            if (minX < 0 || minY < 0 || maxX >= Chunk::CHUNK_WIDTH || maxY >= Chunk::CHUNK_HEIGHT)
                return false;

            for (int y = minY; y <= maxY; ++y)
            {
                for (int x = minX; x <= maxX; ++x)
                {
                    if (m_chunk.GetTile(x, y).solid)
                        return false;
                }
            }

            return true;
        }

    void World::AddObject(Texture2D texture, Vector2 position, Vector2 colliderSize, Vector2 colliderOffset, Lighting::ShadowStyle shadow, bool hasCollision)
    {
        if (texture.id == 0) return;

        if (hasCollision && (colliderSize.x <= 0.0f || colliderSize.y <= 0.0f))
        {
            TraceLog(LOG_WARNING, "AddObject: collidable objects require positive explicit collider dimensions");
            return;
        }
        m_objects.push_back({texture, position, shadow, hasCollision, colliderSize, colliderOffset});
        CreateObjectCollider(m_objects.back());
    }

    void World::CreateObjectCollider(const WorldObject& object)
    {
        if (!object.hasCollision) return;

        const float width = object.texture.width;
        const float height = object.texture.height;
        const Vector2 colliderCenter = {
            object.position.x + width * 0.5f + object.colliderOffset.x,
            object.position.y + height - object.colliderSize.y * 0.5f + object.colliderOffset.y
        };
        m_colliders.push_back(m_physicsWorld.CreateStaticBox(colliderCenter, object.colliderSize));
    }
}
