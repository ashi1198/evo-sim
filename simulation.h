#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cmath>
#include "terrain.h"
#include "creature.h"
#include "params.h"

const float MAX_ENERGY = 100.0f;
const float MAX_HYDRATION = 100.0f;
const float HYDRATION_DRAIN = 0.03f;
const int START_CREATURES = 500;
const int MATE_RANGE = 2;
const int MAP = 100;
const int STEP_X[4] = {1, -1, 0, 0}; // right, left, down, up
const int STEP_Y[4] = {0, 0, 1, -1};

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
    std::vector<int> readyMates; // indices of creatures ready to mate, rebuilt each tick

    Simulation(Terrain &t, unsigned int seed, Params params)
    {
        terrain = &t;
        p = params;
        rng.seed(seed);
        food.assign(MAP * MAP, 0.0f);
        isFoodSpot.assign(MAP * MAP, false);

        std::uniform_real_distribution<float> roll_food(0.0f, 1.0f);
        for (int y = 0; y < MAP; y++)
            for (int x = 0; x < MAP; x++)
            {
                float roll = roll_food(rng);
                if (terrain->getTile(x, y) == 0 && roll < 0.3f)
                {
                    int i = terrain->index(x, y);
                    food[i] = 1.0f;
                    isFoodSpot[i] = true;
                }
            }

        std::uniform_int_distribution<int> pos(0, MAP - 1);
        std::uniform_real_distribution<float> chance(0.0f, 1.0f);
        std::uniform_real_distribution<float> speedRoll(0.5f, 2.0f);
        std::uniform_real_distribution<float> visRoll(2.0f, 8.0f);
        std::uniform_real_distribution<float> hungerRoll(40.0f, 90.0f);
        std::uniform_real_distribution<float> thirstRoll(40.0f, 90.0f);
        std::uniform_int_distribution<int> agePick(0, 400);

        for (int n = 0; n < START_CREATURES; n++)
        {
            Creature c;
            c.speed = speedRoll(rng);
            c.visibility = visRoll(rng);
            c.hungerThreshold = hungerRoll(rng);
            c.thirstThreshold = thirstRoll(rng);
            int x = pos(rng), y = pos(rng);
            int tries = 0;
            while (terrain->getTile(x, y) == 2 && tries < 10000)
            {
                x = pos(rng);
                y = pos(rng);
                tries++;
            }
            c.x = x;
            c.y = y;
            c.reproThreshold = p.reproThreshold;
            c.age = agePick(rng);
            c.brain.randomize(rng);
            c.sex = (chance(rng) < 0.5f) ? 'f' : 'm';
            creatures.push_back(c);
        }
    }

    static int wrap(int v)
    {
        return (v % MAP + MAP) % MAP;
    }

    int tileAt(int x, int y) const
    {
        return terrain->getTile(wrap(x), wrap(y));
    }

    void regrowFood()
    {
        for (int i = 0; i < MAP * MAP; i++)
            if (isFoodSpot[i])
                food[i] = std::min(1.0f, food[i] + p.foodRegrow);
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
        if (tileAt(c.x, c.y) == 1)
            c.hydration = std::min(MAX_HYDRATION, c.hydration + 10.0f);
    }

    float speedCost(float speed) const
    {
        return p.moveCostGrass * std::log2(1.0f + speed);
    }

    int wrapDist(int a, int b) const
    {
        int d = std::abs(a - b);
        return std::min(d, MAP - d);
    }

    int wrapStep(int from, int to) const
    {
        int d = (to - from + MAP) % MAP;
        if (d == 0)
            return 0;
        return (d <= MAP / 2) ? 1 : -1;
    }

    bool findNearestFood(const Creature &c, int &outX, int &outY) const
    {
        int range = (int)c.visibility;
        int bestDist = range + 1;
        bool found = false;
        for (int dy = -range; dy <= range; dy++)
            for (int dx = -range; dx <= range; dx++)
            {
                if (dx == 0 && dy == 0)
                    continue;
                int x = wrap(c.x + dx), y = wrap(c.y + dy);
                if (food[terrain->index(x, y)] > 0.0f)
                {
                    int d = std::max(std::abs(dx), std::abs(dy));
                    if (d < bestDist)
                    {
                        bestDist = d;
                        outX = x;
                        outY = y;
                        found = true;
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
            for (int dx = -range; dx <= range; dx++)
            {
                if (dx == 0 && dy == 0)
                    continue;
                int x = wrap(c.x + dx), y = wrap(c.y + dy);
                if (tileAt(x, y) == 1)
                {
                    int d = std::max(std::abs(dx), std::abs(dy));
                    if (d < bestDist)
                    {
                        bestDist = d;
                        outX = x;
                        outY = y;
                        found = true;
                    }
                }
            }
        return found;
    }

    bool findNearestMate(const Creature &c, int &outX, int &outY) const
    {
        int range = (int)c.visibility;
        int bestDist = range + 1;
        bool found = false;
        for (int idx : readyMates)
        {
            const Creature &o = creatures[idx];
            if (&o == &c || o.sex == c.sex)
                continue;
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

    void setDirInputs(const Creature &c, bool found, int tx, int ty,
                      std::array<float, IN> &in, int k) const
    {
        in[k] = found ? (float)wrapStep(c.x, tx) : 0.0f;
        in[k + 1] = found ? (float)wrapStep(c.y, ty) : 0.0f;
    }

    void buildInputs(const Creature &c, std::array<float, IN> &in) const
    {
        int tx = 0, ty = 0;
        setDirInputs(c, findNearestFood(c, tx, ty), tx, ty, in, 0);
        setDirInputs(c, findNearestWater(c, tx, ty), tx, ty, in, 2);
        setDirInputs(c, findNearestMate(c, tx, ty), tx, ty, in, 4);

        in[6] = c.energy / 100.0f;
        in[7] = c.hydration / 100.0f;

        for (int k = 0; k < 4; k++)
            in[8 + k] = (tileAt(c.x + STEP_X[k], c.y + STEP_Y[k]) == 2) ? 1.0f : 0.0f;
    }

    static int toMove(float v)
    {
        return v > 0.5f ? 1 : (v < -0.5f ? -1 : 0);
    }

    void moveCreature(Creature &c)
    {
        std::array<float, IN> in{};
        std::array<float, OUT> out{};
        buildInputs(c, in);
        c.brain.forward(in, out);

        int dx = toMove(out[0]);
        int dy = toMove(out[1]);
        if (out[2] <= 0.0f || (dx == 0 && dy == 0))
        {
            c.energy -= 0.05f; // resting
            return;
        }

        c.dirX = dx;
        c.dirY = dy;
        c.moveProgress += c.speed;
        while (c.moveProgress >= 1.0f)
        {
            c.moveProgress -= 1.0f;
            int nx = wrap(c.x + dx), ny = wrap(c.y + dy);
            int tile = terrain->getTile(nx, ny);
            if (tile == 2)
            {
                c.energy -= 0.1f; // bumped into rock
                c.moveProgress = 0.0f;
                break;
            }
            c.x = nx;
            c.y = ny;
            c.energy -= speedCost(c.speed) * (tile == 1 ? 2.5f : 1.0f);
            if (c.energy <= 0.0f)
                break;
        }
    }

    float inheritTrait(float a, float b, float lo, float hi, float mutRate)
    {
        std::uniform_real_distribution<float> coin(0.0f, 1.0f);
        std::uniform_real_distribution<float> mut(-1.0f, 1.0f);
        float choice = coin(rng);
        float pick = coin(rng);
        float base;
        if (choice < 0.5f)
            base = (a + b) / 2.0f;
        else
            base = (pick < 0.5f) ? a : b;
        float value = base + mut(rng) * mutRate * (hi - lo);
        return std::clamp(value, lo, hi);
    }

    void reproduce()
    {
        std::vector<int> males;
        for (int i = 0; i < (int)creatures.size(); i++)
            if (creatures[i].sex == 'm' && creatures[i].energy >= creatures[i].reproThreshold)
                males.push_back(i);

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
                const Creature &cand = creatures[males[k]];
                int d = std::max(wrapDist(f.x, cand.x), wrapDist(f.y, cand.y));
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
            child.speed = inheritTrait(f.speed, m.speed, 0.5f, 5.0f, child.mutationRate);
            child.visibility = inheritTrait(f.visibility, m.visibility, 1.0f, 15.0f, child.mutationRate);
            child.hungerThreshold = inheritTrait(f.hungerThreshold, m.hungerThreshold, 20.0f, 95.0f, child.mutationRate);
            child.thirstThreshold = inheritTrait(f.thirstThreshold, m.thirstThreshold, 20.0f, 95.0f, child.mutationRate);
            child.reproThreshold = p.reproThreshold;

            // whole brain from one parent, then mutate: keeps each network's structure intact
            const Creature &src = (coin(rng) < 0.5f) ? f : m;
            child.brain = src.brain;
            std::uniform_real_distribution<float> mutW(-1.0f, 1.0f);
            for (float &x : child.brain.w)
                x = std::clamp(x + mutW(rng) * p.brainMutation, -2.0f, 2.0f);

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

        readyMates.clear();
        for (int i = 0; i < (int)creatures.size(); i++)
            if (creatures[i].energy >= creatures[i].reproThreshold)
                readyMates.push_back(i);

        for (Creature &c : creatures)
        {
            c.age++;
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