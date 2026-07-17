#pragma once

#include "game/chunk.h"
#include "mesh.h"

struct ChunkMeshData
{
    MeshData solidData;
    MeshData waterData;
};

class ChunkMesh : public Drawable
{
  public:
    explicit ChunkMesh(ChunkCoord coord)
        : m_coord(coord)
    {
    }

    void updateSolid(const MeshData &data);
    void updateWater(const MeshData &data);
    void draw() override;

    ChunkCoord getCoords() const { return m_coord; }

  private:
    ChunkCoord m_coord;
    Mesh m_solidMesh;
    Mesh m_waterMesh;
};
