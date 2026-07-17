#pragma once

#include <glm/glm.hpp>
#include <vector>

class Spline
{
  public:
    Spline(float xMin, float xMax, float yMin, float yMax);
    void addPoint(float x, float y);
    void removePoint(size_t index);
    void setPoint(size_t index, float x, float y);

    float get(float x) const;
    const std::vector<float> &getXValues() const { return m_xVal; }
    const std::vector<float> &getYValues() const { return m_yVal; }
    glm::vec2 getXBounds() const { return m_xBounds; }
    glm::vec2 getYBounds() const { return m_yBounds; }

  private:
    glm::vec2 m_xBounds;
    glm::vec2 m_yBounds;
    std::vector<float> m_xVal{};
    std::vector<float> m_yVal{};
};