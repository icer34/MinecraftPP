#include "debug_draw.h"

void DebugDraw::line(glm::vec3 a, glm::vec3 b, glm::vec3 color, bool onTop)
{
    if (onTop)
    {
        _onTop.push_back({a, color});
        _onTop.push_back({b, color});
    }
    else
    {
        _depthTested.push_back({a, color});
        _depthTested.push_back({b, color});
    }
}

void DebugDraw::box(glm::vec3 min, glm::vec3 max, glm::vec3 color, bool onTop)
{
    // bottom face (y = min.y)
    line({min.x, min.y, min.z}, {max.x, min.y, min.z}, color, onTop);
    line({max.x, min.y, min.z}, {max.x, min.y, max.z}, color, onTop);
    line({max.x, min.y, max.z}, {min.x, min.y, max.z}, color, onTop);
    line({min.x, min.y, max.z}, {min.x, min.y, min.z}, color, onTop);

    // top face (y = max.y)
    line({min.x, max.y, min.z}, {max.x, max.y, min.z}, color, onTop);
    line({max.x, max.y, min.z}, {max.x, max.y, max.z}, color, onTop);
    line({max.x, max.y, max.z}, {min.x, max.y, max.z}, color, onTop);
    line({min.x, max.y, max.z}, {min.x, max.y, min.z}, color, onTop);

    // vertical edges, joining the corners of the two faces
    line({min.x, min.y, min.z}, {min.x, max.y, min.z}, color, onTop);
    line({max.x, min.y, min.z}, {max.x, max.y, min.z}, color, onTop);
    line({max.x, min.y, max.z}, {max.x, max.y, max.z}, color, onTop);
    line({min.x, min.y, max.z}, {min.x, max.y, max.z}, color, onTop);
}

void DebugDraw::clear()
{
    _onTop.clear();
    _depthTested.clear();
}

const std::vector<DebugVertex> &DebugDraw::depthTested() const { return _depthTested; }

const std::vector<DebugVertex> &DebugDraw::onTop() const { return _onTop; }