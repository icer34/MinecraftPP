#include "mesh.h"

#include <glad/glad.h>
#include <iostream>

#include "core/chunk_attrib_registry.h"

Mesh::Mesh()
{
    // create the buffers
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &m_vao);
    glDeleteBuffers(1, &m_vbo);
    glDeleteBuffers(1, &m_ebo);
}

void Mesh::update(const MeshData &data)
{
    m_nVert = data.vertices.size();
    m_nIdx = data.indices.size();

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_nVert * sizeof(GLuint), data.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_nIdx * sizeof(GLuint), data.indices.data(), GL_STATIC_DRAW);

    size_t N = ChunkAttribRegistry::instance().computeVertexWordCount();
    glVertexAttribIPointer(
        0, static_cast<GLint>(N), GL_UNSIGNED_INT, static_cast<GLsizei>(N * sizeof(uint32_t)), (void *)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Mesh::draw()
{
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, m_nIdx, GL_UNSIGNED_INT, 0);
}
