#include "ScreenManager.h"

namespace UI
{
    ScreenManager::ScreenManager(Screen* screen)
        : currentScreen(screen)
    {
    }

    void ScreenManager::ChangeScreen(Screen* screen)
    {
        currentScreen = screen;
    }

    void ScreenManager::Update()
    {
        currentScreen->Update();
    }

    void ScreenManager::Draw()
    {
        currentScreen->Draw();
    }
}