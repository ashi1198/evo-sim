#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include "terrain.h"
#include "creature.h"
#include "params.h"

const float MAX_ENERGY = 100.0f;
const float MAX_HYDRATION = 100.0f;
const float HYDRATION_DRAIN = 0.03f; // per tick
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
        std::uniform_real_distribution<float> speedRoll(0.5f, 2.0f);
        std::uniform_real_distribution<float> visRoll(2.0f, 8.0f);
        std::uniform_real_distribution<float> hungerRoll(40.0f, 90.0f);
        std::uniform_real_distribution<float> thirstRoll(40.0f, 90.0f);
        for (int n = 0; n < START_CREATURES; n++)
        {
            Creature c;
            c.speed = speedRoll(rng);
            c.visibility = visRoll(rng);
            c.hungerThreshold = hungerRoll(rng);
            c.thirstThreshold = thirstRoll(rng);
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

    void drink(Creature &c)
    {
        uint8_t tile = terrain->getTile(c.x, c.y);
        if (tile == 1) // standing on water
        {
            c.hydration = std::min(MAX_HYDRATION, c.hydration + 10.0f);
        }
    }

    float speedCost(float speed) const
    {
        return p.moveCostGrass * std::log2(1.0f + speed);
    }

    bool findNearestFood(const Creature &c, int &outX, int &outY) const
    {
        int range = (int)c.visibility;
        int bestDist = range + 1;
        bool found = false;
        for (int dy = -range; dy <= range; dy++)
        {
            for (int dx = -range; dx <= range; dx++)
            {
                if (dx == 0 && dy == 0)
                    continue;
                int x = (c.x + dx + 100) % 100;
                int y = (c.y + dy + 100) % 100;
                int i = terrain->index(x, y);
                if (food[i] > 0.0f)
                {
                    int d = std::max(std::abs(dx), std::abs(dy));
                    if (d <= range && d < bestDist)
                    {
                        bestDist = d;
                        outX = x;
                        outY = y;
                        found = true;
                    }
                }
            }
        }
        return found;
    }

    bool findNearestWater(const Creature &c, int &outX, int &outY) const
    {
        int range = (int)c.visibility;
        int bestDist = range + 1;
        bool found = false;
        for (int dy = -range; dy <= range; dy++)
        {
            for (int dx = -range; dx <= range; dx++)
            {
                if (dx == 0 && dy == 0)
                    continue;
                int x = (c.x + dx + 100) % 100;
                int y = (c.y + dy + 100) % 100;
                if (terrain->getTile(x, y) == 1)
                {
                    int d = std::max(std::abs(dx), std::abs(dy));
                    if (d <= range && d < bestDist)
                    {
                        bestDist = d;
                        outX = x;
                        outY = y;
                        found = true;
                    }
                }
            }
        }
        return found;
    }
    int wrapStep(int from, int to) const
    {
        int d = (to - from + 100) % 100; // steps going forward, 0 to 99
        if (d == 0)
            return 0;
        return (d <= 50) ? 1 : -1; // shorter way around the wrap
    }

    bool findNearestMate(const Creature &c, int &outX, int &outY) const
    {
        int range = (int)c.visibility;
        int bestDist = range + 1;
        bool found = false;
        for (const Creature &o : creatures)
        {
            if (&o == &c)
                continue; // not itself
            if (o.sex == c.sex)
                continue; // opposite sex only
            if (o.energy < o.reproThreshold)
                continue; // must be ready too
            int d = std::max(wrapDist(c.x, o.x), wrapDist(c.y, o.y));
            if (d <= range && d < bestDist)
            {
                bestDist = d;
                outX = o.x;
                outY = o.y;
                found = true;
            }
        }
        return found;
    }
    void moveCreature(Creature &c)
    {
        int targetX = 0, targetY = 0;
        bool seeking = false;

        bool needsFood = c.energy < c.hungerThreshold;
        bool needsWater = c.hydration < c.thirstThreshold;
        bool readyToMate = c.energy >= c.reproThreshold;

        // priority: food and water first, then mates
        if (needsFood && needsWater)
        {
            float foodGap = c.hungerThreshold - c.energy;
            float waterGap = c.thirstThreshold - c.hydration;
            if (foodGap >= waterGap)
                seeking = findNearestFood(c, targetX, targetY);
            else
                seeking = findNearestWater(c, targetX, targetY);
        }
        else if (needsFood)
        {
            seeking = findNearestFood(c, targetX, targetY);
        }
        else if (needsWater)
        {
            seeking = findNearestWater(c, targetX, targetY);
        }
        else if (readyToMate)
        {
            seeking = findNearestMate(c, targetX, targetY);
        }

        bool inNeed = needsFood || needsWater;

        std::uniform_real_distribution<float> chance(0.0f, 1.0f);
        std::uniform_int_distribution<int> dir(-1, 1);
        int dx = 0, dy = 0;

        if (seeking)
        {
            dx = wrapStep(c.x, targetX);
            dy = wrapStep(c.y, targetY);
        }
        else if (inNeed || chance(rng) < c.moveChance)
        {
            dx = dir(rng);
            dy = dir(rng);
        }
        else
        {
            c.energy -= 0.05f; // resting
            return;
        }

        // already on the target tile (e.g. a mate on the same spot): no movement to pay for
        if (dx == 0 && dy == 0)
        {
            c.energy -= 0.05f;
            return;
        }

        c.moveProgress += c.speed; // this tick's movement budget
        while (c.moveProgress >= 1.0f)
        {
            c.moveProgress -= 1.0f; // spend one tile's worth of progress

            int newX = c.x + dx;
            int newY = c.y + dy;
            int tile = terrain->getTile(newX, newY);
            if (tile == 2)
            {
                c.energy -= 0.1f; // blocked by rock: stop for this tick
                c.moveProgress = 0.0f;
                break;
            }
            c.x = (newX % 100 + 100) % 100;
            c.y = (newY % 100 + 100) % 100;
            c.energy -= speedCost(c.speed) * (tile == 1 ? 2.5f : 1.0f);
            if (c.energy <= 0.0f)
            {
                c.moveProgress = 0.0f;
                break;
            }
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
            child.speed = inheritTrait(f.speed, m.speed, 0.5f, 5.0f, child.mutationRate); // reproduce()
            child.visibility = inheritTrait(f.visibility, m.visibility, 1.0f, 15.0f, child.mutationRate);
            child.hungerThreshold = inheritTrait(f.hungerThreshold, m.hungerThreshold, 20.0f, 95.0f, child.mutationRate);
            child.thirstThreshold = inheritTrait(f.thirstThreshold, m.thirstThreshold, 20.0f, 95.0f, child.mutationRate);
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
            c.hydration = std::max(0.0f, c.hydration - HYDRATION_DRAIN);
            moveCreature(c);
            eat(c);
            drink(c);
        }

        reproduce();
        removeDead();
        stepCount++;
    }
};