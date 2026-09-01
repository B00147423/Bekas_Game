#include "FlowField.h"

#include <queue>
#include <utility>
#include <algorithm>
namespace World
{
    namespace
    {
        constexpr int kCardinal[4][2] = {
            { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 }
        };

        constexpr int kDirs8[8][2] = {
            { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 },
            { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 }
        };
    }

    FlowField::FlowField()
    {
        for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x)
        {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y)
                m_cost[x][y] = -1;
        }
    }

    bool FlowField::IsStandable(const Chunk& chunk, int x, int y) const
    {
        if (x < 0 || x >= Chunk::CHUNK_WIDTH ||
            y < 0 || y >= Chunk::CHUNK_HEIGHT)
        {
            return false;
        }

        return !chunk.GetTile(x, y).solid;
    }

    bool FlowField::IsPassable(const Chunk& chunk, int x, int y) const
    {
        if (!IsStandable(chunk, x, y))
            return false;

        // Paths stay one tile clear of trees so the physics body
        // doesn't clip stump colliders in neighboring tiles.
        for (const auto& d : kCardinal)
        {
            const int nx = x + d[0];
            const int ny = y + d[1];

            if (nx < 0 || nx >= Chunk::CHUNK_WIDTH ||
                ny < 0 || ny >= Chunk::CHUNK_HEIGHT)
            {
                return false;
            }

            if (chunk.GetTile(nx, ny).solid)
                return false;
        }

        return true;
    }

    void FlowField::Generate(const Chunk& chunk, int targetX, int targetY)
    {
        for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x)
        {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y)
                m_cost[x][y] = -1;
        }

        if (!IsStandable(chunk, targetX, targetY))
            return;

        std::queue<std::pair<int, int>> queue;

        m_cost[targetX][targetY] = 0;
        queue.push({ targetX, targetY });

        // 1) Flood open space (passable tiles) + direct standable neighbors of target
        while (!queue.empty())
        {
            auto [x, y] = queue.front();
            queue.pop();

            const bool fromTarget = (x == targetX && y == targetY);

            for (const auto& direction : kDirs8)
            {
                const int nx = x + direction[0];
                const int ny = y + direction[1];

                if (nx < 0 || nx >= Chunk::CHUNK_WIDTH ||
                    ny < 0 || ny >= Chunk::CHUNK_HEIGHT)
                {
                    continue;
                }

                if (m_cost[nx][ny] != -1)
                    continue;

                if (!IsStandable(chunk, nx, ny))
                    continue;

                // No cutting diagonally through tree corners
                if (direction[0] != 0 && direction[1] != 0)
                {
                    if (!IsStandable(chunk, x + direction[0], y) ||
                        !IsStandable(chunk, x, y + direction[1]))
                    {
                        continue;
                    }
                }

                // From the click target, allow one step onto any standable tile
                // (even beside trees). Everywhere else only expand through passable space.
                if (!fromTarget && !IsPassable(chunk, nx, ny))
                    continue;

                // Only keep expanding the BFS through passable tiles (or from target)
                if (!fromTarget && !IsPassable(chunk, x, y))
                    continue;

                m_cost[nx][ny] = m_cost[x][y] + 1;

                if (IsPassable(chunk, nx, ny) || fromTarget)
                    queue.push({ nx, ny });
            }
        }

        // 2) Pull standable-but-tight tiles into the field from nearby costs
        //    so you can walk out toward open space if you're beside a tree.
        for (int pass = 0; pass < 2; ++pass)
        {
            for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y)
            {
                for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x)
                {
                    if (m_cost[x][y] >= 0)
                        continue;
                    if (!IsStandable(chunk, x, y))
                        continue;

                    int best = -1;
                    for (const auto& d : kCardinal)
                    {
                        const int nx = x + d[0];
                        const int ny = y + d[1];
                        if (nx < 0 || nx >= Chunk::CHUNK_WIDTH ||
                            ny < 0 || ny >= Chunk::CHUNK_HEIGHT)
                        {
                            continue;
                        }

                        const int c = m_cost[nx][ny];
                        if (c >= 0 && (best < 0 || c < best))
                            best = c;
                    }

                    if (best >= 0)
                        m_cost[x][y] = best + 1;
                }
            }
        }

        // 3) Directions: step to lowest-cost neighbor (8-way)
        for (int y = 0; y < Chunk::CHUNK_HEIGHT; ++y)
        {
            for (int x = 0; x < Chunk::CHUNK_WIDTH; ++x)
            {
                m_direction[x][y] = { 0.0f, 0.0f };

                const int currentCost = m_cost[x][y];
                if (currentCost <= 0)
                    continue;

                int bestCost = currentCost;

                for (const auto& direction : kDirs8)
                {
                    const int nx = x + direction[0];
                    const int ny = y + direction[1];

                    if (nx < 0 || nx >= Chunk::CHUNK_WIDTH ||
                        ny < 0 || ny >= Chunk::CHUNK_HEIGHT)
                    {
                        continue;
                    }

                    if (direction[0] != 0 && direction[1] != 0)
                    {
                        if (!IsStandable(chunk, x + direction[0], y) ||
                            !IsStandable(chunk, x, y + direction[1]))
                        {
                            continue;
                        }
                    }

                    const int neighborCost = m_cost[nx][ny];
                    if (neighborCost >= 0 && neighborCost < bestCost)
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

void FlowField::DrawFlowField(
    float tileSize,
    Vector2 worldTopLeft,
    Vector2 worldBottomRight) const
{
    int startX = static_cast<int>(worldTopLeft.x / tileSize);
    int startY = static_cast<int>(worldTopLeft.y / tileSize);

    int endX = static_cast<int>(worldBottomRight.x / tileSize) + 1;
    int endY = static_cast<int>(worldBottomRight.y / tileSize) + 1;

    startX = std::clamp(startX, 0, Chunk::CHUNK_WIDTH - 1);
    startY = std::clamp(startY, 0, Chunk::CHUNK_HEIGHT - 1);

    endX = std::clamp(endX, 0, Chunk::CHUNK_WIDTH);
    endY = std::clamp(endY, 0, Chunk::CHUNK_HEIGHT);

    for (int y = startY; y < endY; ++y)
    {
        for (int x = startX; x < endX; ++x)
        {
            if (GetCost(x, y) < 0)
                continue;

            const Vector2 dir = GetDirection(x, y);

            if (dir.x == 0.0f && dir.y == 0.0f)
                continue;

            const Vector2 start = {
                x * tileSize + tileSize * 0.5f,
                y * tileSize + tileSize * 0.5f
            };

            const Vector2 end = {
                start.x + dir.x * tileSize * 0.4f,
                start.y + dir.y * tileSize * 0.4f
            };

            DrawLineV(start, end, SKYBLUE);
        }
    }
}
}
