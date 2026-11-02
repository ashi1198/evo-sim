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
        file << "step,population,avg_mutation_rate,avg_energy,avg_hydration,"
                "avg_speed,avg_visibility,avg_repro_threshold,avg_hunger_threshold,avg_thirst_threshold,food_total\n";
    }

    void record(int step, const std::vector<Creature> &creatures, float foodTotal)
    {
        int n = static_cast<int>(creatures.size());
        float sumMut = 0.0f, sumEnergy = 0.0f, sumHydration = 0.0f;
        float sumSpeed = 0.0f, sumVisibility = 0.0f;
        float sumRepro = 0.0f, sumHunger = 0.0f, sumThirst = 0.0f;

        for (const Creature &c : creatures)
        {

            sumMut += c.mutationRate;
            sumEnergy += c.energy;
            sumHydration += c.hydration;
            sumSpeed += c.speed;
            sumVisibility += c.visibility;
            sumRepro += c.reproThreshold;
            sumHunger += c.hungerThreshold;
            sumThirst += c.thirstThreshold;
        }

        auto avg = [n](float sum)
        { return n ? sum / n : 0.0f; };

        file << step << "," << n << "," << avg(sumMut) << ","
             << avg(sumEnergy) << "," << avg(sumHydration) << "," << avg(sumSpeed) << ","
             << avg(sumVisibility) << "," << avg(sumRepro) << "," << avg(sumHunger) << ","
             << avg(sumThirst) << "," << foodTotal << "\n";
        file.flush();
    }
};