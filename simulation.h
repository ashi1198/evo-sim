#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <cstdint>
#include "terrain.h"
#include "creature.h"

const float FOOD_ENERGY = 20.0f;
const float MAX_ENERGY = 100.0f;
const int START_CREATURES = 50;

class Simulation
{
public:
    int stepCount = 0;
    Terrain *terrain;
    std::mt19937 rng;
    std::vector<Creature> creatures;
    std::vector<float> food;
    std::vector<bool> isFoodSpot;

    Simulation(Terrain &t, unsigned int seed)
    {
        terrain = &t;
        rng.seed(seed);
        food.assign(100 * 100, 0.0f);
        isFoodSpot.assign(100 * 100, false);

        // spawn food on grass, one roll per tile
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

        // spawn the starting creatures on non-rock tiles
        std::uniform_int_distribution<int> pos(0, 99);
        std::uniform_real_distribution<float> chance(0.0f, 1.0f);
        for (int n = 0; n < START_CREATURES; n++)
        {
            Creature c;
            int x = pos(rng);
            int y = pos(rng);
            int tries = 0;
            while (terrain->getTile(x, y) == 2 && tries < 10000)
            {
                x = pos(rng);
                y = pos(rng);
                tries++;
            }
            c.x = x;
            c.y = y;
            c.moveChance = chance(rng);
            c.sex = (chance(rng) < 0.5f) ? 'f' : 'm';
            creatures.push_back(c);
        }
    }

    float moveCost(uint8_t tile) const
    {
        if (tile == 1)
            return 0.5f; // water
        return 0.2f;     // grass
    }

    void regrowFood()
    {
        for (int i = 0; i < 100 * 100; i++)
        {
            if (isFoodSpot[i])
                food[i] = std::min(1.0f, food[i] + 0.002f);
        }
    }
    float totalFood() const
    {
        float sum = 0.0f;
        for (float f : food)
            sum += f;
        return sum;
    }
    void eat(Creature &c)
    {
        int i = terrain->index(c.x, c.y);
        if (food[i] <= 0.0f)
            return;
        float eaten = std::min(0.5f, food[i]);
        food[i] -= eaten;
        c.energy = std::min(MAX_ENERGY, c.energy + eaten * FOOD_ENERGY);
    }

    void moveCreature(Creature &c)
    {
        std::uniform_real_distribution<float> chance(0.0f, 1.0f);
        std::uniform_int_distribution<int> dir(-1, 1);

        if (chance(rng) < c.moveChance)
        {
            // separate statements keep the random order fixed
            int dx = dir(rng);
            int dy = dir(rng);
            int newX = c.x + dx;
            int newY = c.y + dy;
            int tile = terrain->getTile(newX, newY);
            if (tile != 2)
            {
                c.x = (newX % 100 + 100) % 100;
                c.y = (newY % 100 + 100) % 100;
                c.energy -= moveCost(tile);
            }
            else
            {
                c.energy -= 0.1f; // bumped into rock
            }
        }
        else
        {
            c.energy -= 0.05f; // resting
        }
    }

    // one trait: average or pick one parent, then add a small mutation
    float inheritTrait(float a, float b, float lo, float hi, float mutRate)
    {
        std::uniform_real_distribution<float> coin(0.0f, 1.0f);
        float choice = coin(rng); // decides average or pick
        float pick = coin(rng);   // used only if picking one parent
        float base;
        if (choice < 0.5f)
            base = (a + b) / 2.0f;
        else
            base = (pick < 0.5f) ? a : b;

        std::uniform_real_distribution<float> mut(-1.0f, 1.0f);
        float m = mut(rng);
        float value = base + m * mutRate * (hi - lo);
        return std::clamp(value, lo, hi);
    }

    void reproduce()
    {
        // collect ready males first
        std::vector<int> males;
        for (int i = 0; i < (int)creatures.size(); i++)
        {
            if (creatures[i].sex == 'm' && creatures[i].energy >= creatures[i].reproThreshold)
                males.push_back(i);
        }

        std::vector<Creature> newborns;
        std::uniform_real_distribution<float> coin(0.0f, 1.0f);

        for (int i = 0; i < (int)creatures.size(); i++)
        {
            Creature &f = creatures[i];
            if (f.sex != 'f' || f.energy < f.reproThreshold || males.empty())
                continue;

            // pick a random ready male, and remove him so he mates once per tick
            std::uniform_int_distribution<int> pick(0, (int)males.size() - 1);
            int k = pick(rng);
            Creature &m = creatures[males[k]];
            males.erase(males.begin() + k);

            // both parents give 25% of their energy
            float fGive = f.energy * 0.25f;
            float mGive = m.energy * 0.25f;
            f.energy -= fGive;
            m.energy -= mGive;

            Creature child;
            child.x = f.x;
            child.y = f.y;
            child.energy = fGive + mGive;
            child.sex = (coin(rng) < 0.5f) ? 'f' : 'm';

            // inherit the mutation rate first, since other traits use it
            child.mutationRate = inheritTrait(f.mutationRate, m.mutationRate, 0.001f, 0.5f, 0.01f);
            child.moveChance = inheritTrait(f.moveChance, m.moveChance, 0.0f, 1.0f, child.mutationRate);
            child.reproThreshold = inheritTrait(f.reproThreshold, m.reproThreshold, 20.0f, 100.0f, child.mutationRate);

            newborns.push_back(child);
        }

        creatures.insert(creatures.end(), newborns.begin(), newborns.end());
    }

    void update()
    {
        regrowFood();

        for (Creature &c : creatures)
        {
            moveCreature(c);
            eat(c);
        }

        reproduce();
        stepCount++;
    }
};