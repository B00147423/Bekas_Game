#include "Chunk.h"

namespace World
{
    Chunk::Chunk()
    {
        for (int x = 0; x < CHUNK_WIDTH; x++)
        {
            for (int y = 0; y < CHUNK_HEIGHT; y++)
            {
                m_tiles[x][y] = Tile{};
            }
        }
    }

    Tile Chunk::GetTile(int x, int y) const
    {
        return m_tiles[x][y];
    }

    void Chunk::SetTile(int x, int y, const Tile& tile)
    {
        m_tiles[x][y] = tile;
    }
}
