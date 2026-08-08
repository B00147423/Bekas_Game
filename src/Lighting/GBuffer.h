#pragma once

#include <raylib.h>
#include "rlgl.h"

namespace Lighting
{
    class GBuffer
    {
    public:
        GBuffer() = default;
        ~GBuffer();

        GBuffer(const GBuffer&) = delete;
        GBuffer& operator=(const GBuffer&) = delete;

        void Init(int width, int height);
        void Begin();
        void End();
        void Unload();

        // Texture wrappers for the later lighting pass / debug present
        Texture2D GetAlbedoTexture() const;
        Texture2D GetNormalTexture() const;

        int GetWidth() const { return m_width; }
        int GetHeight() const { return m_height; }

    private:
        unsigned int m_framebuffer = 0;
        unsigned int m_albedoTexture = 0;
        unsigned int m_normalTexture = 0;
        Shader m_shader{};

        int m_width = 0;
        int m_height = 0;
    };
}
