cmake_minimum_required(VERSION 3.20)

project(BEKAS_GAME)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(BEKAS_GAME
    src/main.cpp
)

target_include_directories(BEKAS_GAME PRIVATE
    ${CMAKE_SOURCE_DIR}/external/raylib/include
    ${CMAKE_SOURCE_DIR}/external/box2d/include
    ${CMAKE_SOURCE_DIR}/external/nlohmann
)

target_link_libraries(BEKAS_GAME PRIVATE
    ${CMAKE_SOURCE_DIR}/external/raylib/lib/libraylib.a
    ${CMAKE_SOURCE_DIR}/external/box2d/lib/libbox2d.a
    winmm
)