#pragma once

struct Creature
{
    int x = 0;
    int y = 0;
    float energy = 100.0f;
    float moveChance = 0.5f;      // 0 to 1
    float mutationRate = 0.05f;   // size of random change in children
    float reproThreshold = 80.0f; // energy needed to reproduce
    char sex = 'f';               // 'f' or 'm'
    int age = 0;                  // ticks

    bool isAlive(int maxAge) const
    {
        return energy > 0.0f && age < maxAge;
    }
};