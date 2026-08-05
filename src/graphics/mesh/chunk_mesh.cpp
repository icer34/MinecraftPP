#include "chunk_mesh.h"

void ChunkMesh::drawSolid() { m_solidMesh.draw(); }

void ChunkMesh::drawWater() { m_waterMesh.draw(); }

void ChunkMesh::updateSolid(const MeshData &data) { m_solidMesh.update(data); }

void ChunkMesh::updateWater(const MeshData &data) { m_waterMesh.update(data); }