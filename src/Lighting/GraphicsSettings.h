#pragma once
#include <algorithm>
namespace Lighting {
// Player-facing settings, independent of the window/UI and scene representation.
struct GraphicsSettings {
    int width=1280,height=720,windowMode=0; // 0 windowed, 1 borderless, 2 exclusive
    bool vsync=false;
    int fpsLimit=0;
    int preset=2; // Low, Medium, High, Ultra; -1 custom
    bool shadows=true,gpuShadows=true;
    int samples=8;
    float shadowScale=1.0f,softness=10.0f,opacity=0.85f;
    void SetPreset(int value) {
        preset=std::clamp(value,0,3);
        samples=preset==0?4:preset==3?16:8;
        shadowScale=preset<=1?0.5f:1.0f;
        gpuShadows=true; shadows=true;
    }
};
}
