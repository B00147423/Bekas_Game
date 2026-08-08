#include "SettingsScreen.h"
#include <raylib.h>

namespace UI
{
    void SettingsScreen::Update()
    {
    }

    void SettingsScreen::Draw()
    {
        DrawText("Settings", 520, 100, 40, BLACK);

        DrawText("Music Volume", 500, 220, 24, BLACK);
        DrawText("SFX Volume", 500, 270, 24, BLACK);

        DrawRectangle(500, 500, 200, 60, DARKGRAY);
        DrawText("Back", 570, 518, 24, WHITE);
    }
}