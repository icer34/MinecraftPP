#include "block_outline.h"

#include <glad/glad.h>

#include "graphics/gl/gl_debug.h"

namespace
{
constexpr GLint BLOCK_POS_LOCATION = 0; // layout(location = 0) in outline_vert.glsl
constexpr GLsizei VERTEX_COUNT = 12 * 6; // 12 edges, 2 triangles each
} // namespace

BlockOutline::BlockOutline()
    : _vao(gl::createVertexArray()),
      _shader("shaders/outline_vert.glsl", "shaders/outline_frag.glsl")
{
    gl::setLabel(GL_VERTEX_ARRAY, _vao.id(), "Block outline (empty)");
}

void BlockOutline::draw(glm::vec3 pos)
{
    _shader.use();
    _shader.setVec3(BLOCK_POS_LOCATION, pos);

    // the edge quads face the camera with either winding
    glDisable(GL_CULL_FACE);
    glBindVertexArray(_vao.id());
    glDrawArrays(GL_TRIANGLES, 0, VERTEX_COUNT);
    glEnable(GL_CULL_FACE);
}
