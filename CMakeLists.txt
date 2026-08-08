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
    src/World/World.cpp
    src/World/Generation/WorldGenerator.cpp
    src/World/Generation/HeightMapGenerator.cpp
    src/World/Generation/TerrainGenerator.cpp
    src/World/Chunk.cpp
    src/World/Generation/NoiseGenerator.cpp
    src/World/Tileset.cpp
    src/Lighting/GBuffer.cpp
    src/Lighting/OcclusionBuffer.cpp
    src/Lighting/ShadowBuffer.cpp
    src/World/FlowField.cpp
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
