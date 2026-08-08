#include "GBuffer.h"

namespace Lighting
{
    GBuffer::~GBuffer()
    {
        Unload();
    }

    void GBuffer::Init(int width, int height)
    {
        Unload();

        m_width = width;
        m_height = height;

        // Default vertex shader + custom MRT fragment shader
        m_shader = LoadShader(nullptr, "src/shaders/gbuffer.fs");
        if (!IsShaderValid(m_shader))
            TraceLog(LOG_WARNING, "GBuffer: Failed to load gbuffer.fs");

        m_framebuffer = rlLoadFramebuffer();
        if (m_framebuffer == 0)
        {
            TraceLog(LOG_WARNING, "GBuffer: Failed to create framebuffer");
            return;
        }

        rlEnableFramebuffer(m_framebuffer);

        m_albedoTexture = rlLoadTexture(nullptr, width, height, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);

        m_normalTexture = rlLoadTexture(nullptr, width, height, RL_PIXELFORMAT_UNCOMPRESSED_R8G8B8A8, 1);

        rlFramebufferAttach(m_framebuffer, m_albedoTexture, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);

        rlFramebufferAttach(m_framebuffer,m_normalTexture, RL_ATTACHMENT_COLOR_CHANNEL1, RL_ATTACHMENT_TEXTURE2D, 0);

        rlActiveDrawBuffers(2);

        if (!rlFramebufferComplete(m_framebuffer))
            TraceLog(LOG_WARNING, "GBuffer: Framebuffer is not complete");

        // rlFramebufferComplete unbinds the FBO
        rlDisableFramebuffer();
    }

    void GBuffer::Begin()
    {
        rlDrawRenderBatchActive();

        rlEnableFramebuffer(m_framebuffer);

        rlViewport(0, 0, m_width, m_height);
        rlSetFramebufferWidth(m_width);
        rlSetFramebufferHeight(m_height);

        // Match BeginTextureMode / 2D screen space (top-left origin)
        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();
        rlOrtho(0.0, static_cast<double>(m_width), static_cast<double>(m_height), 0.0, 0.0, 1.0);

        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();

        rlActiveDrawBuffers(2);

        rlClearColor(0, 0, 0, 255);
        rlClearScreenBuffers();

        if (IsShaderValid(m_shader))
            BeginShaderMode(m_shader);
    }

    void GBuffer::End()
    {
        if (IsShaderValid(m_shader))
            EndShaderMode();

        rlDrawRenderBatchActive();
        rlDisableFramebuffer();

        // Restore default framebuffer viewport / projection for screen draws
        const int renderWidth = GetRenderWidth();
        const int renderHeight = GetRenderHeight();

        rlViewport(0, 0, renderWidth, renderHeight);
        rlSetFramebufferWidth(renderWidth);
        rlSetFramebufferHeight(renderHeight);

        rlMatrixMode(RL_PROJECTION);
        rlLoadIdentity();
        rlOrtho(0.0, static_cast<double>(renderWidth), static_cast<double>(renderHeight), 0.0, 0.0, 1.0);

        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();
    }

    void GBuffer::Unload()
    {
        if (m_shader.id != 0)
        {
            UnloadShader(m_shader);
            m_shader = {};
        }

        if (m_albedoTexture != 0)
        {
            rlUnloadTexture(m_albedoTexture);
            m_albedoTexture = 0;
        }

        if (m_normalTexture != 0)
        {
            rlUnloadTexture(m_normalTexture);
            m_normalTexture = 0;
        }

        if (m_framebuffer != 0)
        {
            rlUnloadFramebuffer(m_framebuffer);
            m_framebuffer = 0;
        }

        m_width = 0;
        m_height = 0;
    }


    Texture2D GBuffer::GetAlbedoTexture() const
    {
        Texture2D texture{};
        texture.id = m_albedoTexture;
        texture.width = m_width;
        texture.height = m_height;
        texture.mipmaps = 1;
        texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        return texture;
    }

    Texture2D GBuffer::GetNormalTexture() const
    {
        Texture2D texture{};
        texture.id = m_normalTexture;
        texture.width = m_width;
        texture.height = m_height;
        texture.mipmaps = 1;
        texture.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
        return texture;
    }
}
