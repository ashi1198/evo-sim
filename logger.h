#pragma once
#include <fstream>
#include <string>
#include <vector>
#include "creature.h"
#include "params.h"

class DataLogger
{
public:
    std::ofstream file;

    DataLogger(const std::string &path, unsigned int seed, const Params &p)
        : file(path)
    {
        file << "# seed=" << seed
             << " foodRegrow=" << p.foodRegrow
             << " foodEnergy=" << p.foodEnergy
             << " moveCostGrass=" << p.moveCostGrass
             << " moveCostWater=" << p.moveCostWater
             << " reproThreshold=" << p.reproThreshold
             << " maxAge=" << p.maxAge << "\n";
        file << "step,population,avg_move_chance,avg_mutation_rate,avg_energy,avg_hydration,avg_speed,avg_visibility,food_total\n";
    }

    void record(int step, const std::vector<Creature> &creatures, float foodTotal)
    {
        int n = static_cast<int>(creatures.size());
        float sumMove = 0.0f, sumMut = 0.0f, sumEnergy = 0.0f;
        float sumHydration = 0.0f, sumSpeed = 0.0f, sumVisibility = 0.0f;
        for (const Creature &c : creatures)
        {
            sumMove += c.moveChance;
            sumMut += c.mutationRate;
            sumEnergy += c.energy;
            sumHydration += c.hydration;
            sumSpeed += c.speed;
            sumVisibility += c.visibility;
        }
        float avgMove = n ? sumMove / n : 0.0f;
        float avgMut = n ? sumMut / n : 0.0f;
        float avgEnergy = n ? sumEnergy / n : 0.0f;
        float avgHydration = n ? sumHydration / n : 0.0f;
        float avgSpeed = n ? sumSpeed / n : 0.0f;
        float avgVisibility = n ? sumVisibility / n : 0.0f;

        file << step << "," << n << "," << avgMove << "," << avgMut << ","
             << avgEnergy << "," << avgHydration << "," << avgSpeed << ","
             << avgVisibility << "," << foodTotal << "\n";
        file.flush();
    }
};