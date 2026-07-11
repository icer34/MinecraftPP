#pragma once

#include <array>
#include <cstdint>

#include "blocks.h"

struct ChunkCoord
{
    int32_t x, z;
    bool operator==(const ChunkCoord &o) const { return x == o.x && z == o.z; }
};

inline uint64_t computeChunkID(ChunkCoord coord)
{
    constexpr int BITS = 21;
    constexpr uint64_t MASK = (1ull << BITS) - 1;

    return (static_cast<uint64_t>(coord.x) & MASK) |
           ((static_cast<uint64_t>(coord.z) & MASK) << (2 * BITS));
}

namespace std
{
template <> struct hash<ChunkCoord>
{
    size_t operator()(const ChunkCoord &coord) const noexcept
    {
        return static_cast<size_t>(computeChunkID(coord));
    }
};
} // namespace std

class Chunk
{
  public:
    static constexpr int SIZE = 16;
    static constexpr int HEIGHT = 128;

    explicit Chunk(ChunkCoord coord)
        : m_coord(coord),
          m_id(computeId(coord))
    {
        m_blocks.fill(Blocks::AIR);
    }

    uint16_t getBlock(int x, int y, int z) const { return m_blocks[index(x, y, z)]; }

    void setBlock(uint16_t id, int x, int y, int z)
    {
        m_blocks[index(x, y, z)] = id;
        m_dirty = true;
    }

    ChunkCoord getCoords() const { return m_coord; }

    uint64_t getID() const { return m_id; }

    bool isDirty() { return m_dirty; }
    void clearDirty() { m_dirty = false; }

  private:
    const uint64_t m_id;
    ChunkCoord m_coord;
    std::array<uint16_t, SIZE * SIZE * HEIGHT> m_blocks;
    bool m_dirty; // a chunk is set dirty if its modified thus needs to be remeshed

    static size_t index(int x, int y, int z)
    {
        return static_cast<size_t>(x + y * SIZE + z * SIZE * HEIGHT);
    }

    static uint64_t computeId(ChunkCoord coord) { return computeChunkID(coord); }
};