#include "Application.h"

#include <raylib.h>

namespace App
{
    Application::Application() = default;

    Application::~Application() = default;

    void Application::Run()
    {
        InitWindow(1280, 720, "BEKAS_GAME");
        //SetTargetFPS(60);

        m_settings = std::make_unique<UI::SettingsScreen>();
        m_screenManager = std::make_unique<UI::ScreenManager>(m_settings.get());
        m_menu = std::make_unique<UI::MainMenuScreen>(m_screenManager.get(), m_settings.get());
        m_screenManager->ChangeScreen(m_menu.get());
        m_game = std::make_unique<Game::Game>();

        m_state = State::Menu;

        while (!WindowShouldClose())
        {
            if (m_state == State::Menu && m_menu->StartGameRequested())
                m_state = State::Playing;

            if (m_state == State::Playing)
                m_game->Update();
            else
                m_screenManager->Update();

            BeginDrawing();
            ClearBackground(RAYWHITE);

            if (m_state == State::Playing)
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
