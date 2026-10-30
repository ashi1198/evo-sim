#pragma once
#include <random>
#include <vector>
#include <cstdint>
class Terrain
{
    std::vector<std::uint8_t> tiles;

public:
    Terrain(unsigned int seed)
    {
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> roll(0, 99);
        tiles.resize(100 * 100);
        for (std::uint8_t &tile : tiles)
        {
            int r = roll(rng);
            if (r < 50)
                tile = 0; // grass
            else if (r < 80)
                tile = 1; // dirt
            else
                tile = 2; // water
        }
    }
    int index(int x, int y) const
    {
        x = (x % 100 + 100) % 100; // wrap
        y = (y % 100 + 100) % 100;
        return y * 100 + x; // convert to array position
    }
    void smooth(int passes)
    {
        std::vector<std::uint8_t> next(tiles.size());
        for (int p = 0; p < passes; p++)
        {
            for (int y = 0; y < 100; y++)
            {
                for (int x = 0; x < 100; x++)
                {
                    int count[3] = {0, 0, 0}; // how many grass, water, rock
                    for (int dy = -1; dy <= 1; dy++)
                        for (int dx = -1; dx <= 1; dx++)
                            count[getTile(x + dx, y + dy)]++;
                    int best = 0;
                    for (int t = 1; t < 3; t++)
                        if (count[t] > count[best])
                            best = t;
                    next[index(x, y)] = static_cast<std::uint8_t>(best);
                }
            }
            tiles = next;
        }
    }
    int getTile(int x, int y) const
    {
        return tiles[index(x, y)];
    }
};