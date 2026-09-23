#include "mesh.h"

#include <glad/glad.h>
#include <iostream>

Mesh::Mesh()
{
    // create the buffers
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
}

void Mesh::update(const MeshData &data)
{
    _nVert = data.vertices.size();
    _nIdx = data.indices.size();

    glBindVertexArray(_vao);

    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, _nVert * sizeof(GLuint), data.vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER, _nIdx * sizeof(GLuint), data.indices.data(), GL_STATIC_DRAW);

    // packed data inside 2 32-bit unsigned integers
    glVertexAttribIPointer(0, 2, GL_UNSIGNED_INT, 0, 0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void Mesh::draw()
{
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, _nIdx, GL_UNSIGNED_INT, 0);
}
