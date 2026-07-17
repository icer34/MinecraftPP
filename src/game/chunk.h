#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>

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

    return (static_cast<uint64_t>(coord.x) & MASK)
         | ((static_cast<uint64_t>(coord.z) & MASK) << (2 * BITS));
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

    void setTemp(uint8_t temp, int x, int z) { m_temperature[index(x, z)] = temp; }
    uint8_t getTemp(int x, int z) const { return m_temperature[index(x, z)]; }

    void setHumidity(uint8_t value, int x, int z) { m_humidity[index(x, z)] = value; }
    uint8_t getHumidity(int x, int z) const { return m_humidity[index(x, z)]; }

    ChunkCoord getCoords() const { return m_coord; }

    uint64_t getID() const { return m_id; }

    bool isDirty() { return m_dirty; }
    void clearDirty() { m_dirty = false; }

  private:
    const uint64_t m_id;
    ChunkCoord m_coord;
    std::array<uint16_t, SIZE * SIZE * HEIGHT> m_blocks;

    //* temperature and humidity used to determine the tint of grass blocks for example
    std::array<uint8_t, SIZE * SIZE> m_temperature;
    std::array<uint8_t, SIZE * SIZE> m_humidity;

    bool m_dirty; // a chunk is set dirty if its modified thus needs to be remeshed

    static size_t index(int x, int y, int z)
    {
        if (x < 0 || y < 0 || z < 0)
        {
            throw std::runtime_error("CHUNK::INVALID POSITION INDEXING\n");
        }
        return static_cast<size_t>(x + y * SIZE + z * SIZE * HEIGHT);
    }

    static size_t index(int x, int z)
    {
        if (x < 0 || z < 0)
        {
            throw std::runtime_error("CHUNK::INVALID POSITION INDEXING\n");
        }
        return static_cast<size_t>(x + z * SIZE);
    }

    static uint64_t computeId(ChunkCoord coord) { return computeChunkID(coord); }
};