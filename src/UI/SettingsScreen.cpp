#include "SettingsScreen.h"
#include <raylib.h>
#include <json.hpp>
#include <fstream>
#include <cmath>
namespace UI {
SettingsScreen::SettingsScreen() {
    try {
        std::ifstream file("graphics_settings.json");
        if(!file) return;
        nlohmann::json j; file>>j;
        m_graphics.width=std::clamp(j.value("width",1280),800,7680);
        m_graphics.height=std::clamp(j.value("height",720),600,4320);
        m_graphics.windowMode=std::clamp(j.value("windowMode",0),0,2);
        m_graphics.vsync=j.value("vsync",false);
        m_graphics.fpsLimit=std::clamp(j.value("fpsLimit",0),0,1000);
        m_graphics.preset=std::clamp(j.value("preset",2),-1,3);
        m_graphics.shadows=j.value("shadows",true);
        m_graphics.gpuShadows=j.value("gpuShadows",true);
        int samples=j.value("samples",8);
        m_graphics.samples=samples<=4?4:samples<=8?8:16;
        m_graphics.shadowScale=std::clamp(j.value("shadowScale",1.0f),0.25f,1.0f);
        m_graphics.softness=std::clamp(j.value("softness",10.0f),0.0f,40.0f);
        m_graphics.opacity=std::clamp(j.value("opacity",0.85f),0.0f,1.0f);
    } catch(...) { TraceLog(LOG_WARNING,"Invalid graphics settings; using defaults"); m_graphics={}; }
}
    void SettingsScreen::Save() const {
        nlohmann::json j={{"width",m_graphics.width},{"height",m_graphics.height},
            {"windowMode",m_graphics.windowMode},{"vsync",m_graphics.vsync},{"fpsLimit",m_graphics.fpsLimit},
            {"preset",m_graphics.preset},{"shadows",m_graphics.shadows},{"gpuShadows",m_graphics.gpuShadows},
            {"samples",m_graphics.samples},{"shadowScale",m_graphics.shadowScale},
            {"softness",m_graphics.softness},{"opacity",m_graphics.opacity}};
        std::ofstream file("graphics_settings.json"); file<<j.dump(2);
    }
    Rectangle SettingsScreen::Row(int index) const {
        float width=std::min(700,GetScreenWidth()-40);
        float step=std::min(42.0f,(GetScreenHeight()-170.0f)/11.0f);
        return {(GetScreenWidth()-width)*0.5f,80+index*step,width,step-4};
    }

    void SettingsScreen::Update() {
        if(IsKeyPressed(KEY_ESCAPE)) m_back=true;
        if(!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;
        Vector2 mouse=GetMousePosition();
        if(CheckCollisionPointRec(mouse,{(GetScreenWidth()-200)*0.5f,(float)GetScreenHeight()-65,200,40})) { m_back=true; return; }
        for(int row=0;row<11;++row) if(CheckCollisionPointRec(mouse,Row(row))) {
            switch(row) {
            case 0: m_graphics.windowMode=(m_graphics.windowMode+1)%3; break;
            case 1: {
                const int widths[]={1280,1920,2560,3840},heights[]={720,1080,1440,2160};
                int i=0; while(i<4 && widths[i]!=m_graphics.width) ++i;
                i=(i+1)%4; m_graphics.width=widths[i];m_graphics.height=heights[i];break;
            }
            case 2:m_graphics.vsync=!m_graphics.vsync;break;
            case 3:{ const int caps[]={0,60,120,144,165,240}; int i=0;while(i<6 && caps[i]!=m_graphics.fpsLimit)++i;m_graphics.fpsLimit=caps[(i+1)%6];break; }
            case 4:m_graphics.SetPreset((m_graphics.preset+1)%4);break;
            case 5:m_graphics.shadows=!m_graphics.shadows;break;
            case 6:m_graphics.samples=m_graphics.samples==4?8:m_graphics.samples==8?16:4;m_graphics.preset=-1;break;
            case 7:m_graphics.shadowScale=m_graphics.shadowScale>=1?0.25f:m_graphics.shadowScale+0.25f;m_graphics.preset=-1;break;
            case 8:m_graphics.softness=m_graphics.softness>=40?0:m_graphics.softness+5;break;
            case 9:m_graphics.opacity=m_graphics.opacity>=0.99f?0:std::min(1.0f,m_graphics.opacity+0.1f);break;
            case 10:m_graphics.gpuShadows=!m_graphics.gpuShadows;break;
            }
            ++m_revision; Save(); break;
        }
    }

    void SettingsScreen::Draw() {
        ClearBackground(RAYWHITE);
        float left=Row(0).x;
        DrawText("Graphics settings",(int)left,25,30,BLACK);
        const char* names[]={"Display mode","Resolution (window/exclusive)","VSync","FPS limit","Quality preset","Shadows","Shadow samples","Shadow resolution","Shadow softness","Shadow opacity","Shadow renderer"};
        const char* modes[]={"Windowed","Borderless fullscreen","Exclusive fullscreen"};
        const char* presets[]={"Low","Medium","High","Ultra"};
        for(int i=0;i<11;++i) {
            Rectangle rect=Row(i);bool hover=CheckCollisionPointRec(GetMousePosition(),rect);
            DrawRectangleRec(rect,hover?LIGHTGRAY:Color{225,228,232,255});
            DrawText(names[i],(int)rect.x+12,(int)rect.y+7,18,DARKGRAY);
            const char* value="";
            switch(i) {
            case 0:value=modes[m_graphics.windowMode];break;
            case 1:value=TextFormat("%d x %d",m_graphics.width,m_graphics.height);break;
            case 2:value=m_graphics.vsync?"On":"Off";break;
            case 3:value=m_graphics.fpsLimit?TextFormat("%d",m_graphics.fpsLimit):"Unlimited";break;
            case 4:value=m_graphics.preset<0?"Custom":presets[m_graphics.preset];break;
            case 5:value=m_graphics.shadows?"On":"Off";break;
            case 6:value=TextFormat("%d",m_graphics.samples);break;
            case 7:value=TextFormat("%.0f%% (nearest)",m_graphics.shadowScale*100);break;
            case 8:value=TextFormat("%.0f world pixels",m_graphics.softness);break;
            case 9:value=TextFormat("%.0f%%",m_graphics.opacity*100);break;
            case 10:value=m_graphics.gpuShadows?"GPU instancing":"CPU reference";break;
            }
            DrawText(value,(int)(rect.x+rect.width*0.56f),(int)rect.y+7,18,BLACK);
        }
        DrawText("Click a row to change. F10 opens settings during play.",(int)left,GetScreenHeight()-92,16,DARKGRAY);
        Rectangle back{(GetScreenWidth()-200)*0.5f,(float)GetScreenHeight()-65,200,40};
        DrawRectangleRec(back,DARKGRAY);DrawText("Back",(int)back.x+75,(int)back.y+9,20,WHITE);
    }
}
