#include "mesh.h"

#include <string>

#include "graphics/gl/gl_debug.h"

namespace
{
constexpr GLuint VERTEX_BINDING = 0;
constexpr GLuint PACKED_DATA_ATTRIB = 0;
constexpr GLsizei VERTEX_STRIDE = 2 * sizeof(uint32_t); // two packed 32-bit words
} // namespace

GLVertexArray Mesh::createVertexArray()
{
    GLVertexArray vao = gl::createVertexArray();

    // vertex format: attribute 0 = uvec2 of packed data, read from binding 0
    glEnableVertexArrayAttrib(vao.id(), PACKED_DATA_ATTRIB);
    glVertexArrayAttribIFormat(vao.id(), PACKED_DATA_ATTRIB, 2, GL_UNSIGNED_INT, 0);
    glVertexArrayAttribBinding(vao.id(), PACKED_DATA_ATTRIB, VERTEX_BINDING);

    gl::setLabel(GL_VERTEX_ARRAY, vao.id(), "Packed mesh format");
    return vao;
}

void Mesh::update(const MeshData &data, std::string_view label)
{
    _nIdx = data.indices.size();

    // immutable storage cannot be empty: an empty mesh simply has no buffers
    if (data.vertices.empty() || data.indices.empty())
    {
        _vbo.reset();
        _ebo.reset();
        _nIdx = 0;
        return;
    }

    // immutable buffers cannot be resized: new ones replace the old ones, which their handles
    // delete
    _vbo = gl::createBuffer();
    glNamedBufferStorage(
        _vbo.id(), data.vertices.size() * sizeof(uint32_t), data.vertices.data(), 0);

    _ebo = gl::createBuffer();
    glNamedBufferStorage(_ebo.id(), data.indices.size() * sizeof(uint32_t), data.indices.data(), 0);

    gl::setLabel(GL_BUFFER, _vbo.id(), std::string(label) + " vertices");
    gl::setLabel(GL_BUFFER, _ebo.id(), std::string(label) + " indices");
}

void Mesh::draw(const GLVertexArray &vao) const
{
    if (_nIdx == 0)
        return;

    glVertexArrayVertexBuffer(vao.id(), VERTEX_BINDING, _vbo.id(), 0, VERTEX_STRIDE);
    glVertexArrayElementBuffer(vao.id(), _ebo.id());

    glBindVertexArray(vao.id());
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(_nIdx), GL_UNSIGNED_INT, nullptr);
}
