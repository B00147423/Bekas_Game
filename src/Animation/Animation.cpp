#include "Animation.h"

namespace Animation
{


    Animation::Animation::Animation(int firstFrame, int lastFrame, int framesPerRow, int step, float frameDuration, AnimationType type){
        this->firstFrame = firstFrame;
        this->lastFrame = lastFrame;
        this->currentFrame = firstFrame;
        this->framesPerRow = framesPerRow;
        this->step = step;
        this->frameDuration = frameDuration;
        this->timeRemaining = frameDuration;
        this->type = type;
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

    Rectangle Animation::GetFrame() const{
        int x = (currentFrame % framesPerRow) * 96;
        int y = (currentFrame / framesPerRow) * 96;

        if(direction == Direction::Left){
            return Rectangle{
                static_cast<float>(x),
                static_cast<float>(y),
                -96.0f,
                96.0f
            };
        }

        return Rectangle{
            static_cast<float>(x),
            static_cast<float>(y),
            96.0f,
            96.0f
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