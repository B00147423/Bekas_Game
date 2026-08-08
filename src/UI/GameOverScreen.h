#pragma once
#include "Screen.h"

namespace UI
{
    class GameOverScreen : public Screen
    {
    public:
        virtual void Update() override;
        virtual void Draw() override;
    };
}