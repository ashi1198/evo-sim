#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include "terrain.h"
#include "creature.h"
#include "params.h"

const float MAX_ENERGY = 100.0f;
const int START_CREATURES = 50;
const int MATE_RANGE = 2; // tiles: how close a male must be to mate

class Simulation
{
public:
    Params p;
    int stepCount = 0;
    Terrain *terrain;
    std::mt19937 rng;
    std::vector<Creature> creatures;
    std::vector<float> food;
    std::vector<bool> isFoodSpot;

    Simulation(Terrain &t, unsigned int seed, Params params)
    {
        terrain = &t;
        p = params;
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
            c.reproThreshold = p.reproThreshold;
            c.sex = (chance(rng) < 0.5f) ? 'f' : 'm';
            creatures.push_back(c);
        }
    }

    float moveCost(uint8_t tile) const
    {
        if (tile == 1)
            return p.moveCostWater; // water
        return p.moveCostGrass;     // grass
    }

    void regrowFood()
    {
        for (int i = 0; i < 100 * 100; i++)
        {
            if (isFoodSpot[i])
                food[i] = std::min(1.0f, food[i] + p.foodRegrow);
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
        c.energy = std::min(MAX_ENERGY, c.energy + eaten * p.foodEnergy);
    }

    void moveCreature(Creature &c)
    {
        std::uniform_real_distribution<float> chance(0.0f, 1.0f);
        std::uniform_int_distribution<int> dir(-1, 1);

        if (chance(rng) < c.moveChance)
        {
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

    int wrapDist(int a, int b) const
    {
        int d = std::abs(a - b);
        return std::min(d, 100 - d);
    }

    // one trait: average or pick one parent, then add a small mutation
    float inheritTrait(float a, float b, float lo, float hi, float mutRate)
    {
        std::uniform_real_distribution<float> coin(0.0f, 1.0f);
        float choice = coin(rng);
        float pick = coin(rng);
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
            if (f.sex != 'f' || f.energy < f.reproThreshold)
                continue;

            int best = -1;
            int bestDist = MATE_RANGE + 1;
            for (int k = 0; k < (int)males.size(); k++)
            {
                const Creature &m = creatures[males[k]];
                int d = std::max(wrapDist(f.x, m.x), wrapDist(f.y, m.y));
                if (d <= MATE_RANGE && d < bestDist)
                {
                    bestDist = d;
                    best = k;
                }
            }
            if (best == -1)
                continue;

            Creature &m = creatures[males[best]];
            males.erase(males.begin() + best);

            float fGive = f.energy * 0.25f;
            float mGive = m.energy * 0.25f;
            f.energy -= fGive;
            m.energy -= mGive;

            Creature child;
            child.x = f.x;
            child.y = f.y;
            child.energy = fGive + mGive;
            child.sex = (coin(rng) < 0.5f) ? 'f' : 'm';

            child.mutationRate = inheritTrait(f.mutationRate, m.mutationRate, 0.001f, 0.5f, 0.01f);
            child.moveChance = inheritTrait(f.moveChance, m.moveChance, 0.0f, 1.0f, child.mutationRate);
            child.reproThreshold = inheritTrait(f.reproThreshold, m.reproThreshold, 20.0f, 100.0f, child.mutationRate);

            newborns.push_back(child);
        }

        creatures.insert(creatures.end(), newborns.begin(), newborns.end());
    }

    void removeDead()
    {
        int maxAge = p.maxAge;
        creatures.erase(
            std::remove_if(creatures.begin(), creatures.end(),
                           [maxAge](const Creature &c)
                           { return !c.isAlive(maxAge); }),
            creatures.end());
    }

    void update()
    {
        regrowFood();

        for (Creature &c : creatures)
        {
            c.age += 1;
            moveCreature(c);
            eat(c);
        }

        reproduce();
        removeDead();
        stepCount++;
    }
};