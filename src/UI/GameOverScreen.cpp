#include "GameOverScreen.h"
#include <raylib.h>

namespace UI
{
    void GameOverScreen::Update()
    {
    }

    void GameOverScreen::Draw()
    {
        DrawText("Main Menu", 20, 20, 30, BLACK);
    }
}