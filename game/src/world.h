#pragma once

#include <array>
#include <future>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

#include "BS_thread_pool.hpp"
#include "core/chunk.h"
#include "graphics/mesh/chunk_mesh.h"
#include "graphics/mesh/chunk_mesher.h"
#include "terrain_generator.h"
#include "core/voxel_world.h"

// a chunk and its GPU mesh always exist/disappear together, so they're kept as one
// map entry instead of two separately-synced maps.
struct ChunkData
{
    std::shared_ptr<Chunk> chunk;
    std::unique_ptr<ChunkMesh> mesh;
};

// what a worker thread hands back -- pure CPU data, no GL objects. newChunk is only set
// for a fresh generate (nullptr means "this was a remesh, the chunk's blocks didn't change").
struct ChunkBuildResult
{
    ChunkCoord coord;
    std::shared_ptr<Chunk> newChunk;
    std::unique_ptr<ChunkMeshData> meshData;
};

class World : public IVoxelWorld
{
public:
    World(unsigned long seed);

    void update(glm::vec3 playerPos, float dt);
    void regenerate();

    void breakBlock(glm::vec3 wPos);
    void placeBlock(uint16_t block, glm::vec3 wPos);

    uint16_t getBlock(glm::vec3 wPos) const override;
    bool isBlockSolid(glm::vec3 wPos) const override;

    std::vector<ChunkMesh *> getChunkMeshes() const override;
    std::vector<Chunk *> getChunks() const override;

private:
    static constexpr int RENDER_DISTANCE = 12;
    static constexpr int LOAD_DISTANCE = RENDER_DISTANCE + 1;

    unsigned long m_seed;

    std::optional<glm::ivec3> getLocalPos(glm::vec3 wPos) const;

    //! ========== MAIN THREAD ONLY ==========
    ChunkCoord m_playerCoord{};
    std::unordered_map<ChunkCoord, ChunkData> m_chunks;

    // one in-flight future per coord currently being generated/remeshed -- also doubles
    // as the "already scheduled, don't submit it again" check in update().
    std::unordered_map<ChunkCoord, std::future<ChunkBuildResult>> m_pending;

    void scheduleGenerate(ChunkCoord coord);
    void scheduleRemesh(ChunkCoord coord);

    // snapshot of the currently-loaded cardinal neighbors of coord, as shared_ptr handles
    // (cheap refcount bump) -- safe to hand to a worker thread since a Chunk is never
    // mutated once inserted into m_chunks.
    std::array<std::shared_ptr<const Chunk>, 4> copyNeighbors(ChunkCoord coord) const;
    //!========================================

    BS::light_thread_pool m_pool;
};
