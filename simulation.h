#pragma once
class Simulation
{
public:
    int stepCount = 0;
    void update()
    {
        stepCount++;
    }
};