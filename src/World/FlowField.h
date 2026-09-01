#pragma once
#include "Chunk.h"
#include <raylib.h>

namespace World
{
    class FlowField
    {
    public:
        FlowField();

        void Generate(const Chunk& chunk, int targetX, int targetY);

        int GetCost(int x, int y) const;
        Vector2 GetDirection(int x, int y) const;

        // Tile itself not solid (can stand here)
        bool IsStandable(const Chunk& chunk, int x, int y) const;
        // Standable + no solid neighbors (body can pass without clipping stumps)
        bool IsPassable(const Chunk& chunk, int x, int y) const;
        void DrawFlowField(float tileSize, Vector2 worldTopLeft, Vector2 worldBottomRight) const;

    private:
        int m_cost[Chunk::CHUNK_WIDTH][Chunk::CHUNK_HEIGHT];
        Vector2 m_direction[Chunk::CHUNK_WIDTH][Chunk::CHUNK_HEIGHT];
    };
}
