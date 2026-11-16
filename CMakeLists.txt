cmake_minimum_required(VERSION 3.20)

project(BEKAS_GAME)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(BEKAS_GAME
    src/main.cpp
    src/App/Application.cpp
    src/UI/ScreenManager.cpp
    src/UI/SettingsScreen.cpp
    src/UI/MainMenuScreen.cpp
    src/UI/GameOverScreen.cpp
    src/Animation/Animation.cpp
    src/Game/Game.cpp
    src/Game/Player/Player.cpp
    src/World/WaterRenderer.cpp
    src/World/World.cpp
    src/World/WorldRendering.cpp
    src/World/WorldShadows.cpp
    src/World/WorldNavigation.cpp
    src/World/WorldPhysics.cpp
    src/World/WorldFog.cpp
    src/Generation/WorldGenerator.cpp
    src/World/Chunk.cpp
    src/Generation/NoiseGenerator.cpp
    src/Rendering/Tileset.cpp
    src/Lighting/LightingSystem.cpp
    src/Lighting/GBuffer.cpp
    src/Lighting/ShadowBuffer.cpp
    src/Navigation/FlowField.cpp
    src/Physics/Physicsworld.cpp
    src/Game/Camera.cpp
    src/Lighting/ShadowRenderer2D.cpp
)

target_include_directories(BEKAS_GAME PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external/raylib/include
    ${CMAKE_SOURCE_DIR}/external/box2d/include
    ${CMAKE_SOURCE_DIR}/external/nlohmann
    
)

target_link_libraries(BEKAS_GAME PRIVATE
    ${CMAKE_SOURCE_DIR}/external/raylib/lib/libraylib.a
    ${CMAKE_SOURCE_DIR}/external/box2d/lib/libbox2d.a
    winmm
)

# Run from build dir with assets found relative to project root
set_target_properties(BEKAS_GAME PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)
