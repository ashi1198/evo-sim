#pragma once

struct Params
{
    float foodRegrow = 0.002f;
    float foodEnergy = 20.0f;
    float moveCostGrass = 0.2f;
    float moveCostWater = 0.5f;
    float reproThreshold = 80.0f;
    int maxAge = 600;
    float brainMutation = 0.03f; // size of random change to brain weights at birth
};