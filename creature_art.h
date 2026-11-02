#pragma once
#include <algorithm>
#include <cmath>
#include "raylib.h"
#include "creature.h"

// Draws a creature as a small character:
// body color shifts from green (healthy) to red (low energy),
// eyes look in the direction it last moved, and the hat shows sex (blue = male, pink = female).
inline void drawCreature(const Creature &c, int tile)
{
    float cx = c.x * tile + tile / 2.0f;
    float cy = c.y * tile + tile / 2.0f;
    float r = tile * 0.7f;

    float e = std::clamp(c.energy / 100.0f, 0.0f, 1.0f);
    Color body = Color{(unsigned char)(90 + 165 * (1.0f - e)),
                       (unsigned char)(90 + 140 * e), 110, 255};

    Vector2 center = {cx, cy};
    DrawCircleV(center, r, body);
    DrawCircleLinesV(center, r, BLACK);

    // eyes and pupils
    float eyeY = cy - r * 0.2f;
    for (int side = -1; side <= 1; side += 2)
    {
        float eyeX = cx + side * r * 0.35f;
        DrawCircleV({eyeX, eyeY}, r * 0.22f, WHITE);
        DrawCircleV({eyeX + c.dirX * r * 0.1f, eyeY + c.dirY * r * 0.1f}, r * 0.1f, BLACK);
    }

    // hat
    Color hat = (c.sex == 'm') ? SKYBLUE : PINK;
    DrawRectangleV({cx - r * 0.7f, cy - r * 1.1f}, {r * 1.4f, r * 0.4f}, hat);
}