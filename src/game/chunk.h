#pragma once

#include <cstdint>
#include <array>

#include "blocks.h"

constexpr int CHUNK_SIZE = 16;

struct ChunkCoord {
    int x, y, z;
    bool operator==(const ChunkCoord& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
};

class Chunk
{
public:
    explicit Chunk(ChunkCoord coord)
    {
        m_blocks.fill(Blocks::AIR);
    }

    uint16_t getBlock(int x, int y, int z)
    {
        return m_blocks[index(x, y, z)];
    }

    void setBlock(uint16_t id, int x, int y, int z)
    {
        m_blocks[index(x, y, z)] = id;
        m_dirty = true;
    }

    bool isDirty() { return m_dirty; }
    void clearDirty() { m_dirty = false; }

private:
    static size_t index(int x, int y, int z)
    {
        return static_cast<size_t>(x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE);
    }

    ChunkCoord m_coord;
    std::array<uint16_t, CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE> m_blocks;
    bool m_dirty; //a chunk is set dirty if its modified thus needs to be remeshed
};