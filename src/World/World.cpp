#include "World.h"

#include <cmath>

namespace World
{
    World::World(uint64_t seed, Physics::PhysicsWorld& physicsWorld)
        : m_physicsWorld(physicsWorld),
        m_seed(seed),
        m_worldGenerator(seed)
    {
    }

    void World::ClearSpawnArea(int centerTileX, int centerTileY, int radius)
    {
        for (int y = centerTileY - radius; y <= centerTileY + radius; y++){
            for (int x = centerTileX - radius; x <= centerTileX + radius; x++){
                if (x < 0 || x >= Chunk::CHUNK_WIDTH ||
                    y < 0 || y >= Chunk::CHUNK_HEIGHT){
                    continue;
                }

                int dx = x - centerTileX;
                int dy = y - centerTileY;

                if (dx * dx + dy * dy > radius * radius)
                    continue;

                Tile tile = m_chunk.GetTile(x, y);

                if (tile.type != TileType::Tree)
                    continue;

                tile.type = TileType::Grass;
                tile.solid = false;

                m_chunk.SetTile(x, y, tile);
            }
        }
    }
    void World::Generate(Vector2 playerPosition)
    {
        m_worldGenerator.GenerateChunk(m_chunk);

        int playerTileX = static_cast<int>(playerPosition.x / TileSize);

        int playerTileY = static_cast<int>(playerPosition.y / TileSize);

        ClearSpawnArea(playerTileX, playerTileY, 3);

        CreateTreeColliders();

        m_flowField.Generate(
            m_chunk,
            playerTileX,
            playerTileY
        );
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

    void World::CreateTreeColliders()
    {
        const float stumpSize = TreeStumpSize * TreeScale;

        for (int y = 0; y < Chunk::CHUNK_HEIGHT; y++) {
            for (int x = 0; x < Chunk::CHUNK_WIDTH; x++){
                if (m_chunk.GetTile(x, y).type != TileType::Tree)
                    continue;

                // Stump at tree feet (bottom of tile), not the full canopy
                const float stumpX = static_cast<float>(x) * TileSize + TileSize * 0.5f;
                const float stumpY = static_cast<float>(y) * TileSize + TileSize - stumpSize * 0.5f;

                m_physicsWorld.CreateStaticBox({ stumpX, stumpY }, { stumpSize, stumpSize });
            }
        }
    }

    bool World::isForest(int tileX, int tileY) const
    {
        if (tileX < 0 || tileX >= Chunk::CHUNK_WIDTH ||
            tileY < 0 || tileY >= Chunk::CHUNK_HEIGHT)
        {
            return false;
        }
        return m_chunk.GetTile(tileX, tileY).biome== BiomeType::Forest;
    }
    

    uint8_t World::GetForestMask(int tileX, int tileY) const
    {
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
    
    void World::DrawGround(const Tileset& tileset) const
    {
        constexpr float tileSize = TileSize;

        for (int x = 0; x < Chunk::CHUNK_WIDTH; x++){
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; y++){
                const Tile tile = m_chunk.GetTile(x, y);

                if (tile.type == TileType::Air)
                    continue;

                const Rectangle dest = {
                    static_cast<float>(x) * tileSize,
                    static_cast<float>(y) * tileSize,
                    tileSize,
                    tileSize
                };

                const bool canDrawForest = tile.biome == BiomeType::Forest &&  tileset.GetForestTexture().id != 0;

                if (canDrawForest){  
                    const uint8_t mask = GetForestMask(x, y);

                    DrawTexturePro(
                        tileset.GetForestTexture(),
                        tileset.GetForest(mask),
                        dest,
                        {0.0f, 0.0f},
                        0.0f,
                        WHITE
                    );
                }
                else{
                    DrawGrassTile(
                        tileset,
                        x,
                        y,
                        tile.metadata,
                        dest,
                        WHITE
                    );
                }
            }
        }
    }


    void World::DrawDebugGrid()  const{
        for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y){
            for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x){
                int cost = m_flowField.GetCost(x, y);

                if (cost >= 0)
                {
                    DrawText(
                        TextFormat("%d", cost),
                        x * TileSize + 2,
                        y * TileSize + 2,
                        8,
                        DARKBLUE
                    );
                }
            }
        }
    }

    // flow FIELD DEBUG DRAWING
    void World::DrawFlowField(
        Vector2 worldTopLeft,
        Vector2 worldBottomRight) const
    {
        m_flowField.DrawFlowField(
            TileSize,
            worldTopLeft,
            worldBottomRight
        );
    }

    void World::DrawOccluders() const
    {
        for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y){
            for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x){
                const Tile& tile = m_chunk.GetTile(x, y);

                if (tile.type != TileType::Tree)
                    continue;

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

    void World::DrawTrees(const Tileset& tileset, float playerFeetY, bool behindPlayer) const
    {
        constexpr float tileSize = TileSize;
        const Rectangle pine = tileset.GetPineTree();

        for (int y = 0; y < Chunk::CHUNK_HEIGHT; y++){
            for (int x = 0; x < Chunk::CHUNK_WIDTH; x++){
                const Tile tile = m_chunk.GetTile(x, y);
                if (tile.type != TileType::Tree)
                    continue;

                const float treeFeetY = static_cast<float>(y) * tileSize + tileSize;
                const bool isBehind = treeFeetY < playerFeetY;
                if (isBehind != behindPlayer)
                    continue;

                const float treeWidth = pine.width * TreeScale;
                const float treeHeight = pine.height * TreeScale;

                const Rectangle dest = {
                    static_cast<float>(x) * tileSize + (tileSize - treeWidth) * 0.5f,
                    static_cast<float>(y) * tileSize + tileSize - treeHeight,
                    treeWidth,
                    treeHeight
                };

                DrawTexturePro(
                    tileset.GetFoliageTexture(),
                    pine,
                    dest,
                    { 0.0f, 0.0f },
                    0.0f,
                    WHITE
                );
            }
        }
    }

    void World::SetFlowFieldTarget(Vector2 worldPosition){
        const int targetX =static_cast<int>(worldPosition.x / TileSize);

        const int targetY = static_cast<int>(worldPosition.y / TileSize);

        if (targetX < 0 || targetX >= Chunk::CHUNK_WIDTH ||
            targetY < 0 || targetY >= Chunk::CHUNK_HEIGHT) 
        {
            return;
        }

        const Tile tile = m_chunk.GetTile(targetX, targetY);

        if (tile.solid)
            return;

        m_flowField.Generate(
            m_chunk,
            targetX,
            targetY
        );
    }
    
    Vector2 World::GetFlowDirection(Vector2 worldPosition) const
    {
        int tileX = static_cast<int>(worldPosition.x / TileSize);
        int tileY = static_cast<int>(worldPosition.y / TileSize);

        if (tileX < 0 || tileX >= Chunk::CHUNK_WIDTH ||
            tileY < 0 || tileY >= Chunk::CHUNK_HEIGHT)
        {
            return { 0.0f, 0.0f };
        }

        Vector2 tileDir = m_flowField.GetDirection(tileX, tileY);

        // Unreachable tile: step toward any cheaper neighbor
        if (tileDir.x == 0.0f && tileDir.y == 0.0f &&
            m_flowField.GetCost(tileX, tileY) < 0)
        {
            const int dirs[4][2] = {
                { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 }
            };

            int bestCost = 0x7fffffff;
            for (const auto& d : dirs)
            {
                const int nx = tileX + d[0];
                const int ny = tileY + d[1];
                if (nx < 0 || nx >= Chunk::CHUNK_WIDTH ||
                    ny < 0 || ny >= Chunk::CHUNK_HEIGHT)
                {
                    continue;
                }

                const int cost = m_flowField.GetCost(nx, ny);
                if (cost >= 0 && cost < bestCost)
                {
                    bestCost = cost;
                    tileDir = {
                        static_cast<float>(d[0]),
                        static_cast<float>(d[1])
                    };
                }
            }
        }

        if (tileDir.x == 0.0f && tileDir.y == 0.0f)
            return { 0.0f, 0.0f };

        // Steer toward the center of the next tile — reduces getting
        // hung up on stump corners while only using cardinal tile steps.
        const float nextCenterX =
            (static_cast<float>(tileX) + tileDir.x) * TileSize + TileSize * 0.5f;
        const float nextCenterY =
            (static_cast<float>(tileY) + tileDir.y) * TileSize + TileSize * 0.5f;

        Vector2 steer = {
            nextCenterX - worldPosition.x,
            nextCenterY - worldPosition.y
        };

        const float lenSq = steer.x * steer.x + steer.y * steer.y;
        if (lenSq < 0.0001f)
            return tileDir;

        const float invLen = 1.0f / sqrtf(lenSq);
        return { steer.x * invLen, steer.y * invLen };
    }

    Vector2 World::GetSlideDirection(Vector2 worldPosition, Vector2 blockedDir) const
    {
        int tileX = static_cast<int>(worldPosition.x / TileSize);
        int tileY = static_cast<int>(worldPosition.y / TileSize);

        if (tileX < 0 || tileX >= Chunk::CHUNK_WIDTH ||
            tileY < 0 || tileY >= Chunk::CHUNK_HEIGHT)
        {
            return { 0.0f, 0.0f };
        }

        // Prefer side steps that still go downhill on the cost field
        const Vector2 candidates[4] = {
            { -blockedDir.y, blockedDir.x },
            { blockedDir.y, -blockedDir.x },
            { -blockedDir.x, -blockedDir.y },
            blockedDir
        };

        int bestCost = 0x7fffffff;
        Vector2 best = { 0.0f, 0.0f };

        for (const Vector2& c : candidates)
        {
            // Quantize to tile step
            const int dx = (c.x > 0.3f) ? 1 : (c.x < -0.3f ? -1 : 0);
            const int dy = (c.y > 0.3f) ? 1 : (c.y < -0.3f ? -1 : 0);
            if (dx == 0 && dy == 0)
                continue;

            const int nx = tileX + dx;
            const int ny = tileY + dy;
            if (nx < 0 || nx >= Chunk::CHUNK_WIDTH ||
                ny < 0 || ny >= Chunk::CHUNK_HEIGHT)
            {
                continue;
            }

            const int cost = m_flowField.GetCost(nx, ny);
            if (cost >= 0 && cost < bestCost)
            {
                bestCost = cost;
                const float nextCenterX = static_cast<float>(nx) * TileSize + TileSize * 0.5f;
                const float nextCenterY = static_cast<float>(ny) * TileSize + TileSize * 0.5f;
                Vector2 steer = {
                    nextCenterX - worldPosition.x,
                    nextCenterY - worldPosition.y
                };
                const float lenSq = steer.x * steer.x + steer.y * steer.y;
                if (lenSq > 0.0001f)
                {
                    const float invLen = 1.0f / sqrtf(lenSq);
                    best = { steer.x * invLen, steer.y * invLen };
                }
            }
        }

        return best;
    }
}
