#include "OcclusionBuffer.h"

namespace Lighting
{
    OcclusionBuffer::OcclusionBuffer()
    {
    }

    OcclusionBuffer::~OcclusionBuffer()
    {
        Unload();
    }

    void OcclusionBuffer::Init(int width, int height)
    {
        m_width = width;
        m_height = height;

        m_renderTexture = LoadRenderTexture(width, height);
    }

    void OcclusionBuffer::Begin()
    {
        BeginTextureMode(m_renderTexture);

        ClearBackground(BLACK);
    }

    void OcclusionBuffer::End()
    {
        EndTextureMode();
    }

    void OcclusionBuffer::Unload()
    {
        if (m_renderTexture.id != 0)
        {
            UnloadRenderTexture(m_renderTexture);
            m_renderTexture = {};
        }

        m_width = 0;
        m_height = 0;
    }

    Texture2D OcclusionBuffer::GetTexture() const
    {
        return m_renderTexture.texture;
    }
}