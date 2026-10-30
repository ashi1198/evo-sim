#pragma once

struct Creature
{
    char sex = 'f';
    float mutationRate = 0.05f;
    int x = 0;
    int y = 0;
    float energy = 100.0f;
    float moveChance = 0.5f;
};