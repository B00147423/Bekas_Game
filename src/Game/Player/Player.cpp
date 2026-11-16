#include "Player.h"
#include "World/World.h"
#include <cmath>

namespace Player {
Player::Player(Physics::PhysicsWorld& physicsWorld, World::World& world)
    : m_physicsWorld(physicsWorld), m_world(world),
        m_idleAnimation(
        GameConfig::Player::IdleFirstFrame,
        GameConfig::Player::IdleLastFrame,
        GameConfig::Player::FramesPerRow,
        1,
        GameConfig::Player::IdleFrameDuration,
        Animation::Animation::AnimationType::REPEATING,
        GameConfig::Player::FrameWidth,
        GameConfig::Player::FrameHeight
    ),
    
    m_runAnimation(
        GameConfig::Player::WalkFirstFrame,
        GameConfig::Player::WalkLastFrame,
        GameConfig::Player::FramesPerRow,
        1,
        GameConfig::Player::WalkFrameDuration,
        Animation::Animation::AnimationType::REPEATING,
        GameConfig::Player::FrameWidth,
        GameConfig::Player::FrameHeight
    )
{}

void Player::Init() {
    if (!m_idleTexture.id) m_idleTexture = LoadTexture(GameConfig::Player::TexturePath);
    if (!m_runTexture.id) m_runTexture = LoadTexture(GameConfig::Player::TexturePath);
    if (!b2Body_IsValid(m_playerBody)) CreatePlayerBody();
}

Player::~Player() {
        if (m_idleTexture.id != 0)UnloadTexture(m_idleTexture);
        if (m_runTexture.id != 0)UnloadTexture(m_runTexture);
    m_physicsWorld.DestroyBody(m_playerBody);
}

void Player::Update(bool clicked, Vector2 mouseWorld) {
        if (clicked)
        {

            m_hasTreeTarget = false;

            if (m_world.IsTreeAt(mouseWorld))
            {
                Vector2 approachPoint{};

                if (m_world.FindTreeApproachPoint(
                        mouseWorld,
                        GetPlayerFeetPosition(),
                        approachPoint))
                {
                    m_treeTarget = mouseWorld;
                    m_hasTreeTarget = true;

                    m_world.SetFlowFieldTarget(approachPoint);
                }
            }
            else
            {
                m_world.SetFlowFieldTarget(mouseWorld);
            }
        }

        PlayerInput();
        if (m_hasTreeTarget)
        {
            const int treeX = static_cast<int>(
                std::floor(m_treeTarget.x / World::World::TileSize)
            );

            const int treeY = static_cast<int>(
                std::floor(m_treeTarget.y / World::World::TileSize)
            );

            Vector2 treeCenter = {
                (treeX + 0.5f) * World::World::TileSize,
                (treeY + 0.5f) * World::World::TileSize
            };

            Vector2 playerFeet = GetPlayerFeetPosition();

            float dx = treeCenter.x - playerFeet.x;
            float dy = treeCenter.y - playerFeet.y;

            if (dx * dx + dy * dy <= m_chopRange * m_chopRange)
            {
                if (m_world.RemoveTree(m_treeTarget))
                {
                    TraceLog(LOG_INFO, "Tree chopped!");
                }

                m_hasTreeTarget = false;

                // Stop moving toward the old destination.
                m_world.SetFlowFieldTarget(playerFeet);
                m_physicsWorld.SetVelocity(m_playerBody, {0.0f, 0.0f});
            }
        }

}

void Player::CreatePlayerBody()
    {
        Vector2 playerFeet = {
            m_position.x + GameConfig::Player::BodyOffset.x,
            m_position.y + GameConfig::Player::BodyOffset.y
        };

        m_playerBody = m_physicsWorld.CreateDynamicBox(
            playerFeet,
            GameConfig::Player::ColliderSize
        );
    }

void Player::SyncPlayerFromPhysics()
    {
        Vector2 pos = m_physicsWorld.GetBodyPosition(m_playerBody);

        m_position.x = pos.x - GameConfig::Player::BodyOffset.x;
        m_position.y = pos.y - GameConfig::Player::BodyOffset.y;
    }

void Player::PlayerInput()
    {
        Vector2 playerFeet = {
            m_position.x + GameConfig::Player::BodyOffset.x,
            m_position.y + GameConfig::Player::BodyOffset.y
        };

        const float deltaTime = GetFrameTime();
        Vector2 direction = m_world.GetFlowDirection(playerFeet, m_playerSpeed * deltaTime);

        m_playerMoving =
            direction.x != 0.0f ||
            direction.y != 0.0f;

        if (m_playerMoving)
        {
            if (direction.x < 0.0f && direction.y < 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::UpLeft);
            else if (direction.x > 0.0f && direction.y < 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::UpRight);
            else if (direction.x < 0.0f && direction.y > 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::DownLeft);
            else if (direction.x > 0.0f && direction.y > 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::DownRight);
            else if (direction.x < 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::Left);
            else if (direction.x > 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::Right);
            else if (direction.y < 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::Up);
            else if (direction.y > 0.0f)
                m_runAnimation.SetDirection(Animation::Animation::Direction::Down);

            // Keep idle facing the same way for when movement stops
            m_idleAnimation.SetDirection(m_runAnimation.GetDirection());
            m_runAnimation.updateAnimation();
        }
        else
        {
            m_idleAnimation.updateAnimation();
        }

        Vector2 velocity = {
            direction.x * m_playerSpeed,
            direction.y * m_playerSpeed
        };

        const Vector2 before = playerFeet;

        m_physicsWorld.SetVelocity(m_playerBody, velocity);
        m_physicsWorld.Step(deltaTime);
        SyncPlayerFromPhysics();

        // If flow wants to move but physics barely moved, slide around the blocker
        if (m_playerMoving)
        {
            const Vector2 after = {
                m_position.x + GameConfig::Player::BodyOffset.x,
                m_position.y + GameConfig::Player::BodyOffset.y
            };
            const float dx = after.x - before.x;
            const float dy = after.y - before.y;
            const float movedSq = dx * dx + dy * dy;

            // Tiny final steps are intentional, not a reason to slide away from the click.
            if (movedSq < 0.25f && !m_world.IsInFlowTargetTile(before))
            {
                const Vector2 slide = m_world.GetSlideDirection(before, direction);
                if (slide.x != 0.0f || slide.y != 0.0f)
                {
                    m_physicsWorld.SetVelocity(
                        m_playerBody,
                        {
                            slide.x * m_playerSpeed,
                            slide.y * m_playerSpeed
                        }
                    );
                    m_physicsWorld.Step(GetFrameTime());
                    SyncPlayerFromPhysics();
                }
            }
        }
    }

void Player::DrawPlayer(Color tint)
{
        if (m_playerMoving)
        {
            DrawTexturePro(
                m_runTexture,
                m_runAnimation.GetFrame(),
                {
                    m_position.x,
                    m_position.y,
                    GameConfig::Player::SpriteSize.x,
                    GameConfig::Player::SpriteSize.y
                },
                { 0.0f, 0.0f },
                0.0f,
                tint
            );
        }
        else
        {
            DrawTexturePro(
                m_idleTexture,
                m_idleAnimation.GetFrame(),
                {
                    m_position.x,
                    m_position.y,
                    GameConfig::Player::SpriteSize.x,
                    GameConfig::Player::SpriteSize.y
                },
                { 0.0f, 0.0f },
                0.0f,
                tint
            );
        }
    }

Vector2 Player::GetPlayerFeetPosition() const
    {
        return {
            m_position.x + GameConfig::Player::BodyOffset.x,
            m_position.y + GameConfig::Player::BodyOffset.y
        };
    }

}
