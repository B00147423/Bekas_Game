#pragma once

#include <raylib.h>

namespace Lighting
{
    class ShadowBuffer
    {
    public:
        ShadowBuffer();
        ~ShadowBuffer();

        void Init(int width, int height);
        void Begin();
        void End();
        void Unload();

        Texture2D GetTexture() const;

    private:
        RenderTexture2D m_renderTexture{};

        int m_width = 0;
        int m_height = 0;
    };
}