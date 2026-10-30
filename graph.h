#pragma once
#include <vector>
#include "raylib.h"

inline void drawGraph(const std::vector<float> &values, float x, float y, float w, float h, Color color)
{
    DrawRectangleLines((int)x, (int)y, (int)w, (int)h, GRAY);
    if (values.size() < 2)
        return;

    float hi = 1.0f;
    for (float v : values)
    {
        if (v > hi)
            hi = v;
    }
    hi *= 1.1f;

    float step = w / (values.size() - 1);
    for (size_t i = 1; i < values.size(); i++)
    {
        float x1 = x + (i - 1) * step;
        float x2 = x + i * step;
        float y1 = y + h - (values[i - 1] / hi) * h;
        float y2 = y + h - (values[i] / hi) * h;
        DrawLineEx({x1, y1}, {x2, y2}, 2.0f, color);
    }
}