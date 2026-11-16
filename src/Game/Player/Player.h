#pragma once
#include "GameConfig.h"
#include "Animation/Animation.h"
#include "Physics/PhysicsWorld.h"
namespace World { class World; }

namespace Player {
// Borrows the existing simulation and world; both must outlive this player.
class Player {
public:
    Player(Physics::PhysicsWorld& physicsWorld, World::World& world);
    ~Player();
    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
    void Init();
    void Update(bool clicked, Vector2 mouseWorld);
    void DrawPlayer(Color tint);
    Vector2 GetPlayerFeetPosition() const;
    Vector2 GetPosition() const { return m_position; }
    Texture2D GetTexture() const { return m_playerMoving ? m_runTexture : m_idleTexture; }
    Rectangle GetFrame() const { return m_playerMoving ? m_runAnimation.GetFrame() : m_idleAnimation.GetFrame(); }
    Rectangle GetBounds() const { return {m_position.x, m_position.y, GameConfig::Player::SpriteSize.x, GameConfig::Player::SpriteSize.y}; }
private:
    void CreatePlayerBody();
    void SyncPlayerFromPhysics();
    void PlayerInput();
    Physics::PhysicsWorld& m_physicsWorld;
    World::World& m_world;
    Animation::Animation m_idleAnimation;
    Animation::Animation m_runAnimation;
    Texture2D m_idleTexture{};
    Texture2D m_runTexture{};
    bool m_playerMoving = false;
    b2BodyId m_playerBody{};
    Vector2 m_position = GameConfig::Player::SpawnPosition;
    float m_playerSpeed = GameConfig::Player::Speed;
    bool m_hasTreeTarget = false;
    Vector2 m_treeTarget{};
    float m_chopRange = 24.0f;
};
}
