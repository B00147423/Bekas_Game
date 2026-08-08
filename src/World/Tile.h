#pragma once

#include <cstdint>

namespace World
{

    enum TileType
    {
        Air,
        Grass,
        Dirt,
        Stone,
        Water,
        Tree,
        CoalOre,
        IronOre,
    };

    struct Tile
    {
        TileType type = TileType::Air;
        bool solid = false;
        uint8_t metadata = 0;
    };

}
