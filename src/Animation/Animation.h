#pragma once
#include "raylib.h"


namespace Animation{
    class Animation{

    public:
    
        enum class Direction
        {
            Up,
            Down,
            Left,
            Right,

            UpLeft,
            UpRight,
            DownLeft,
            DownRight
        };
        enum class AnimationType {
            REPEATING = 1,
            ONESHOT = 2,
        };

        Animation(
            int firstFrame,
            int lastFrame,
            int framesPerRow,
            int step,
            float frameDuration,
            AnimationType type,
            int frameWidth,
            int frameHeight
        );

  
        void updateAnimation();
        Rectangle GetFrame() const;
        void ResetAnimation();
        bool IsFinished() const;

        void SetDirection(Direction direction);
        Direction GetDirection() const;
    private:

        int firstFrame;
        int lastFrame;
        int currentFrame;
        int framesPerRow;
        int step;
        float frameDuration;
        float timeRemaining;
        int frameWidth;
        int frameHeight;
        Direction direction = Direction::Down;

        
        AnimationType type;
    };
}