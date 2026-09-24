#include "chunk_mesh.h"

#include <string>

namespace
{
std::string label(ChunkCoord coord, const char *kind)
{
    return "Chunk (" + std::to_string(coord.x) + ", " + std::to_string(coord.z) + ") " + kind;
}
} // namespace

void ChunkMesh::drawSolid(const GLVertexArray &vao) const { _solidMesh.draw(vao); }

void ChunkMesh::drawWater(const GLVertexArray &vao) const { _waterMesh.draw(vao); }

void ChunkMesh::updateSolid(const MeshData &data) { _solidMesh.update(data, label(_coord, "solid")); }

void ChunkMesh::updateWater(const MeshData &data) { _waterMesh.update(data, label(_coord, "water")); }
