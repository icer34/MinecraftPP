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

    // owns GL buffer handles that get freed in the destructor -- a shallow copy would leave
    // two Mesh instances sharing (and eventually double-freeing) the same handles, so copying
    // is disabled outright rather than left as an implicit, silently-broken default.
    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;

    void draw() override;
    void update(const MeshData &data);

private:
    unsigned int m_vao;
    unsigned int m_vbo;
    unsigned int m_ebo;
    size_t m_nVert = 0;
    size_t m_nIdx = 0;
};