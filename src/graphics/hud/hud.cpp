#include "hud.h"

#include <glm/glm.hpp>

using glm::vec2;

Hud::Hud(HudRenderer &renderer)
    : _renderer(renderer)
{
}

void Hud::render(int screenWidth, int screenHeight)
{
    _screenW = screenWidth;
    _screenH = screenHeight;

    _renderer.begin();

    drawCrosshair();

    _renderer.end(_screenW, _screenH);
}

void Hud::drawCrosshair()
{
    vec2 screenCenter(_screenW / 2, _screenH / 2);
    vec2 crosshairSize(CROSSHAIR_BASE_SIZE * _crosshairScale);
    vec2 crosshairPos(screenCenter.x - crosshairSize.x / 2, screenCenter.y - crosshairSize.y / 2);

    _renderer.drawIcon("crosshair", crosshairPos, crosshairSize);
}