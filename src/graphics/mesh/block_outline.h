#pragma once

#include <glm/glm.hpp>

#include "graphics/camera.h"
#include "graphics/shader.h"

class BlockOutline
{
public:
    BlockOutline();
    void draw(glm::vec3 pos, const Camera &cam);

private:
    unsigned int m_vao;
    unsigned int m_vbo;
    Shader m_shader;
};