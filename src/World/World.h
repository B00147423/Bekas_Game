#pragma once

#include <cstdint>
#include "Tileset.h"
#include "Generation/WorldGenerator.h"
#include "Chunk.h"
#include "FlowField.h"
#include "../Physics/PhysicsWorld.h"

namespace World
{
    class World
    {
    public:
        World(uint64_t seed, Physics::PhysicsWorld& physicsWorld);

        World(const World&) = delete;
        World& operator=(const World&) = delete;

        void Generate(Vector2 playerPosition);
        void ClearSpawnArea(int centerTileX, int centerTileY, int radius);

        void DrawGround(const Tileset& tileset) const;
        bool isForest(int tileX, int tileY) const;
        uint8_t GetForestMask(int tileX, int tileY) const;
        void DrawTrees(const Tileset& tileset, float playerFeetY, bool behindPlayer) const;
        void DrawOccluders() const;
        void DrawDebugGrid() const;
        void DrawFlowField(Vector2 worldTopLeft, Vector2 worldBottomRight) const;
        void SetFlowFieldTarget(Vector2 worldPosition);

        // Steering direction in world space (toward next tile center / escape)
        Vector2 GetFlowDirection(Vector2 worldPosition) const;
        // When blocked by physics, try a side step along the field
        Vector2 GetSlideDirection(Vector2 worldPosition, Vector2 blockedDir) const;

        static constexpr float TileSize = 16.0f;

        static constexpr float TreeScale = 1.5f;
        // Source pine stump ~8x8 at scale 1; scales with TreeScale
        static constexpr float TreeStumpSize = 8.0f;


    private:
        void DrawGrassTile(const Tileset& tileset, int tileX, int tileY, uint8_t metadata, const Rectangle& dest, Color tint) const;

        void CreateTreeColliders();

        Physics::PhysicsWorld& m_physicsWorld;

        FlowField m_flowField;
        uint64_t m_seed;

        Generation::WorldGenerator m_worldGenerator;
        Chunk m_chunk;
    };
}
