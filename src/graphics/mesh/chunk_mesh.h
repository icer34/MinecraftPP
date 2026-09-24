/**
 * @file chunk_mesh.h
 * @brief GPU meshes of one chunk, and the CPU-side data they are built from.
 */

#pragma once

#include "game/chunk.h"
#include "mesh.h"

/**
 * @brief CPU-side mesh data of one chunk, as produced by ChunkMesher.
 *
 * Contains no GL objects, so it can be built on a worker thread and uploaded later on the
 * main thread.
 */
struct ChunkMeshData
{
    MeshData solidData; ///< Opaque geometry.
    MeshData waterData; ///< Water geometry, drawn in a separate transparent pass.
};

/**
 * @brief GPU meshes of one chunk: one for opaque blocks, one for water.
 *
 * The two meshes are kept separate because water is drawn in its own pass, after the opaque
 * scene. Vertex positions are local to the chunk, so the renderer must apply the chunk
 * offset (see getCoords()).
 */
class ChunkMesh
{
public:
    /**
     * @param coord coordinates of the chunk this mesh belongs to
     */
    explicit ChunkMesh(ChunkCoord coord)
        : _coord(coord)
    {
    }

    /** @brief Uploads new opaque geometry, replacing the previous one. */
    void updateSolid(const MeshData &data);
    /** @brief Uploads new water geometry, replacing the previous one. */
    void updateWater(const MeshData &data);
    /**
     * @brief Draws the opaque geometry with the currently bound shader.
     *
     * @param vao the shared packed vertex format (see Mesh::createVertexArray())
     */
    void drawSolid(const GLVertexArray &vao) const;
    /**
     * @brief Draws the water geometry with the currently bound shader.
     *
     * @param vao the shared packed vertex format (see Mesh::createVertexArray())
     */
    void drawWater(const GLVertexArray &vao) const;

    /** @brief Coordinates of the chunk this mesh belongs to. */
    ChunkCoord getCoords() const { return _coord; }

private:
    ChunkCoord _coord;
    Mesh _solidMesh;
    Mesh _waterMesh;
};