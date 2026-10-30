#pragma once
#include "terrain.h"
#include "creature.h"
#include <random>
#include <cmath>
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
        for (int tries = 0; tries < 10000 && terrain->getTile(creature.x, creature.y) == 2; tries++)
        {
            creature.x = (creature.x + 1) % 100; // wrap, so it stays 0 to 99
        }
        creatures.push_back(creature);
    }
    float moveCost(uint8_t tile)
    {
        if (tile == 1)
            return 0.5f; // water
        return 0.2f;     // grass
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
                creatures[0].energy -= moveCost(terrain->getTile(newposx, newposy));
            }
            else
            {
                creatures[0].energy -= 0.1f; // bumping into rock costs energy
            }
        }
        else
        {
            creatures[0].energy -= 0.05f; // resting loose energy
        }

        stepCount++;
    }
};