#pragma once

#include "Tile.h"

namespace World
{
    class Chunk
    {
    public:
        Chunk();

        Tile GetTile(int x, int y) const;
        void SetTile(int x, int y, const Tile& tile);

        static constexpr int CHUNK_WIDTH = 128;
        static constexpr int CHUNK_HEIGHT = 128;

    private:
        struct ChunkPosition
        {
            int x = 0;
            int y = 0;
        };

        Tile m_tiles[CHUNK_WIDTH][CHUNK_HEIGHT];
        ChunkPosition m_position;
        bool m_generated = false;
    };
}
