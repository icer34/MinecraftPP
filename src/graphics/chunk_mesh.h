#pragma once

#include "mesh.h"

class ChunkMesh : public Drawable
{
public:
    void updateSolid(const MeshData& data);
    void updateWater(const MeshData& data);
    void draw() override;

private:
    Mesh m_solidMesh;
    Mesh m_waterMesh;
};