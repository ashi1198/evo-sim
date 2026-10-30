#pragma once

struct Params
{
    float foodRegrow = 0.002f;    // food gained per tick on food spots
    float foodEnergy = 20.0f;     // energy per unit of food eaten
    float moveCostGrass = 0.2f;   // energy per move on grass
    float moveCostWater = 0.5f;   // energy per move on water
    float reproThreshold = 80.0f; // default energy needed to reproduce
    int maxAge = 600;             // ticks
};