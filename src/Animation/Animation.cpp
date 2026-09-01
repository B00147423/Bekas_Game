#include "Animation.h"

namespace Animation
{


    Animation::Animation::Animation(
        int firstFrame,
        int lastFrame,
        int framesPerRow,
        int step,
        float frameDuration,
        AnimationType type,
        int frameWidth,
        int frameHeight
    )
    {
        this->firstFrame = firstFrame;
        this->lastFrame = lastFrame;
        this->currentFrame = firstFrame;
        this->framesPerRow = framesPerRow;
        this->step = step;
        this->frameDuration = frameDuration;
        this->timeRemaining = frameDuration;
        this->type = type;
        this->frameWidth = frameWidth;
        this->frameHeight = frameHeight;
    }

    void Animation::updateAnimation(){
        float dt = GetFrameTime();
        timeRemaining -=dt;

        if(timeRemaining <= 0.0f)
        {
            timeRemaining = frameDuration;
            currentFrame += step;
        }

        if (currentFrame > lastFrame)
        {
            switch (type)
            {
            case AnimationType::REPEATING:
                ResetAnimation();
                break;

            case AnimationType::ONESHOT:
                currentFrame = lastFrame;
                break;
            }
        }
    }

    void Animation::ResetAnimation(){
        currentFrame = firstFrame;
        timeRemaining = frameDuration;
    }

    bool Animation::IsFinished() const{
        return type == AnimationType::ONESHOT &&
            currentFrame == lastFrame;
    }

    Rectangle Animation::Animation::GetFrame() const
    {
        int row = 0;

        switch (direction)
        {
            case Direction::Down:      row = 0; break;
            case Direction::DownRight: row = 1; break;
            case Direction::Right:     row = 2; break;
            case Direction::UpRight:   row = 3; break;
            case Direction::Up:        row = 4; break;
            case Direction::UpLeft:    row = 5; break;
            case Direction::Left:      row = 6; break;
            case Direction::DownLeft:  row = 7; break;
        }

        int x = currentFrame * frameWidth;
        int y = row * frameHeight;

        return {
            static_cast<float>(x),
            static_cast<float>(y),
            static_cast<float>(frameWidth),
            static_cast<float>(frameHeight)
        };
    }



    void Animation::SetDirection(Direction direction)
    {
        this->direction = direction;
    }

    Animation::Direction Animation::GetDirection() const
    {
        return direction;
    }

}