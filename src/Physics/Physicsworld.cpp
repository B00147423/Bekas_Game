#include "PhysicsWorld.h"

namespace Physics
{
    namespace
    {
        constexpr Color kDebugFill = { 100, 180, 255, 80 };
        constexpr Color kDebugOutline = { 100, 180, 255, 200 };
    }

    PhysicsWorld::PhysicsWorld()
    {
        b2WorldDef worldDef = b2DefaultWorldDef();

        worldDef.gravity = {
            0.0f,
            0.0f
        };

        m_world = b2CreateWorld(&worldDef);
    }

    PhysicsWorld::~PhysicsWorld()
    {
        b2DestroyWorld(m_world);
    }

    void PhysicsWorld::Step(float deltaTime)
    {
        b2World_Step(
            m_world,
            deltaTime,
            4
        );
    }

    void PhysicsWorld::RegisterDebugBox(b2BodyId body, Vector2 size)
    {
        m_debugBoxes.push_back({ body, size });
    }

    b2BodyId PhysicsWorld::CreateDynamicBox(
        Vector2 position,
        Vector2 size
    )
    {
        b2BodyDef bodyDef = b2DefaultBodyDef();

        bodyDef.type = b2_dynamicBody;

        bodyDef.position = {
            position.x / PixelsPerMeter,
            position.y / PixelsPerMeter
        };

        bodyDef.motionLocks.angularZ = true;
        bodyDef.linearDamping = 0.0f;

        b2BodyId body = b2CreateBody(m_world, &bodyDef);

        // b2MakeBox wants half-width / half-height in meters
        const b2Polygon shape = b2MakeBox(
            (size.x * 0.5f) / PixelsPerMeter,
            (size.y * 0.5f) / PixelsPerMeter
        );

        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;

        b2CreatePolygonShape(
            body,
            &shapeDef,
            &shape
        );

        b2Body_ApplyMassFromShapes(body);

        RegisterDebugBox(body, size);

        return body;
    }

    b2BodyId PhysicsWorld::CreateStaticBox(
        Vector2 position,
        Vector2 size
    )
    {
        b2BodyDef bodyDef = b2DefaultBodyDef();

        bodyDef.type = b2_staticBody;

        bodyDef.position = {
            position.x / PixelsPerMeter,
            position.y / PixelsPerMeter
        };

        b2BodyId body =
            b2CreateBody(m_world, &bodyDef);

        const b2Polygon shape = b2MakeBox(
            (size.x * 0.5f) / PixelsPerMeter,
            (size.y * 0.5f) / PixelsPerMeter
        );

        b2ShapeDef shapeDef = b2DefaultShapeDef();

        b2CreatePolygonShape(
            body,
            &shapeDef,
            &shape
        );

        RegisterDebugBox(body, size);

        return body;
    }

    Vector2 PhysicsWorld::GetBodyPosition(
        b2BodyId body
    ) const
    {
        const b2Vec2 position = b2Body_GetPosition(body);

        return {
            position.x * PixelsPerMeter,
            position.y * PixelsPerMeter
        };
    }

    void PhysicsWorld::SetVelocity(b2BodyId body,Vector2 velocityPixels )
    {
        b2Body_SetLinearVelocity(
            body,
            {
                velocityPixels.x / PixelsPerMeter,
                velocityPixels.y / PixelsPerMeter
            }
        );
    }

    void PhysicsWorld::DrawDebugColliders() const
    {
        for (const DebugBox& box : m_debugBoxes){
            if (!b2Body_IsValid(box.body))
                continue;

            const Vector2 center = GetBodyPosition(box.body);
            const Rectangle rect = {
                center.x - box.size.x * 0.5f,
                center.y - box.size.y * 0.5f,
                box.size.x,
                box.size.y
            };

            DrawRectangleRec(rect, kDebugFill);
            DrawRectangleLinesEx(rect, 1.0f, kDebugOutline);
        }
    }

    b2WorldId PhysicsWorld::GetWorld() const
    {
        return m_world;
    }
}
