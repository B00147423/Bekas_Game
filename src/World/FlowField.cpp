#include "FlowField.h"

#include <queue>
#include <utility>

namespace World
{
    FlowField::FlowField()
    {
        for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x)
        {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y)
            {
                m_cost[x][y] = -1;
            }
        }
    }

    void FlowField::Generate(const Chunk& chunk, int targetX, int targetY)
    {
        // Reset the cost grid
        for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x)
        {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y)
            {
                m_cost[x][y] = -1;
            }
        }

        std::queue<std::pair<int, int>> queue;

        m_cost[targetX][targetY] = 0;
        queue.push({ targetX, targetY });

        const int directions[4][2] = {
            { 1,  0 },
            {-1,  0 },
            { 0,  1 },
            { 0, -1 }
        };

        // -------------------------
        // 1. BUILD COST FIELD
        // -------------------------

        while (!queue.empty())
        {
            auto [x, y] = queue.front();
            queue.pop();

            for (const auto& direction : directions)
            {
                int nx = x + direction[0];
                int ny = y + direction[1];

                if (nx < 0 || nx >= Chunk::CHUNK_WIDTH ||
                    ny < 0 || ny >= Chunk::CHUNK_HEIGHT)
                {
                    continue;
                }

                if (m_cost[nx][ny] != -1)
                    continue;

                Tile tile = chunk.GetTile(nx, ny);

                if (tile.solid)
                    continue;

                m_cost[nx][ny] = m_cost[x][y] + 1;

                queue.push({ nx, ny });
            }
        }

        // -------------------------
        // 2. BUILD FLOW DIRECTIONS
        // -------------------------

        for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y)
        {
            for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x)
            {
                m_direction[x][y] = { 0.0f, 0.0f };

                int currentCost = m_cost[x][y];

                // -1 = unreachable/blocked
                //  0 = destination
                if (currentCost <= 0)
                    continue;

                int bestCost = currentCost;

                for (const auto& direction : directions)
                {
                    int nx = x + direction[0];
                    int ny = y + direction[1];

                    if (nx < 0 || nx >= Chunk::CHUNK_WIDTH ||
                        ny < 0 || ny >= Chunk::CHUNK_HEIGHT)
                    {
                        continue;
                    }

                    int neighborCost = m_cost[nx][ny];

                    if (neighborCost >= 0 &&
                        neighborCost < bestCost)
                    {
                        bestCost = neighborCost;

                        m_direction[x][y] = {
                            static_cast<float>(direction[0]),
                            static_cast<float>(direction[1])
                        };
                    }
                }
            }
        }
    }

    int FlowField::GetCost(int x, int y) const
    {
        return m_cost[x][y];
    }

    Vector2 FlowField::GetDirection(int x, int y) const
    {
        return m_direction[x][y];
    }
}