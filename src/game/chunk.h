#pragma once

#include <array>
#include <cstdint>
#include <glm/glm.hpp>
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
    static constexpr int HEIGHT = 256;

    explicit Chunk(ChunkCoord coord)
        : m_coord(coord),
          m_id(computeId(coord))
    {
        m_blocks.fill(Blocks::AIR);
    }

    uint16_t getBlock(glm::ivec3 pos) const { return m_blocks.at(index(pos)); }

    void setBlock(uint16_t id, glm::ivec3 pos)
    {
        m_blocks[index(pos)] = id;
        m_dirty = true;
    }

    void setTemp(uint8_t temp, glm::ivec2 pos) { m_temperature[index(pos)] = temp; }
    uint8_t getTemp(glm::ivec2 pos) const { return m_temperature[index(pos)]; }

    void setHumidity(uint8_t value, glm::ivec2 pos) { m_humidity[index(pos)] = value; }
    uint8_t getHumidity(glm::ivec2 pos) const { return m_humidity[index(pos)]; }

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

    static size_t index(glm::ivec3 pos)
    {
        return static_cast<size_t>(pos.x + pos.y * SIZE + pos.z * SIZE * HEIGHT);
    }

    static size_t index(glm::ivec2 pos) { return static_cast<size_t>(pos.x + pos.y * SIZE); }

    static uint64_t computeId(ChunkCoord coord) { return computeChunkID(coord); }
};