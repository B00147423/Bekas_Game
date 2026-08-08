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

        int         GetCost(int x, int y) const;
        Vector2     GetDirection(int x, int y) const;

    private:
        int m_cost[Chunk::CHUNK_WIDTH][Chunk::CHUNK_HEIGHT];
        Vector2 m_direction [Chunk::CHUNK_WIDTH] [Chunk::CHUNK_HEIGHT];
    };
}