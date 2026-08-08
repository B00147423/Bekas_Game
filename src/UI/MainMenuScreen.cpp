#include "MainMenuScreen.h"

namespace UI
{
    MainMenuScreen::MainMenuScreen(ScreenManager* manager, SettingsScreen* settings)
        : screenManager(manager), settingsScreen(settings)
    {

    m_uiTexture = LoadTexture("assets/FreeFairyTaleUI.png");
    m_backgroundTexture = LoadTexture("assets/menu_background.png");
    m_logoTexture = LoadTexture("assets/logo.png");
    m_menuFont = LoadFont("assets/fonts/menu_font.ttf");
    

    if (m_uiTexture.id == 0)
    {
        TraceLog(LOG_ERROR, "FAILED TO LOAD TEXTURE");
    }
    else
    {
        TraceLog(LOG_INFO, "Texture loaded!");
    }
        playButton     = {40, 500, 200, 50};
        settingsButton = {40, 560, 250, 50};
        quitButton     = {40, 620, 150, 50};
    }

    MainMenuScreen::~MainMenuScreen()
    {
        UnloadTexture(m_uiTexture);
        UnloadTexture(m_backgroundTexture);
        UnloadTexture(m_logoTexture);
        UnloadFont(m_menuFont);
    }

    void MainMenuScreen::Update()
    {
        Vector2 mouse = GetMousePosition();
DrawText(
    TextFormat("Mouse: %.0f, %.0f", mouse.x, mouse.y),
    10, 10, 20, RED
);

            DrawRectangleLinesEx(playButton, 2, RED);
            DrawRectangleLinesEx(settingsButton, 2, GREEN);
            DrawRectangleLinesEx(quitButton, 2, BLUE);
        if (CheckCollisionPointRec(mouse, playButton) &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            startGame = true;
        }

        if (CheckCollisionPointRec(mouse, settingsButton) &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            screenManager->ChangeScreen(settingsScreen);
        }

        if (CheckCollisionPointRec(mouse, quitButton) &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            CloseWindow();
        }
    }

    bool MainMenuScreen::StartGameRequested() const
    {
        return startGame;
    }

    void MainMenuScreen::Draw()
    {
        Vector2 mouse = GetMousePosition();

        bool playHovered = CheckCollisionPointRec(mouse, playButton);
        bool settingsHovered = CheckCollisionPointRec(mouse, settingsButton);
        bool quitHovered = CheckCollisionPointRec(mouse, quitButton);

        DrawTexture(m_backgroundTexture, 0, 0, WHITE);

        DrawTextureEx(
            m_logoTexture,
            {430.0f, 35.0f},
            0.0f,
            1.0f,
            WHITE
        );

        Color playColor = playHovered ? LIME : BLACK;
        Color settingsColor = settingsHovered ? LIME : BLACK;
        Color quitColor = quitHovered ? LIME : BLACK;

        DrawTextEx(
            m_menuFont,
            "Play",
            { playButton.x, playButton.y},
            48,
            2,
            playColor
        );
        DrawTextEx(
            m_menuFont,
            "Settings",
            {settingsButton.x, settingsButton.y},
            48.0f,
            2.0f,
            settingsColor
        );

        DrawTextEx(
            m_menuFont,
            "Quit",
            {quitButton.x, quitButton.y},
            48.0f,
            2.0f,
            quitColor
        );
    }
}