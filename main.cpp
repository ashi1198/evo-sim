#include "raylib.h"
#include "simulation.h"
#include "terrain.h"
int main()
{
    InitWindow(800, 600, "Evo Sim");
    SetTargetFPS(60);
    unsigned int seed = 12345; // fixed seed for
    Terrain terrain(seed);     // create a terrain with a fixed seed
    terrain.smooth(4);         // 4 passes: more passes = bigger blobs
    Simulation sim;
    const float TIMESTEP = 1.0f / 60.0f; // one simulation tick = 1/60 second
    float accumulator = 0.0f;            // real time not yet turned into ticks
    bool paused = false;

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_SPACE))
        {
            paused = !paused;
        }

        if (!paused)
        {
            accumulator += GetFrameTime(); // add the real time this frame took
            while (accumulator >= TIMESTEP)
            { // enough time for one or more ticks?
                sim.update();
                accumulator -= TIMESTEP; // use up one tick's worth of time
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);
        DrawText(TextFormat("Steps: %d", sim.stepCount), 620, 10, 20, WHITE);
        for (int i = 0; i < 100; i++)
        {
            for (int j = 0; j < 100; j++)
            {
                uint8_t tile = terrain.getTile(j, i);
                Color color;
                if (tile == 0)
                    color = GREEN; // grass
                else if (tile == 1)
                    color = BLUE; // dirt
                else
                    color = GRAY; // water
                DrawRectangle(j * 6, i * 6, 6, 6, color);
            }
        }
        if (paused)
        {
            DrawText(TextFormat("Steps: %d", sim.stepCount), 620, 10, 20, WHITE);
            DrawText("PAUSED", 620, 40, 20, YELLOW);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}