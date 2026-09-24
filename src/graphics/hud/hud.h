/**
 * @file hud.h
 * @brief In-game heads-up display (crosshair, and later hotbar, chat, player stats...).
 */

#pragma once

#include "hud_renderer.h"

/**
 * @brief Lays out and draws the in-game HUD elements on top of the 3D scene.
 *
 * Only decides what to draw and where: the actual batching and draw calls are delegated to
 * a HudRenderer.
 */
class Hud
{
public:
    /**
     * @param renderer renderer used to draw the HUD, must outlive this object
     */
    explicit Hud(HudRenderer &renderer);

    /**
     * @brief Draws the whole HUD for the current frame.
     *
     * @param screenWidth current screen width in pixels
     * @param screenHeight current screen height in pixels
     */
    void render(int screenWidth, int screenHeight);

private:
    void drawCrosshair();

    // needs to track past messages, a msg is a sender, timestamp, content, (response if its a
    // command)
    // void drawChat();

    // drawHotbar(const Inventory& inv, selected slot);
    // drawPlayerStats(const Player& player) --> health, air, hunger, ...
    // ...

    // size of the frame being drawn, updated by every render() call
    int _screenW = 0;
    int _screenH = 0;

    float _crosshairScale = 1.0f;
    static constexpr int CROSSHAIR_BASE_SIZE = 30;

    HudRenderer &_renderer;
};