#include "hud.h"

#include <glm/glm.hpp>

using glm::vec2;

Hud::Hud(int screenWidth, int screenHeight)
    : m_renderer(),
      m_screenW(screenWidth),
      m_screenH(screenHeight)
{
}

void Hud::render()
{
    m_renderer.begin();

    drawCrosshair();

    m_renderer.end(m_screenW, m_screenH);
}

void Hud::drawCrosshair()
{
    vec2 screenCenter(m_screenW / 2, m_screenH / 2);
    vec2 crosshairSize(CROSSHAIR_BASE_SIZE * m_crosshairScale);
    vec2 crosshairPos(screenCenter.x - crosshairSize.x / 2, screenCenter.y - crosshairSize.y / 2);

    m_renderer.drawIcon("crosshair", crosshairPos, crosshairSize);
}