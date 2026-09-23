/**
 * @file chunk_mesher.h
 * @brief Converts chunk block data into renderable mesh data.
 */

#pragma once

#include "chunk_mesh.h"
#include "game/block_registry.h"
#include "game/chunk.h"
#include "util/directions.h"

#include <array>
#include <optional>

/**
 * @brief Corner of the atlas cell that a face vertex maps to.
 *
 * The vertex shader turns this into an actual UV offset from the cell origin, so nothing on
 * the CPU side ever deals with a raw UV value.
 */
enum class Corner : uint8_t
{
    BOTTOM_LEFT = 0,
    TOP_LEFT = 1,
    TOP_RIGHT = 2,
    BOTTOM_RIGHT = 3,
};

/**
 * @brief Builds the opaque and water meshes of a chunk, one face per visible block side.
 *
 * A face is emitted only when the neighboring block does not hide it, including across chunk
 * borders thanks to the neighboring chunks. Each vertex also stores its ambient occlusion
 * value and the column's temperature / humidity (for tinting).
 *
 * Has no GL dependency and no shared state, so it can run on worker threads.
 */
class ChunkMesher
{
public:
    /**
     * @brief Meshes one chunk.
     *
     * @param chunk the chunk to mesh
     * @param neighbors the four neighboring chunks, in CARDINAL_DIRECTIONS order; an entry is
     * nullptr if that neighbor is not loaded, in which case its blocks are treated as air
     * @param meshData receives the opaque and water geometry
     */
    void mesh(const Chunk &chunk, std::array<const Chunk *, 4> neighbors, ChunkMeshData &meshData);

private:
    /**
     * @brief determines if a given position is in the boundaries of the chunk or not.
     * if not, it returns the direction of the neighbor chunk where the position is.
     *
     * @return Direction of the neighbor chunk if out of bounds, std::nullopt otherwise
     */
    std::optional<Direction> posInChunk(int x, int y, int z);

    /**
     * @param localPos position of the block inside the chunk (in [0, Chunk::SIZE[)
     */
    void addSolidFace(const BlockType &block,
                      uint8_t humidity,
                      uint8_t temperature,
                      Direction dir,
                      glm::ivec3 localPos,
                      const Chunk &chunk,
                      const std::array<const Chunk *, 4> &neighbors,
                      std::vector<uint32_t> &vert,
                      std::vector<unsigned int> &indices);

    void addWaterFace(const BlockType &block,
                      uint8_t humidity,
                      uint8_t temperature,
                      Direction dir,
                      glm::ivec3 localPos,
                      const Chunk &chunk,
                      const std::array<const Chunk *, 4> &neighbors,
                      std::vector<uint32_t> &vert,
                      std::vector<unsigned int> &indices);

    glm::ivec3 wrapCoords(int x, int y, int z);

    // true if the block at pos is solid. pos may fall outside the chunk (including diagonally,
    // e.g. at a chunk corner) -- out of Y bounds or diagonally out of chunk bounds is treated
    // as "not solid" (air), which is an accepted simplification since `neighbors` only tracks
    // the 4 cardinal chunks, not diagonal ones.
    bool isSolidNeighbor(glm::ivec3 pos,
                         const Chunk &chunk,
                         std::array<const Chunk *, 4> neighbors);

    // 0..3 occlusion count for one corner of a face (0 = fully lit, 3 = most occluded).
    // see: https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
    uint8_t cornerAO(const Chunk &chunk,
                     std::array<const Chunk *, 4> neighbors,
                     glm::ivec3 localPos,
                     Direction dir,
                     Corner corner);
};