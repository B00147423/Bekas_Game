#include "ShadowBuffer.h"

namespace Lighting
{
    ShadowBuffer::ShadowBuffer()
    {
    }

    ShadowBuffer::~ShadowBuffer()
    {
        Unload();
    }

    void ShadowBuffer::Init(int width, int height)
    {
        Unload();

        m_width = width;
        m_height = height;

        m_renderTexture = LoadRenderTexture(width, height);
    }

    void ShadowBuffer::Begin()
    {
        BeginTextureMode(m_renderTexture);

        ClearBackground(BLACK);
    }

    void ShadowBuffer::End()
    {
        EndTextureMode();
    }

    void ShadowBuffer::Unload()
    {
        if (m_renderTexture.id != 0)
        {
            UnloadRenderTexture(m_renderTexture);
            m_renderTexture = {};
        }

        m_width = 0;
        m_height = 0;
    }

    Texture2D ShadowBuffer::GetTexture() const
    {
        return m_renderTexture.texture;
    }
}