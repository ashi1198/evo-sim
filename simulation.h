#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <cstdint>
#include "terrain.h"
#include "creature.h"

const float FOOD_ENERGY = 20.0f; // energy per unit of food eaten
const float MAX_ENERGY = 100.0f; // energy can't grow past this

class Simulation
{
public:
    int stepCount = 0;
    Terrain *terrain;
    std::mt19937 rng;
    std::vector<Creature> creatures;
    std::vector<float> food;      // food amount per tile, 0 to 1
    std::vector<bool> isFoodSpot; // true only for tiles that spawned food

    Simulation(Terrain &t, unsigned int seed)
    {
        terrain = &t;
        rng.seed(seed);
        food.assign(100 * 100, 0.0f);
        isFoodSpot.assign(100 * 100, false);

        // spawn food on grass, one roll per tile so the order stays fixed
        std::uniform_real_distribution<float> roll_food(0.0f, 1.0f);
        for (int y = 0; y < 100; y++)
        {
            for (int x = 0; x < 100; x++)
            {
                float roll = roll_food(rng);
                if (terrain->getTile(x, y) == 0 && roll < 0.3f)
                {
                    int i = terrain->index(x, y);
                    food[i] = 1.0f;
                    isFoodSpot[i] = true;
                }
            }
        }

        // place the creature, moving it off rock if needed
        Creature creature;
        creature.x = 50;
        creature.y = 50;
        for (int tries = 0; tries < 10000 && terrain->getTile(creature.x, creature.y) == 2; tries++)
        {
            creature.x = (creature.x + 1) % 100;
        }
        creatures.push_back(creature);
    }

    float moveCost(uint8_t tile) const
    {
        if (tile == 1)
            return 0.5f; // water
        return 0.2f;     // grass
    }

    // food on spots that spawned food slowly grows back, even from 0
    void regrowFood()
    {
        for (int i = 0; i < 100 * 100; i++)
        {
            if (isFoodSpot[i])
            {
                food[i] = std::min(1.0f, food[i] + 0.002f);
            }
        }
    }

    // eat from the tile the creature is standing on
    void eat(Creature &c)
    {
        int i = terrain->index(c.x, c.y);
        if (food[i] <= 0.0f)
            return;
        float eaten = std::min(0.5f, food[i]);
        food[i] -= eaten;
        c.energy = std::min(MAX_ENERGY, c.energy + eaten * FOOD_ENERGY);
    }

    void update()
    {
        regrowFood();

        std::uniform_real_distribution<float> roll_move_chance(0.0f, 1.0f);
        std::uniform_int_distribution<int> roll_move(-1, 1);
        Creature &c = creatures[0];

        if (roll_move_chance(rng) < c.moveChance)
        {
            int dx = roll_move(rng); // first random call
            int dy = roll_move(rng);
            int newX = c.x + dx;
            int newY = c.y + dy;
            int tile = terrain->getTile(newX, newY);

            if (tile != 2) // not rock
            {
                c.x = (newX % 100 + 100) % 100;
                c.y = (newY % 100 + 100) % 100;
                c.energy -= moveCost(tile);
            }
            else
            {
                c.energy -= 0.1f; // bumping into rock
            }
        }
        else
        {
            c.energy -= 0.05f; // resting
        }

        eat(c);
        stepCount++;
    }
};