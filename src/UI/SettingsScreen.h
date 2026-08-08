#pragma once
#include "ScreenManager.h"


namespace UI
{
    class SettingsScreen : public Screen
    {
    public:
        void Update() override;
        void Draw() override;
    };
}