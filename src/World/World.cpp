#include "World.h"

namespace World
{
    World::World(uint64_t seed)
        : m_seed(seed),
        m_worldGenerator(seed)
    {
        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = { 0.0f, 0.0f };
        m_physicsWorld = b2CreateWorld(&worldDef);
    }

    World::~World()
    {
        b2DestroyWorld(m_physicsWorld);
    }
    void World::ClearSpawnArea(int centerTileX, int centerTileY, int radius)
    {
        for (int y = centerTileY - radius; y <= centerTileY + radius; y++)
        {
            for (int x = centerTileX - radius; x <= centerTileX + radius; x++)
            {
                if (x < 0 || x >= Chunk::CHUNK_WIDTH ||
                    y < 0 || y >= Chunk::CHUNK_HEIGHT)
                {
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

    void World::Step(float deltaTime)
    {
        b2World_Step(m_physicsWorld, deltaTime, 4);
    }

    void World::DrawGrassTile(
        const Tileset& tileset,
        int tileX,
        int tileY,
        uint8_t metadata,
        const Rectangle& dest,
        Color tint) const
    {
        const unsigned hash =
            static_cast<unsigned>(tileX) * 374761393u ^
            static_cast<unsigned>(tileY) * 668265263u ^
            static_cast<unsigned>(metadata) * 1274126177u;

        const Rectangle src = tileset.GetGrass(static_cast<int>(hash & 0x7fffffffu));

        static constexpr Color kTints[] = {
            Color{ 255, 255, 255, 255 },
            Color{ 242, 255, 240, 255 },
            Color{ 232, 250, 228, 255 },
            Color{ 248, 255, 242, 255 },
            Color{ 224, 245, 220, 255 },
            Color{ 238, 252, 232, 255 },
            Color{ 250, 255, 244, 255 },
            Color{ 216, 240, 214, 255 },
        };

        const Color base = kTints[(hash >> 3) & 7u];
        const Color finalTint = {
            static_cast<unsigned char>((base.r * tint.r) / 255),
            static_cast<unsigned char>((base.g * tint.g) / 255),
            static_cast<unsigned char>((base.b * tint.b) / 255),
            255
        };

        DrawTexturePro(
            tileset.GetGrassTexture(),
            src,
            dest,
            { 0.0f, 0.0f },
            0.0f,
            finalTint
        );
    }

    void World::CreateTreeColliders()
    {
        constexpr float tileSize = 16.0f;
        constexpr float pixelsPerMeter = 32.0f;

        for (int y = 0; y < Chunk::CHUNK_HEIGHT; y++)
        {
            for (int x = 0; x < Chunk::CHUNK_WIDTH; x++)
            {
                if (m_chunk.GetTile(x, y).type != TileType::Tree)
                    continue;
                const float stumpX =static_cast<float>(x) * tileSize +tileSize * 0.5f;
                const float stumpY =static_cast<float>(y) * tileSize +12.0f;

                b2BodyDef bodyDef = b2DefaultBodyDef();
                bodyDef.type = b2_staticBody;
                bodyDef.position = {
                    stumpX / pixelsPerMeter,
                    stumpY / pixelsPerMeter
                };

                const b2BodyId bodyId =         b2CreateBody(m_physicsWorld, &bodyDef);
                const b2Polygon stumpShape =    b2MakeBox( 8.0f / pixelsPerMeter, 8.0f / pixelsPerMeter);
                b2ShapeDef shapeDef =           b2DefaultShapeDef();

                b2CreatePolygonShape(bodyId, &shapeDef,&stumpShape);
            }
        }
    }

    void World::DrawGround(const Tileset& tileset) const
    {
        constexpr float tileSize = TileSize;

        for (int x = 0; x < Chunk::CHUNK_WIDTH; x++)
        {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; y++)
            {
                const Tile tile = m_chunk.GetTile(x, y);
                if (tile.type == TileType::Air)
                    continue;

                const Rectangle dest = {
                    static_cast<float>(x) * tileSize,
                    static_cast<float>(y) * tileSize,
                    tileSize,
                    tileSize
                };

                if (tile.type == TileType::Grass || tile.type == TileType::Tree)
                    DrawGrassTile(tileset, x, y, tile.metadata, dest, WHITE);
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

        return m_flowField.GetDirection(tileX, tileY);
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
    void World::DrawOccluders() const{
        for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y){
            for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x)
            {
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

    void World::DrawTrees(const Tileset& tileset, float playerFeetY, bool behindPlayer) const{
        constexpr float tileSize = TileSize;
        const Rectangle pine = tileset.GetPineTree();

        for (int y = 0; y < Chunk::CHUNK_HEIGHT; y++)
        {
            for (int x = 0; x < Chunk::CHUNK_WIDTH; x++)
            {
                const Tile tile = m_chunk.GetTile(x, y);
                if (tile.type != TileType::Tree)
                    continue;

                const float treeFeetY = static_cast<float>(y) * tileSize + tileSize;
                const bool isBehind = treeFeetY < playerFeetY;
                if (isBehind != behindPlayer)
                    continue;

                const Rectangle dest = {
                    static_cast<float>(x) * tileSize + (tileSize - pine.width) * 0.5f,
                    static_cast<float>(y) * tileSize + tileSize - pine.height,
                    pine.width,
                    pine.height
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
}
