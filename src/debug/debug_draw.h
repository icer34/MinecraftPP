#pragma once

#include <glm/glm.hpp>
#include <vector>

struct DebugVertex
{
    glm::vec3 pos;
    glm::vec3 color;
};

/**
 * @brief Holds the vertex data of all the debug rendering (hitboxes, chunk outlines, ...). For now
 * you can only draw lines (or boxes), maybe more support later ??
 */
class DebugDraw
{
public:
    void line(glm::vec3 a, glm::vec3 b, glm::vec3 color, bool onTop = false);
    void box(glm::vec3 min, glm::vec3 max, glm::vec3 color, bool onTop = false);
    void clear();

    // 2 lists, one for the geometry that might be hidden behind terrain, one who is always on top
    const std::vector<DebugVertex> &depthTested() const;
    const std::vector<DebugVertex> &onTop() const;

private:
    std::vector<DebugVertex> _depthTested;
    std::vector<DebugVertex> _onTop;
};