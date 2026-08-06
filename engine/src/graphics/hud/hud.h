#pragma once

#include "hud_renderer.h"

class Hud
{
public:
    Hud(int screenWidth, int screenHeight);
    void render();

private:
    void drawCrosshair();

    // needs to track past messages, a msg is a sender, timestamp, content, (response if its a
    // command)
    // void drawChat();

    // drawHotbar(const Inventory& inv, selected slot);
    // drawPlayerStats(const Player& player) --> health, air, hunger, ...
    // ...

    int m_screenW;
    int m_screenH;

    float m_crosshairScale = 1.0f;
    static constexpr int CROSSHAIR_BASE_SIZE = 30;

    HudRenderer &m_renderer;
};