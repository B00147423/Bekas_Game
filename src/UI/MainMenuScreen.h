#pragma once

#include "Screen.h"
#include "ScreenManager.h"
#include "SettingsScreen.h"
#include <raylib.h>

namespace UI
{
    class MainMenuScreen : public Screen
    {
    public:
        MainMenuScreen(ScreenManager* manager, SettingsScreen* settings);
        ~MainMenuScreen();

        void Update() override;
        void Draw() override;


        bool StartGameRequested() const;

    private:
        ScreenManager* screenManager;
        SettingsScreen* settingsScreen;

        Texture2D m_uiTexture;
        Rectangle playButton;
        Rectangle settingsButton;
        Rectangle quitButton;

        Texture2D m_backgroundTexture{};
        Texture2D m_logoTexture{};
        Font m_menuFont{};

        Vector2 m_position{};

        Vector2 menuPosition = { 40, 500 };

        const float spacing = 60.0f;
        bool startGame = false;
    };
}