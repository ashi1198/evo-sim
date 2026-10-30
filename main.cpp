#include "raylib.h"
#include "simulation.h"
#include "terrain.h"
#include "logger.h"
#include "graph.h"
#include <ctime>
#include <string>
#include <filesystem>
std::string makeRunFileName(unsigned int seed)
{
    std::time_t now = std::time(nullptr);
    char stamp[32];
    std::strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", std::localtime(&now));
    return "runs/run_" + std::to_string(seed) + "_" + stamp + ".csv";
}
int main()
{
    InitWindow(800, 600, "Evo Sim");
    SetTargetFPS(60);
    unsigned int seed = 12345;           // fixed seed for
    Terrain terrain(seed);               // create a terrain with a fixed seed
    terrain.smooth(3);                   // 4 passes: more passes = bigger blobs
    Simulation sim(terrain, seed);       // create a simulation with the same seed
    const float TIMESTEP = 1.0f / 60.0f; // one simulation tick = 1/60 second
    float accumulator = 0.0f;            // real time not yet turned into ticks
    bool paused = false;
    std::filesystem::create_directories("runs"); // makes the folder if it's missing
    DataLogger logger(makeRunFileName(seed), seed);
    bool finished = false;
    std::vector<float> popHistory; // population sampled every 10 ticks

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
            {
                sim.update();
                if (sim.stepCount % 10 == 0)
                {
                    logger.record(sim.stepCount, sim.creatures, sim.totalFood());
                    popHistory.push_back((float)sim.creatures.size());
                    if (popHistory.size() > 300)
                        popHistory.erase(popHistory.begin()); // keep only the newest 300 samples
                }
                accumulator -= TIMESTEP;
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);
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
        for (const Creature &c : sim.creatures)
        {
            DrawCircle(c.x * 6 + 3, c.y * 6 + 3, 3, RED);
        }
        DrawText(TextFormat("Steps: %d", sim.stepCount), 620, 10, 20, WHITE);
        DrawText(TextFormat("Pop: %d", (int)sim.creatures.size()), 620, 40, 20, WHITE);
        DrawText("Population", 620, 100, 16, WHITE);
        drawGraph(popHistory, 620, 120, 160, 100, GREEN);
        DrawText(TextFormat("n=%d", (int)popHistory.size()), 620, 230, 16, WHITE);
        if (paused)
        {

            DrawText("PAUSED", 620, 40, 20, YELLOW);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}