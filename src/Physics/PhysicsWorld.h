#pragma once

#include <raylib.h>
#include <box2d/box2d.h>

#include <vector>

namespace Physics
{
    class PhysicsWorld
    {
    public:
        PhysicsWorld();
        ~PhysicsWorld();

        PhysicsWorld(const PhysicsWorld&) = delete;
        PhysicsWorld& operator=(const PhysicsWorld&) = delete;

        void Step(float deltaTime);

        // position / size are in pixels. size = full width x height (not half-extents).
        b2BodyId CreateDynamicBox(
            Vector2 position,
            Vector2 size
        );

        b2BodyId CreateStaticBox(
            Vector2 position,
            Vector2 size
        );

        Vector2 GetBodyPosition(b2BodyId body) const;

        void SetVelocity(
            b2BodyId body,
            Vector2 velocityPixels
        );

        void DrawDebugColliders() const;

        b2WorldId GetWorld() const;

        static constexpr float PixelsPerMeter = 32.0f;
        static constexpr float DefaultBoxSize = 12.0f;

    private:
        struct DebugBox
        {
            b2BodyId body{};
            Vector2 size{};
        };

        void RegisterDebugBox(b2BodyId body, Vector2 size);

        b2WorldId m_world{};
        std::vector<DebugBox> m_debugBoxes;
    };
}
