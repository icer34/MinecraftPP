#include "debug_line_renderer.h"

#include <algorithm>
#include <cstddef>

#include <glad/glad.h>

#include "debug/debug_draw.h"
#include "graphics/gl/gl_debug.h"

namespace
{
constexpr GLuint VERTEX_BINDING = 0;
constexpr GLuint POS_ATTRIB = 0;   // layout(location = 0) in debug_line_vert.glsl
constexpr GLuint COLOR_ATTRIB = 1; // layout(location = 1) in debug_line_vert.glsl

// enough for a few hundred lines; chunk borders will make it grow once
constexpr size_t INITIAL_CAPACITY = 1024;
} // namespace

DebugLineRenderer::DebugLineRenderer()
    : _vao(gl::createVertexArray()),
      _shader("shaders/debug_line_vert.glsl", "shaders/debug_line_frag.glsl")
{
    // vertex format, read from binding 0: vec3 pos, vec3 color
    glEnableVertexArrayAttrib(_vao.id(), POS_ATTRIB);
    glVertexArrayAttribFormat(
        _vao.id(), POS_ATTRIB, 3, GL_FLOAT, GL_FALSE, offsetof(DebugVertex, pos));
    glVertexArrayAttribBinding(_vao.id(), POS_ATTRIB, VERTEX_BINDING);

    glEnableVertexArrayAttrib(_vao.id(), COLOR_ATTRIB);
    glVertexArrayAttribFormat(
        _vao.id(), COLOR_ATTRIB, 3, GL_FLOAT, GL_FALSE, offsetof(DebugVertex, color));
    glVertexArrayAttribBinding(_vao.id(), COLOR_ATTRIB, VERTEX_BINDING);

    gl::setLabel(GL_VERTEX_ARRAY, _vao.id(), "Debug lines vertex format");

    reserve(INITIAL_CAPACITY);
}

void DebugLineRenderer::reserve(size_t vertices)
{
    if (vertices <= _capacity)
        return;

    // at least double, so that a growing number of lines doesn't recreate the buffer every frame
    _capacity = std::max(vertices, 2 * _capacity);

    // fixed-capacity buffer rewritten every frame with glNamedBufferSubData
    _vbo = gl::createBuffer();
    glNamedBufferStorage(
        _vbo.id(), _capacity * sizeof(DebugVertex), nullptr, GL_DYNAMIC_STORAGE_BIT);
    gl::setLabel(GL_BUFFER, _vbo.id(), "Debug lines");

    // the VAO still points to the old buffer, which was just deleted: attach the new one
    glVertexArrayVertexBuffer(_vao.id(), VERTEX_BINDING, _vbo.id(), 0, sizeof(DebugVertex));
}

void DebugLineRenderer::draw(const DebugDraw &shapes)
{
    const std::vector<DebugVertex> &tested = shapes.depthTested();
    const std::vector<DebugVertex> &onTop = shapes.onTop();
    if (tested.empty() && onTop.empty())
        return;

    // both lists in the same buffer, one after the other, drawn as two ranges
    reserve(tested.size() + onTop.size());
    glNamedBufferSubData(_vbo.id(), 0, tested.size() * sizeof(DebugVertex), tested.data());
    glNamedBufferSubData(_vbo.id(),
                         tested.size() * sizeof(DebugVertex),
                         onTop.size() * sizeof(DebugVertex),
                         onTop.data());

    _shader.use();
    glBindVertexArray(_vao.id());

    // the lines never hide what is drawn after them. glDepthFunc is left untouched: the reverse-Z
    // GL_GEQUAL set by Renderer::renderWorld() is the right test
    glDepthMask(GL_FALSE);

    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(tested.size()));

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_CLAMP); // not clipped by the near / far planes either
    glDrawArrays(GL_LINES, static_cast<GLint>(tested.size()), static_cast<GLsizei>(onTop.size()));
    glDisable(GL_DEPTH_CLAMP);
    glEnable(GL_DEPTH_TEST);

    glDepthMask(GL_TRUE);
    glBindVertexArray(0);
}
