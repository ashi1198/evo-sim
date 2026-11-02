#pragma once
#include "brain.h"

struct Creature
{
    int x = 0, y = 0;
    int dirX = 0, dirY = 1; // last move direction, used for drawing
    float energy = 100.0f;
    float hydration = 100.0f;
    float moveChance = 0.5f; // unused by the brain, kept so the logger still compiles
    float mutationRate = 0.05f;
    float reproThreshold = 80.0f;
    float hungerThreshold = 70.0f;
    float thirstThreshold = 70.0f;
    float speed = 1.0f;
    float visibility = 5.0f;
    float moveProgress = 0.0f; // fractional movement accumulator
    char sex = 'f';
    int age = 0;
    Brain brain;

    bool isAlive(int maxAge) const
    {
        return energy > 0 && hydration > 0 && age < maxAge;
    }
};