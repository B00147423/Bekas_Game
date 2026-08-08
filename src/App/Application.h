#pragma once

#include "../UI/ScreenManager.h"
#include "../UI/SettingsScreen.h"
#include "../UI/MainMenuScreen.h"
#include "../Game/Game.h"

#include <memory>

namespace App
{
    class Application
    {
    public:
        enum class State
        {
            Menu,
            Playing
        };

        Application();
        ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

        void Run();

    private:
        State m_state = State::Menu;

        // Created after InitWindow(); destroyed before CloseWindow()
        std::unique_ptr<UI::SettingsScreen> m_settings;
        std::unique_ptr<UI::ScreenManager> m_screenManager;
        std::unique_ptr<UI::MainMenuScreen> m_menu;
        std::unique_ptr<Game::Game> m_game;
    };
}
