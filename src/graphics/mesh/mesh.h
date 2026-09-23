/**
 * @file mesh.h
 * @brief Indexed GPU mesh with packed vertices.
 */

#pragma once

#include "drawable.h"

#include <cstdint>
#include <vector>

/**
 * @brief CPU-side geometry of a mesh, ready to be uploaded with Mesh::update().
 */
struct MeshData
{
    /// Packed vertex data: each vertex is two consecutive 32-bit words, unpacked by the
    /// vertex shader.
    std::vector<uint32_t> vertices;
    std::vector<unsigned int> indices; ///< Triangle indices into the vertex list.
};

/**
 * @brief Indexed triangle mesh stored on the GPU (VAO + VBO + EBO).
 *
 * Non-copyable because it owns GL handles. Must be created and destroyed while a GL context
 * is current.
 */
class Mesh : public Drawable
{
public:
    /**
     * @brief Creates the GL buffers. The mesh is empty until update() is called.
     */
    Mesh();
    ~Mesh() override;

    // owns GL buffer handles that get freed in the destructor -- a shallow copy would leave
    // two Mesh instances sharing (and eventually double-freeing) the same handles, so copying
    // is disabled outright rather than left as an implicit, silently-broken default.
    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;

    /**
     * @brief Draws the whole mesh as triangles with the currently bound shader.
     */
    void draw() override;

    /**
     * @brief Uploads new geometry, replacing the previous content of the buffers.
     */
    void update(const MeshData &data);

private:
    unsigned int _vao;
    unsigned int _vbo;
    unsigned int _ebo;
    size_t _nVert = 0;
    size_t _nIdx = 0;
};