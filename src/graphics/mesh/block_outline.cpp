#include "block_outline.h"

#include <glad/glad.h>

#include <glm/gtc/matrix_transform.hpp>
using glm::mat4;
using glm::vec3;

// 12 edges of a unit cube, laid out flat (2 vertices per edge, corners duplicated across
// shared edges, 3 floats per vertex) so glDrawArrays(GL_LINES, ...) can consume it directly --
// no EBO needed. sizeof(CUBE_EDGE_VERTICES) / (3 * sizeof(float)) == 24 vertices == 12 edges.
// clang-format off
static const float CUBE_EDGE_VERTICES[24 * 3] = {
    // edges along X
    0.0f, 0.0f, 0.0f,  1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,  1.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f,  1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 1.0f,  1.0f, 1.0f, 1.0f,
    // edges along Y
    0.0f, 0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
    1.0f, 0.0f, 0.0f,  1.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f,  0.0f, 1.0f, 1.0f,
    1.0f, 0.0f, 1.0f,  1.0f, 1.0f, 1.0f,
    // edges along Z
    0.0f, 0.0f, 0.0f,  0.0f, 0.0f, 1.0f,
    1.0f, 0.0f, 0.0f,  1.0f, 0.0f, 1.0f,
    0.0f, 1.0f, 0.0f,  0.0f, 1.0f, 1.0f,
    1.0f, 1.0f, 0.0f,  1.0f, 1.0f, 1.0f,
};
// clang-format on

BlockOutline::BlockOutline()
    : m_shader("shaders/outline_vert.glsl", "shaders/outline_frag.glsl")
{
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    glBufferData(GL_ARRAY_BUFFER, sizeof(CUBE_EDGE_VERTICES), CUBE_EDGE_VERTICES, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), nullptr);
    glEnableVertexAttribArray(0);
}

void BlockOutline::draw(glm::vec3 pos, const Camera &cam)
{
    m_shader.use();
    m_shader.setMat4("projection", cam.getProjectionMatrix());
    m_shader.setMat4("view", cam.getViewMatrix());
    mat4 model = glm::translate(mat4(1.0f), pos);
    m_shader.setMat4("model", model);

    glBindVertexArray(m_vao);
    glLineWidth(3.0f);

    glEnable(GL_POLYGON_OFFSET_LINE);
    glPolygonOffset(-1.0f, -1.0f);
    glDrawArrays(GL_LINES, 0, 24);
    glDisable(GL_POLYGON_OFFSET_LINE);
}
