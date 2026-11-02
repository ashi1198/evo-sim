#include "raylib.h"
#include "simulation.h"
#include "terrain.h"
#include "logger.h"
#include "graph.h"
#include "params.h"
#include <ctime>
#include <string>
#include <filesystem>
#include "creature_art.h"

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

    unsigned int seed = 54321;
    Terrain terrain(seed);
    terrain.smooth(3);

    Params params;
    Simulation sim(terrain, seed, params);

    std::filesystem::create_directories("runs");
    DataLogger logger(makeRunFileName(seed), seed, params);

    const float TIMESTEP = 1.0f / 60.0f;
    float accumulator = 0.0f;
    bool paused = false;
    std::vector<float> popHistory;

    while (!WindowShouldClose())
    {
        if (IsKeyPressed(KEY_SPACE))
            paused = !paused;

        // adjust settings
        if (IsKeyPressed(KEY_ONE))
            params.foodRegrow = std::max(0.0f, params.foodRegrow - 0.0005f);
        if (IsKeyPressed(KEY_TWO))
            params.foodRegrow += 0.0005f;
        if (IsKeyPressed(KEY_THREE))
            params.foodEnergy = std::max(0.0f, params.foodEnergy - 2.0f);
        if (IsKeyPressed(KEY_FOUR))
            params.foodEnergy += 2.0f;
        if (IsKeyPressed(KEY_FIVE))
            params.maxAge = std::max(60, params.maxAge - 60);
        if (IsKeyPressed(KEY_SIX))
            params.maxAge += 60;

        // restart with current settings
        if (IsKeyPressed(KEY_R))
        {
            sim = Simulation(terrain, seed, params);
            popHistory.clear();
            logger = DataLogger(makeRunFileName(seed), seed, params);
            accumulator = 0.0f;
            paused = false;
        }

        if (!paused)
        {
            accumulator += GetFrameTime();
            while (accumulator >= TIMESTEP)
            {
                sim.update();
                if (sim.stepCount % 50 == 0)
                {
                    logger.record(sim.stepCount, sim.creatures, sim.totalFood());
                    popHistory.push_back((float)sim.creatures.size());
                    if (popHistory.size() > 300)
                        popHistory.erase(popHistory.begin());
                }
                accumulator -= TIMESTEP;
            }
        }

        BeginDrawing();
        ClearBackground(BLACK);

        for (int y = 0; y < 100; y++)
        {
            for (int x = 0; x < 100; x++)
            {
                uint8_t tile = terrain.getTile(x, y);
                Color color;
                if (tile == 0)
                    color = GREEN; // grass
                else if (tile == 1)
                    color = BLUE; // water
                else
                    color = GRAY; // rock
                DrawRectangle(x * 6, y * 6, 6, 6, color);
            }
        }

        for (const Creature &c : sim.creatures)
            drawCreature(c, 6); // TILE is your current tile size, 6

        DrawText(TextFormat("Steps: %d", sim.stepCount), 620, 10, 20, WHITE);
        DrawText(TextFormat("Pop: %d", (int)sim.creatures.size()), 620, 40, 20, WHITE);
        if (paused)
            DrawText("PAUSED", 700, 10, 20, YELLOW);

        DrawText("Population", 620, 100, 16, WHITE);
        drawGraph(popHistory, 620, 120, 160, 100, GREEN);
        DrawText(TextFormat("n=%d", (int)popHistory.size()), 620, 230, 16, WHITE);

        DrawText(TextFormat("regrow: %.4f  (1/2)", params.foodRegrow), 620, 260, 16, WHITE);
        DrawText(TextFormat("food energy: %.0f  (3/4)", params.foodEnergy), 620, 280, 16, WHITE);
        DrawText(TextFormat("max age: %d  (5/6)", params.maxAge), 620, 300, 16, WHITE);
        DrawText("R: restart with these settings", 620, 320, 16, WHITE);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}