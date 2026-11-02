#pragma once
#include <array>
#include <random>
#include <cmath>
#include <algorithm>

// 12 inputs: 6 sensing, 2 energy/hydration, 4 terrain (rock next to the creature)
const int IN = 12;
const int HID = 4;
const int OUT = 3;
const int W_IH = IN * HID;                   // input -> hidden weights
const int B_H = HID;                         // hidden biases
const int W_HO = HID * OUT;                  // hidden -> output weights
const int B_O = OUT;                         // output biases
const int WEIGHTS = W_IH + B_H + W_HO + B_O; // 63 total

struct Brain
{
    std::array<float, WEIGHTS> w{};

    void randomize(std::mt19937 &rng)
    {
        std::uniform_real_distribution<float> roll(-1.0f, 1.0f);
        for (float &x : w)
            x = roll(rng);
    }

    void forward(const std::array<float, IN> &in, std::array<float, OUT> &out) const
    {
        std::array<float, HID> hidden{};
        for (int h = 0; h < HID; h++)
        {
            float sum = w[W_IH + h];
            for (int i = 0; i < IN; i++)
                sum += in[i] * w[i * HID + h];
            hidden[h] = std::tanh(sum);
        }
        for (int o = 0; o < OUT; o++)
        {
            float sum = w[W_IH + B_H + W_HO + o];
            for (int h = 0; h < HID; h++)
                sum += hidden[h] * w[W_IH + B_H + h * OUT + o];
            out[o] = sum;
        }
    }
};