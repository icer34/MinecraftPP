/**
 * @file chunk.h
 * @brief A 16x256x16 column of blocks, and its coordinates.
 */

#pragma once

#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <stdexcept>

#include "blocks.h"

/**
 * @brief Position of a chunk on the horizontal chunk grid.
 *
 * Chunk (x, z) covers world blocks [x * Chunk::SIZE, (x + 1) * Chunk::SIZE[ on X, and the
 * same on Z.
 */
struct ChunkCoord
{
    int32_t x; ///< Chunk X coordinate.
    int32_t z; ///< Chunk Z coordinate.
    /** @brief True if both coordinates are equal. */
    bool operator==(const ChunkCoord &o) const { return x == o.x && z == o.z; }
};

/**
 * @brief Packs chunk coordinates into a unique 64-bit ID.
 *
 * Each coordinate keeps its lowest 21 bits, so IDs are unique as long as both coordinates stay
 * within [-2^20, 2^20[.
 */
inline uint64_t computeChunkID(ChunkCoord coord)
{
    constexpr int BITS = 21;
    constexpr uint64_t MASK = (1ull << BITS) - 1;

    return (static_cast<uint64_t>(coord.x) & MASK)
         | ((static_cast<uint64_t>(coord.z) & MASK) << (2 * BITS));
}

/// @cond
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
/// @endcond

/**
 * @brief A column of SIZE x HEIGHT x SIZE blocks, plus per-column climate data.
 *
 * Block positions are local to the chunk: x and z in [0, SIZE[, y in [0, HEIGHT[.
 * Temperature and humidity are stored once per (x, z) column and are used to tint blocks
 * like grass.
 */
class Chunk
{
public:
    static constexpr int SIZE = 16;    ///< Width and depth of a chunk, in blocks.
    static constexpr int HEIGHT = 256; ///< Height of a chunk (and of the world), in blocks.

    /**
     * @brief Creates a chunk filled with air.
     *
     * @param coord position of the chunk on the chunk grid
     */
    explicit Chunk(ChunkCoord coord)
        : _coord(coord),
          _id(computeId(coord))
    {
        _blocks.fill(Blocks::AIR);
    }

    /**
     * @brief Returns the block ID at a local position.
     *
     * @throws std::out_of_range if the position is outside the chunk
     */
    uint16_t getBlock(glm::ivec3 pos) const { return _blocks.at(index(pos)); }

    /**
     * @brief Sets the block at a local position and marks the chunk as dirty. No bounds check.
     */
    void setBlock(uint16_t id, glm::ivec3 pos)
    {
        _blocks[index(pos)] = id;
        _dirty = true;
    }

    /** @brief Sets the temperature of the (x, z) column `pos`. */
    void setTemp(uint8_t temp, glm::ivec2 pos) { _temperature[index(pos)] = temp; }
    /** @brief Temperature of the (x, z) column `pos`, in [0, 255]. */
    uint8_t getTemp(glm::ivec2 pos) const { return _temperature[index(pos)]; }

    /** @brief Sets the humidity of the (x, z) column `pos`. */
    void setHumidity(uint8_t value, glm::ivec2 pos) { _humidity[index(pos)] = value; }
    /** @brief Humidity of the (x, z) column `pos`, in [0, 255]. */
    uint8_t getHumidity(glm::ivec2 pos) const { return _humidity[index(pos)]; }

    /** @brief Position of the chunk on the chunk grid. */
    ChunkCoord getCoords() const { return _coord; }

    /** @brief Unique ID of the chunk, see computeChunkID(). */
    uint64_t getID() const { return _id; }

    /** @brief True if a block was changed since the last clearDirty(). */
    bool isDirty() { return _dirty; }
    /** @brief Resets the dirty flag. */
    void clearDirty() { _dirty = false; }

private:
    ChunkCoord _coord;
    const uint64_t _id;
    std::array<uint16_t, SIZE * SIZE * HEIGHT> _blocks;

    //* temperature and humidity used to determine the tint of grass blocks for example
    std::array<uint8_t, SIZE * SIZE> _temperature;
    std::array<uint8_t, SIZE * SIZE> _humidity;

    bool _dirty = false; // a chunk is set dirty if its modified thus needs to be remeshed

    static size_t index(glm::ivec3 pos)
    {
        return static_cast<size_t>(pos.x + pos.y * SIZE + pos.z * SIZE * HEIGHT);
    }

    static size_t index(glm::ivec2 pos) { return static_cast<size_t>(pos.x + pos.y * SIZE); }

    static uint64_t computeId(ChunkCoord coord) { return computeChunkID(coord); }
};