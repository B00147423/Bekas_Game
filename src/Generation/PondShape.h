#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>
#include <utility>
#include <vector>

namespace World::Generation {
// Independent seed stream: pond choices do not change forest generation.
inline std::vector<std::pair<int,int>> MakePondShape(uint64_t seed, int playerX, int playerY,
                                                    int width, int height) {
    std::mt19937_64 rng(seed ^ 0x706f6e6453686170ULL);
    auto range = [&](float low, float high) {
        return std::uniform_real_distribution<float>(low,high)(rng);
    };
    constexpr float pi = 3.14159265f;
    const float rx = range(4.0f,7.0f), ry = range(3.0f,5.0f);
    const float rotation = range(-0.65f,0.65f);
    const float phaseA = range(0,2*pi), phaseB = range(0,2*pi);
    const float cx = std::clamp(playerX + range(15,20), 11.0f, float(width-12));
    const float cy = std::clamp(playerY + range(-7,7), 11.0f, float(height-12));
    const float c = std::cos(rotation), s = std::sin(rotation);
    std::vector<std::pair<int,int>> tiles;
    for (int y=std::max(1,int(cy)-10);y<=std::min(height-2,int(cy)+10);++y)
        for (int x=std::max(1,int(cx)-10);x<=std::min(width-2,int(cx)+10);++x) {
            const float dx=x-cx, dy=y-cy;
            const float u=(c*dx+s*dy)/rx, v=(-s*dx+c*dy)/ry;
            const float angle=std::atan2(v,u);
            // Broad lobes and bays, rather than independent noisy tile edges.
            const float radius=1.0f + 0.13f*std::sin(3*angle+phaseA)
                                    + 0.08f*std::sin(2*angle+phaseB);
            if (u*u+v*v <= radius*radius) tiles.emplace_back(x,y);
        }
    return tiles;
}
}
