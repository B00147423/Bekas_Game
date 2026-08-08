#pragma once

#include "Screen.h"

namespace UI
{
    class ScreenManager
    {
    public:
        ScreenManager(Screen* screen);

        void ChangeScreen(Screen* screen);
        void Update();
        void Draw();

    private:
        Screen* currentScreen;
    };
}