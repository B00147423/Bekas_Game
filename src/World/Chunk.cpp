#include "Chunk.h"
#include <utility>
namespace World {
Chunk::Chunk() = default;
void Chunk::SetGenerator(std::function<Tile(int,int)> generator) {
    m_pages.clear(); m_waterTiles.clear(); m_generator = std::move(generator);
}
Chunk::Page& Chunk::GetPage(int x, int y) const {
    constexpr int columns = (CHUNK_WIDTH + PageSize - 1) / PageSize;
    const int px=x/PageSize, py=y/PageSize;
    const int key=py*columns+px;
    auto found=m_pages.find(key);
    if(found!=m_pages.end()) return found->second;
    auto& page=m_pages.try_emplace(key).first->second;
    if(m_generator) for(int dy=0;dy<PageSize;++dy) for(int dx=0;dx<PageSize;++dx) {
        const int tx=px*PageSize+dx, ty=py*PageSize+dy;
        if(tx<CHUNK_WIDTH && ty<CHUNK_HEIGHT) page[dy*PageSize+dx]=m_generator(tx,ty);
    }
    return page;
}
Tile Chunk::GetTile(int x,int y) const {
    if(x<0 || y<0 || x>=CHUNK_WIDTH || y>=CHUNK_HEIGHT) return {};
    return GetPage(x,y)[(y%PageSize)*PageSize+x%PageSize];
}
void Chunk::SetTile(int x,int y,const Tile& tile) {
    if(x<0 || y<0 || x>=CHUNK_WIDTH || y>=CHUNK_HEIGHT) return;
    GetPage(x,y)[(y%PageSize)*PageSize+x%PageSize]=tile;
    const int key=y*CHUNK_WIDTH+x;
    if(tile.type==TileType::Water) m_waterTiles.insert(key); else m_waterTiles.erase(key);
}
std::vector<Vector2> Chunk::GetWaterTiles() const {
    std::vector<Vector2> result; result.reserve(m_waterTiles.size());
    for(int key:m_waterTiles) result.push_back({(key%CHUNK_WIDTH+0.5f)*GameConfig::World::TileSize,
                                               (key/CHUNK_WIDTH+0.5f)*GameConfig::World::TileSize});
    return result;
}
}
