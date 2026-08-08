#pragma once
#ifndef BEKAS_GAME_SRC_UI_SCREEN_H
#define BEKAS_GAME_SRC_UI_SCREEN_H

namespace UI
{
    class Screen
    {
    public:
        virtual ~Screen() = default;

        virtual void Update() = 0;
        virtual void Draw() = 0;
    };
}

#endif // BEKAS_GAME_SRC_UI_SCREEN_H