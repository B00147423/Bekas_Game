#include "Application.h"

#include <raylib.h>

namespace App
{
    Application::Application() = default;

    Application::~Application() = default;

    void Application::Run()
    {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
        InitWindow(1280, 720, "BEKAS_GAME");
        SetWindowMinSize(800,600);
        SetExitKey(KEY_NULL);

        m_settings = std::make_unique<UI::SettingsScreen>();
        m_screenManager = std::make_unique<UI::ScreenManager>(m_settings.get());
        m_menu = std::make_unique<UI::MainMenuScreen>(m_screenManager.get(), m_settings.get());
        m_screenManager->ChangeScreen(m_menu.get());

        m_state = State::Menu;

        unsigned int appliedRevision=0;
        Lighting::GraphicsSettings applied{};
        bool settingsOpen=false;
        while (!WindowShouldClose())
        {
            if (m_state == State::Menu && m_menu->StartGameRequested()) {
                BeginDrawing();ClearBackground(RAYWHITE);
                DrawText("Preparing world...",40,40,24,DARKGRAY);EndDrawing();
                m_game = std::make_unique<Game::Game>();
                const auto& graphics=m_settings->GetGraphics();
                if(m_game) m_game->SetShadowSettings(graphics.softness,graphics.shadows?graphics.opacity:0,graphics.samples);
                if(m_game) m_game->SetShadowResolutionScale(graphics.shadowScale);
                if(m_game) m_game->SetShadowBackend(graphics.gpuShadows);
                m_state = State::Playing;
            }

            if(IsKeyPressed(KEY_F10) && m_state==State::Playing) settingsOpen=!settingsOpen;
            if(IsKeyPressed(KEY_ESCAPE) && !settingsOpen &&
                !(m_state==State::Menu && m_screenManager->Current()==m_settings.get())) break;
            if(settingsOpen) m_settings->Update();
            else if (m_state == State::Playing)
                m_game->Update();
            else
                m_screenManager->Update();

            if(m_settings->TakeBackRequest()) {
                settingsOpen=false;
                if(m_state==State::Menu) m_screenManager->ChangeScreen(m_menu.get());
            }
            if(m_menu->QuitRequested()) break;
            const auto& graphics=m_settings->GetGraphics();
            if(appliedRevision!=m_settings->Revision()) {
                const bool changeDisplay=appliedRevision==0 || graphics.windowMode!=applied.windowMode ||
                    graphics.width!=applied.width || graphics.height!=applied.height;
                if(changeDisplay) {
                    if(IsWindowFullscreen()) ToggleFullscreen();
                    if(IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) ToggleBorderlessWindowed();
                    SetWindowSize(graphics.width,graphics.height);
                    if(graphics.windowMode==1) ToggleBorderlessWindowed();
                    if(graphics.windowMode==2) ToggleFullscreen();
                }
                if(graphics.vsync) SetWindowState(FLAG_VSYNC_HINT);
                else ClearWindowState(FLAG_VSYNC_HINT);
                SetTargetFPS(graphics.fpsLimit);
                if(m_game) m_game->SetShadowSettings(graphics.softness,graphics.shadows?graphics.opacity:0,graphics.samples);
                if(m_game) m_game->SetShadowResolutionScale(graphics.shadowScale);
                if(m_game) m_game->SetShadowBackend(graphics.gpuShadows);
                applied=graphics;appliedRevision=m_settings->Revision();
            }
            if(IsWindowMinimized()) { PollInputEvents(); WaitTime(0.01); continue; }

            BeginDrawing();
            ClearBackground(RAYWHITE);

            if(settingsOpen) m_settings->Draw();
            else if (m_state == State::Playing)
                m_game->Draw();
            else
                m_screenManager->Draw();

            EndDrawing();
        }

        // Unload textures while the window/GL context still exists
        m_game.reset();
        m_menu.reset();
        m_screenManager.reset();
        m_settings.reset();

        CloseWindow();
    }
}
