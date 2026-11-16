#include "World.h"
#include "VisibleTiles.h"
#include "Generation/PondShape.h"
#include "./Generation/WorldGenerator.h"
namespace World
{
    World::World(uint64_t seed, Physics::PhysicsWorld& physicsWorld) : m_physicsWorld(physicsWorld),
        m_physicsWorldId(physicsWorld.GetWorld()),
        m_seed(seed),
        m_worldGenerator(seed){
    }

    void World::ClearSpawnArea(int centerTileX, int centerTileY, int radius){
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

                if (tile.type != TileType::Tree) continue;

                tile.type = TileType::Grass;
                tile.solid = false;

                m_chunk.SetTile(x, y, tile);
            }
        }
    }

// void World::GenerateStartingForest(int centerX, int centerY)
// {
//     std::mt19937 rng(static_cast<uint32_t>(m_seed));
//     std::uniform_int_distribution<int> treeType(0, 10);

//     constexpr int borderThickness = 2;

//     for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y)
//     {
//         for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x)
//         {
//             bool isBorder =
//                 x < borderThickness ||
//                 x >= Chunk::CHUNK_WIDTH - borderThickness ||
//                 y < borderThickness ||
//                 y >= Chunk::CHUNK_HEIGHT - borderThickness;

//             if (!isBorder)
//                 continue;

//             Tile tile = m_chunk.GetTile(x, y);

//             tile.type = TileType::Tree;
//             tile.solid = true;
//             tile.metadata = static_cast<uint8_t>(treeType(rng));

//             m_chunk.SetTile(x, y, tile);
//         }
//     }
// }


    bool World::IsTreeAt(Vector2 worldPosition) const
    {
        int tileX = static_cast<int>(std::floor(worldPosition.x / TileSize));
        int tileY = static_cast<int>(std::floor(worldPosition.y / TileSize));

        if (tileX < 0 || tileX >= Chunk::CHUNK_WIDTH ||
            tileY < 0 || tileY >= Chunk::CHUNK_HEIGHT)
        {
            return false;
        }

        return m_chunk.GetTile(tileX, tileY).type == TileType::Tree;
    }

    bool World::RemoveTree(Vector2 worldPosition)
    {
        const int tileX = static_cast<int>(
            std::floor(worldPosition.x / TileSize)
        );

        const int tileY = static_cast<int>(
            std::floor(worldPosition.y / TileSize)
        );

        if (tileX < 0 || tileX >= Chunk::CHUNK_WIDTH ||
            tileY < 0 || tileY >= Chunk::CHUNK_HEIGHT)
        {
            return false;
        }

        Tile tile = m_chunk.GetTile(tileX, tileY);

        if (tile.type != TileType::Tree)
            return false;

        tile.type = TileType::Grass;
        tile.solid = false;
        tile.metadata = 0;

        m_chunk.SetTile(tileX, tileY, tile);

        const int key=tileY*Chunk::CHUNK_WIDTH+tileX;
        if(auto it=m_treeColliders.find(key);it!=m_treeColliders.end()) {
            m_physicsWorld.DestroyBody(it->second);m_treeColliders.erase(it);
        }

        return true;
    }
    bool World::FindTreeApproachPoint(Vector2 treePosition, Vector2 playerPosition,Vector2& approachPoint ) const
    {
        const int treeX = static_cast<int>(std::floor(treePosition.x / TileSize));
        const int treeY = static_cast<int>(std::floor(treePosition.y / TileSize));

        const int playerX = static_cast<int>(std::floor(playerPosition.x / TileSize));
        const int playerY = static_cast<int>(std::floor(playerPosition.y / TileSize));

        if (treeX < 0 || treeX >= Chunk::CHUNK_WIDTH ||
            treeY < 0 || treeY >= Chunk::CHUNK_HEIGHT ||
            playerX < 0 || playerX >= Chunk::CHUNK_WIDTH ||
            playerY < 0 || playerY >= Chunk::CHUNK_HEIGHT)
            return false;

        if (m_chunk.GetTile(treeX, treeY).type != TileType::Tree)
            return false;

        const int directions[4][2] = {
            { 1, 0 },
            {-1, 0 },
            { 0, 1 },
            { 0,-1 }
        };

        int bestCost = INT_MAX;
        bool found = false;

        for (const auto& direction : directions)
        {
            const int x = treeX + direction[0];
            const int y = treeY + direction[1];

            if (x < 0 || x >= Chunk::CHUNK_WIDTH ||
                y < 0 || y >= Chunk::CHUNK_HEIGHT)
                continue;

            if (m_chunk.GetTile(x, y).solid)
                continue;

            // Calculate a path from this tile to the player.
            FlowField testField;
            testField.Generate(m_chunk, x, y, playerX, playerY);

            const int cost = testField.GetCost(playerX, playerY);

            // Negative cost means the player cannot reach this tile.
            if (cost < 0)
                continue;

            if (cost < bestCost)
            {
                bestCost = cost;

                approachPoint = {
                    (x + 0.5f) * TileSize,
                    (y + 0.5f) * TileSize
                };

                found = true;
            }
        }
        return found;
    }

    void World::Generate(Vector2 playerPosition) {
        ClearColliders();
        m_activePosition = playerPosition;
        m_exploredFogPixels.clear();

        m_worldGenerator.GenerateChunk(m_chunk);

        int playerTileX = static_cast<int>(playerPosition.x / TileSize);
        int playerTileY = static_cast<int>(playerPosition.y / TileSize);

        ClearSpawnArea(
            playerTileX,
            playerTileY,
            GameConfig::World::SpawnClearRadius
        );

        //GenerateStartingForest(playerTileX, playerTileY);

        // Seeded position, proportions, rotation and shoreline. Cliffs and water
        // continue to derive from these same tiles.
        for (const auto& [x,y] : Generation::MakePondShape(m_seed, playerTileX, playerTileY,
                Chunk::CHUNK_WIDTH, Chunk::CHUNK_HEIGHT)) {
                Tile tile = m_chunk.GetTile(x, y);
                if (tile.type != TileType::Grass && tile.type != TileType::Tree) continue;
                tile.metadata = 0;
                tile.type = TileType::Water;
                tile.solid = false; // Shallow water: retain existing top-down movement.
                m_chunk.SetTile(x, y, tile);
        }

        // Finalize pond clearance before creating colliders/navigation. Use the
        // entire rendered tree footprint so large canopies cannot cover the water.
        const auto pondTiles = GetWaterTiles();
        constexpr float shoreClearance = TileSize * 2.0f;
        Rectangle pondBounds{playerPosition.x,playerPosition.y,0,0};
        if(!pondTiles.empty()) {
            float left=pondTiles.front().x,right=left,top=pondTiles.front().y,bottom=top;
            for(auto tile:pondTiles) { left=std::min(left,tile.x);right=std::max(right,tile.x);top=std::min(top,tile.y);bottom=std::max(bottom,tile.y); }
            pondBounds={left,top,right-left,bottom-top};
        }
        const auto clearanceBounds=VisibleTiles(pondBounds,TreeExtent()+shoreClearance+TileSize);
        for (int y=clearanceBounds.top; y<clearanceBounds.bottom; ++y)
            for (int x=clearanceBounds.left; x<clearanceBounds.right; ++x) {
                Tile tree = m_chunk.GetTile(x,y);
                if (tree.type != TileType::Tree) continue;
                const Rectangle sprite = GameConfig::Trees::SpriteSourcesTree[
                    tree.metadata % GameConfig::Trees::SpriteSourcesTree.size()];
                const float width = sprite.width * GameConfig::Trees::RenderScale;
                const float height = sprite.height * GameConfig::Trees::RenderScale;
                const float feet = (y+1)*TileSize + GameConfig::Trees::AnchorOffset.y;
                const Rectangle footprint{
                    x*TileSize+(TileSize-width)*0.5f+GameConfig::Trees::AnchorOffset.x,
                    feet-height, width, height};
                bool tooClose = false;
                for (Vector2 water : pondTiles) {
                    const Rectangle exclusion{water.x-TileSize*0.5f-shoreClearance,
                        water.y-TileSize*0.5f-shoreClearance,
                        TileSize+shoreClearance*2,TileSize+shoreClearance*2};
                    if (CheckCollisionRecs(footprint,exclusion)) {tooClose=true;break;}
                }
                if (tooClose) {
                    tree.type=TileType::Grass;
                    tree.solid=false;
                    tree.metadata=0;
                    m_chunk.SetTile(x,y,tree);
                }
            }

        CreateTreeColliders();

        for (const WorldObject& object : m_objects)
            CreateObjectCollider(object);

        GenerateFogRegions();

        m_hasFlowTarget = false;
        m_flowField.Generate(
            m_chunk,
            playerTileX,
            playerTileY
        );
    }

    const Chunk& World::GetChunk() const {
        return m_chunk;
    }

    const std::vector<WorldObject>& World::GetObjects() const {
        return m_objects;
    }

    const Texture2D& World::GetExplorationTexture() const {
        return m_explorationTexture;
    }
}
