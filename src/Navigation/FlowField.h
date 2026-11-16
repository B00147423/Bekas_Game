#pragma once
#include "World/Chunk.h"
#include <raylib.h>

namespace World
{
    class FlowField
    {
    public:
        FlowField();

        void Generate(const Chunk& chunk, int targetX, int targetY, int startX = -1, int startY = -1, int searchMargin = 64);

        int GetCost(int x, int y) const;
        Vector2 GetDirection(int x, int y) const;

        // Tile itself not solid (can stand here)
        bool IsStandable(const Chunk& chunk, int x, int y) const;
        // Standable + no solid neighbors (body can pass without clipping stumps)
        bool IsPassable(const Chunk& chunk, int x, int y) const;
        void DrawFlowField(float tileSize, Vector2 worldTopLeft, Vector2 worldBottomRight) const;

    private:
        int m_left=0,m_top=0,m_right=0,m_bottom=0;
        std::vector<int> m_cost;
        std::vector<Vector2> m_direction;
        bool Contains(int x,int y) const { return x>=m_left && y>=m_top && x<m_right && y<m_bottom; }
        int Index(int x,int y) const { return (y-m_top)*(m_right-m_left)+x-m_left; }
        int& Cost(int x,int y) { return m_cost[Index(x,y)]; }
        Vector2& Direction(int x,int y) { return m_direction[Index(x,y)]; }
    };
}
