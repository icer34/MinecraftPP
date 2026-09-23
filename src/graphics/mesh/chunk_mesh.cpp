#include "chunk_mesh.h"

void ChunkMesh::drawSolid() { _solidMesh.draw(); }

void ChunkMesh::drawWater() { _waterMesh.draw(); }

void ChunkMesh::updateSolid(const MeshData &data) { _solidMesh.update(data); }

void ChunkMesh::updateWater(const MeshData &data) { _waterMesh.update(data); }