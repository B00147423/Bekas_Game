#include <raylib.h>

int main()
{
    InitWindow(1280, 720, "BEKAS_GAME");
    SetTargetFPS(60);

    while (!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(RAYWHITE);

        DrawText("Hello, raylib!", 20, 20, 30, BLACK);

        EndDrawing();
    }

    CloseWindow();

    return 0;
}