/**
 * @file mesh.h
 * @brief Indexed GPU mesh with packed vertices.
 */

#pragma once

#include "graphics/gl/gl_objects.h"

#include <cstdint>
#include <string_view>
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
 * @brief Indexed triangle mesh stored on the GPU: one vertex buffer and one index buffer.
 *
 * A mesh has no VAO of its own: all meshes share the same vertex format, described once by
 * the VAO returned by createVertexArray(), and draw() attaches the mesh's buffers to it.
 *
 * Move-only because it owns GL handles. Must be created and destroyed while a GL context is
 * current.
 */
class Mesh
{
public:
    /**
     * @brief Creates a VAO describing the packed vertex format (one `uvec2` per vertex,
     * attribute 0), without any buffer attached. One is enough for every mesh.
     */
    static GLVertexArray createVertexArray();

    /**
     * @brief Draws the whole mesh as triangles with the currently bound shader.
     *
     * @param vao a VAO from createVertexArray(); binds it and attaches this mesh's buffers
     */
    void draw(const GLVertexArray &vao) const;

    /**
     * @brief Uploads new geometry, replacing the previous buffers.
     *
     * @param data the geometry to upload
     * @param label name given to the GL buffers, for the debug output and RenderDoc
     */
    void update(const MeshData &data, std::string_view label = "Mesh");

private:
    GLBuffer _vbo;
    GLBuffer _ebo;
    size_t _nIdx = 0;
};
