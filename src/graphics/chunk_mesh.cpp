#include "chunk_mesh.h"

void ChunkMesh::draw()
{
    m_solidMesh.draw();
    m_waterMesh.draw();
}

void ChunkMesh::updateSolid(const MeshData &data) { m_solidMesh.update(data); }

void ChunkMesh::updateWater(const MeshData &data) { m_waterMesh.update(data); }