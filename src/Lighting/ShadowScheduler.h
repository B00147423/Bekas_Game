#pragma once
#include <raylib.h>
#include <algorithm>
#include <cmath>
#include <span>
#include <vector>
#include <cstdint>
namespace Lighting {
struct ShadowLight2D {
    unsigned int id=0;
    Vector2 position{};
    float radius=0,softness=0,opacity=1,priority=1;
    int samples=8;
};
struct ShadowWork {
    ShadowLight2D light;
    Rectangle screenBounds{};
    int samples=1;
};
// Scheduling is independent of geometry and light compositing. Call once per
// frame for all lights; consume masks sequentially into a future light buffer.
// Off-screen lights and lights beyond explicit budgets get no shadow job, not
// an unbounded number of full-screen targets. Stable IDs resolve priority ties.
class ShadowScheduler {
public:
    struct Budget {
        int maxLights=4;
        int maxSamplePasses=32;
        uint64_t maxPixelSamples=UINT64_MAX;
    };
    static std::vector<ShadowWork> Select(std::span<const ShadowLight2D> lights, const Camera2D& camera,int width,int height,float maskScale,const Budget& budget) {
        std::vector<ShadowWork> candidates, result;
        for(const auto& light:lights) {
            if(light.radius<=0 || light.opacity<=0) continue;
            Vector2 center=GetWorldToScreen2D(light.position,camera);
            float radius=light.radius*camera.zoom;
            float left=std::clamp(std::floor(center.x-radius)-1,0.0f,(float)width);
            float top=std::clamp(std::floor(center.y-radius)-1,0.0f,(float)height);
            float right=std::clamp(std::ceil(center.x+radius)+1,0.0f,(float)width);
            float bottom=std::clamp(std::ceil(center.y+radius)+1,0.0f,(float)height);
            if(right<=left || bottom<=top) continue;
            int samples=light.softness<=0?1:light.samples<=4?4:light.samples<=8?8:16;
            candidates.push_back({light,{left,top,right-left,bottom-top},samples});
        }
        std::stable_sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){
            return a.light.priority!=b.light.priority?a.light.priority>b.light.priority:a.light.id<b.light.id;
        });
        int passes=0;uint64_t pixels=0;
        for(const auto& job:candidates) {
            uint64_t cost=(uint64_t)std::ceil(job.screenBounds.width*maskScale)*
                (uint64_t)std::ceil(job.screenBounds.height*maskScale)*job.samples;
            if((int)result.size()>=budget.maxLights || passes+job.samples>budget.maxSamplePasses ||
                cost>budget.maxPixelSamples-pixels) continue;
            result.push_back(job);passes+=job.samples;pixels+=cost;
        }
        return result;
    }
};
}
