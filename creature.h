#pragma once

struct Creature
{
    int x = 0;
    int y = 0;
    float energy = 100.0f;
    float hydration = 100.0f;

    float moveChance = 0.5f;       // 0 to 1
    float mutationRate = 0.05f;    // size of random change in children
    float reproThreshold = 80.0f;  // energy needed to reproduce
    float hungerThreshold = 70.0f; // seek food when energy drops below this
    float thirstThreshold = 70.0f; // seek water when hydration drops below this
    float speed = 1.0f;            // tiles moved per tick
    float visibility = 5.0f;       // tiles it can see
    float moveProgress = 0.0f;     // accumulates speed; one tile per whole unit
    char sex = 'f';                // 'f' or 'm'
    int age = 0;                   // ticks

    bool isAlive(int maxAge) const
    {
        return energy > 0.0f && hydration > 0.0f && age < maxAge;
    }
};