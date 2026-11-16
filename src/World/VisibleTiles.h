#pragma once
#include "Chunk.h"
#include <algorithm>
#include <cmath>
namespace World {
struct TileBounds { int left,top,right,bottom; };
inline float TreeExtent() {
    float extent=0;
    for(auto sprite:GameConfig::Trees::SpriteSourcesTree)
        extent=std::max(extent,std::max(sprite.width,sprite.height)*GameConfig::Trees::RenderScale);
    return extent+std::abs(GameConfig::Trees::AnchorOffset.x)+std::abs(GameConfig::Trees::AnchorOffset.y)+GameConfig::World::TileSize;
}
inline TileBounds VisibleTiles(Rectangle bounds,float padding=0) {
    constexpr float size=GameConfig::World::TileSize;
    return {std::clamp(int(std::floor((bounds.x-padding)/size)),0,Chunk::CHUNK_WIDTH),
            std::clamp(int(std::floor((bounds.y-padding)/size)),0,Chunk::CHUNK_HEIGHT),
            std::clamp(int(std::ceil((bounds.x+bounds.width+padding)/size)),0,Chunk::CHUNK_WIDTH),
            std::clamp(int(std::ceil((bounds.y+bounds.height+padding)/size)),0,Chunk::CHUNK_HEIGHT)};
}
}
