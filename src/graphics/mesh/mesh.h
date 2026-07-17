#pragma once

#include "drawable.h"
#include <vector>

struct MeshData
{
    std::vector<uint32_t> vertices;
    std::vector<unsigned int> indices;
};

class Mesh : public Drawable
{
  public:
    Mesh();
    ~Mesh() override;

    void draw() override;
    void update(const MeshData &data);

  private:
    unsigned int m_vao;
    unsigned int m_vbo;
    unsigned int m_ebo;
    size_t m_nVert;
    size_t m_nIdx;
};