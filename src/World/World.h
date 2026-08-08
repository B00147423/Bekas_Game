#pragma once

#include "Tileset.h"
#include "Generation/WorldGenerator.h"
#include "Chunk.h"
#include <box2d/box2d.h>
#include <cstdint>
#include "FlowField.h"
namespace World
{
    class World
    {
    public:
        explicit World(uint64_t seed);
        ~World();

        World(const World&) = delete;
        World& operator=(const World&) = delete;

        void Generate(Vector2 playerPosition);
        void ClearSpawnArea(int centerTileX, int centerTileY, int radius);
        void Step(float deltaTime);
        void DrawGround(const Tileset& tileset) const;
        void DrawTrees(const Tileset& tileset, float playerFeetY, bool behindPlayer) const;
        void DrawOccluders() const;
        void DrawDebugGrid() const;
        void SetFlowFieldTarget(Vector2 worldPosition);
        Vector2 GetFlowDirection(Vector2 worldPosition) const;
        
        b2WorldId GetPhysicsWorld() const { return m_physicsWorld; }
        static constexpr float PixelsPerMeter = 32.0f;
        static constexpr float TileSize = 16.0f;

    private:
        void DrawGrassTile(
            const Tileset& tileset,
            int tileX,
            int tileY,
            uint8_t metadata,
            const Rectangle& dest,
            Color tint
        ) const;

        void CreateTreeColliders();


        FlowField m_flowField;
        b2WorldId m_physicsWorld{};
        uint64_t m_seed;

        Generation::WorldGenerator m_worldGenerator;
        Chunk m_chunk;
    };
}
