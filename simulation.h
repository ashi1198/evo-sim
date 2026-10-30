#pragma once
#include "terrain.h"
#include "creature.h"
#include <random>
class Simulation
{
public:
    int stepCount = 0;
    Terrain *terrain;
    std::mt19937 rng;
    std::vector<Creature> creatures;
    Simulation(Terrain &t, unsigned int seed)
    {
        terrain = &t;
        rng.seed(seed);
        Creature creature;
        creature.x = 50;
        creature.y = 50;
        creatures.push_back(creature);
    }
    void update()
    {
        // moving player
        std::uniform_real_distribution<float> roll_move_chances(0.0f, 1.0f);
        float r = roll_move_chances(rng);
        if (r < creatures[0].moveChance)
        {

            std::uniform_int_distribution<int> roll_move(-1, 1);
            int dx = roll_move(rng);
            int dy = roll_move(rng);
            int newposx = creatures[0].x + dx;
            int newposy = creatures[0].y + dy;
            if (terrain->getTile(newposx, newposy) != 2)
            { // not rock
                creatures[0].x = (newposx % 100 + 100) % 100;
                creatures[0].y = (newposy % 100 + 100) % 100;
                creatures[0].energy -= 0.2f; // moving costs energy
            }
        }
        else
        {
            creatures[0].energy -= 0.05f; // resting gains energy
        }

        stepCount++;
    }
};