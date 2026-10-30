#pragma once
const int MAX_AGE = 600;
struct Creature
{
    char sex = 'f';
    float mutationRate = 0.05f;
    float reproThreshold = 80.0f;
    int x = 0;
    int y = 0;
    int age = 0;
    float energy = 100.0f;
    float moveChance = 0.5f;
    bool isAlive() const
    {
        return energy > 0.0f && age < MAX_AGE;
    }
};