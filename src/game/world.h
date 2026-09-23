/**
 * @file world.h
 * @brief The voxel world: chunk streaming around the player, and block access.
 */

#pragma once

#include <array>
#include <cstdint>
#include <future>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

#include "BS_thread_pool.hpp"
#include "chunk.h"
#include "graphics/mesh/chunk_mesh.h"
#include "graphics/mesh/chunk_mesher.h"
#include "terrain_generator.h"

/**
 * @brief A loaded chunk and its GPU mesh.
 *
 * A chunk and its mesh always exist and disappear together, so they are kept as one map
 * entry instead of two maps that would need to be kept in sync.
 */
struct ChunkData
{
    std::shared_ptr<Chunk> chunk;    ///< Block data, shared read-only with worker threads.
    std::unique_ptr<ChunkMesh> mesh; ///< GPU mesh, only touched on the main thread.
};

/**
 * @brief What a worker thread hands back after generating or remeshing a chunk.
 *
 * Pure CPU data, with no GL objects.
 */
struct ChunkBuildResult
{
    ChunkCoord coord; ///< Coordinates of the chunk.
    /// The newly generated chunk, or nullptr if this was a remesh (the chunk's blocks did not
    /// change).
    std::shared_ptr<Chunk> newChunk;
    std::unique_ptr<ChunkMeshData> meshData; ///< The mesh built for the chunk.
    /// Bit i is set if the CARDINAL_DIRECTIONS[i] neighbor was loaded when this mesh was built.
    uint8_t neighborMask;
};

/**
 * @brief The voxel world: streams chunks in and out around the player and gives block access.
 *
 * Chunks are loaded up to LOAD_DISTANCE chunks from the player, but only drawn up to
 * RENDER_DISTANCE. The extra ring exists so that the drawn edge chunks have real neighbors
 * to cull their border faces against.
 *
 * Chunk generation and meshing run on a thread pool. Finished results are collected by
 * update() on the main thread, which also uploads the meshes to the GPU. When a chunk
 * appears, its already-loaded neighbors are remeshed so their border faces stay correct.
 *
 * Every method must be called from the main thread.
 */
class World
{
public:
    /**
     * @param seed world seed, forwarded to the TerrainGenerator
     */
    World(unsigned long seed);

    /**
     * @brief Streams chunks around the player. Call once per frame.
     *
     * Unloads the chunks that are too far away, schedules the missing ones, and uploads at
     * most MAX_MESH_UPLOADS_PER_FRAME finished meshes, so that crossing a chunk border does not
     * cause a lag spike.
     *
     * @param playerPos world position of the player
     * @param dt duration of the last frame, in seconds (unused)
     */
    void update(glm::vec3 playerPos, float dt);

    /**
     * @brief Unloads every chunk and discards the ones being built, so they are all generated
     * again by the next update() calls.
     */
    void regenerate();

    /**
     * @brief Replaces the block at `wPos` with air, calls its BlockType::onBreak callback and
     * remeshes the affected chunks.
     *
     * Does nothing if the position is outside the world height or in a chunk that is not
     * loaded.
     */
    void breakBlock(glm::vec3 wPos);

    /**
     * @brief Places a block at `wPos`, calls its BlockType::onPlace callback and remeshes the
     * affected chunks.
     *
     * Does nothing if the position is outside the world height or in a chunk that is not
     * loaded.
     *
     * @param block ID of the block to place
     * @param wPos world position of the block
     */
    void placeBlock(uint16_t block, glm::vec3 wPos);

    /**
     * @brief Returns the ID of the block at a world position.
     *
     * @return the block ID, or Blocks::AIR outside the world height or in an unloaded chunk
     */
    uint16_t getBlock(glm::vec3 wPos) const;

    /**
     * @brief Returns whether the block at a world position is solid.
     *
     * Unloaded chunks count as solid, so that the player cannot fall through the world before
     * it is generated. Outside the world height, blocks count as air.
     */
    bool isBlockSolid(glm::vec3 wPos) const;

    /**
     * @brief Returns the meshes of the loaded chunks within RENDER_DISTANCE of the player.
     *
     * Builds a new vector on every call.
     */
    std::vector<ChunkMesh *> getChunkMeshes() const;

    /**
     * @brief Returns every loaded chunk, including the ones only kept for neighbor data.
     *
     * Builds a new vector on every call.
     */
    std::vector<Chunk *> getChunks() const;

private:
    static constexpr int RENDER_DISTANCE = 12;
    static constexpr int LOAD_DISTANCE = RENDER_DISTANCE + 1;
    // max finished chunk meshes uploaded to the GPU per frame -- the rest stay ready in
    // _pending and get picked up on the next frames, so crossing a chunk border never
    // dumps a whole ring of glBufferData calls into a single frame
    static constexpr int MAX_MESH_UPLOADS_PER_FRAME = 8;

    unsigned long _seed;

    std::optional<glm::ivec3> getLocalPos(glm::vec3 wPos) const;

    //! ========== MAIN THREAD ONLY ==========
    ChunkCoord _playerCoord{};
    std::unordered_map<ChunkCoord, ChunkData> _chunks;

    // one in-flight future per coord currently being generated/remeshed -- also doubles
    // as the "already scheduled, don't submit it again" check in update().
    std::unordered_map<ChunkCoord, std::future<ChunkBuildResult>> _pending;

    void scheduleGenerate(ChunkCoord coord);
    void scheduleRemesh(ChunkCoord coord);

    // snapshot of the currently-loaded cardinal neighbors of coord, as shared_ptr handles
    // (cheap refcount bump) -- safe to hand to a worker thread since a Chunk is never
    // mutated once inserted into _chunks.
    std::array<std::shared_ptr<const Chunk>, 4> copyNeighbors(ChunkCoord coord) const;
    // same bit layout as ChunkBuildResult::neighborMask, for the currently-loaded neighbors
    uint8_t currentNeighborMask(ChunkCoord coord) const;
    //!========================================

    BS::light_thread_pool _pool;
};