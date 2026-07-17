#pragma once

#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

#include "chunk.h"
#include "graphics/mesh/chunk_mesh.h"
#include "graphics/mesh/chunk_mesher.h"
#include "terrain_generator.h"
#include "util/thread_pool.h"

struct ChunkBuildResult
{
    ChunkCoord coord;
    std::unique_ptr<Chunk> chunk; // null => this is a remesh, don't touch m_chunks
    std::unique_ptr<ChunkMeshData> meshData;
};

class World
{
  public:
    World(unsigned long seed);

    void update(glm::vec3 playerPos, float dt);
    void regenerate();

    std::vector<ChunkMesh *> getChunkMeshes() const;
    std::vector<Chunk *> getChunks() const;

  private:
    static constexpr int RENDER_DISTANCE = 20;
    static constexpr int LOAD_DISTANCE = RENDER_DISTANCE + 1;

    unsigned long m_seed;

    //! ========== MAIN THREAD ONLY ==========
    ChunkCoord m_playerCoord{};
    std::unordered_map<ChunkCoord, std::shared_ptr<Chunk>> m_chunks;
    std::unordered_map<ChunkCoord, std::unique_ptr<ChunkMesh>> m_meshes;
    std::vector<ChunkCoord> m_pending;

    // schedule generation of a brand new chunk at coord.
    void scheduleGenerate(ChunkCoord coord);

    // schedule a remesh of an already-loaded chunk (e.g. a neighbor just
    // appeared/changed). The chunk's own data is untouched, only its ChunkMesh is rebuilt.
    void scheduleRemesh(ChunkCoord coord);

    // snapshot of the currently-loaded cardinal neighbors of coord.
    // shared_ptr handles (cheap refcount bump), never raw pointers into m_chunks -- safe to
    // hand to a worker thread since the underlying Chunk is never mutated once inserted.
    std::array<std::shared_ptr<const Chunk>, 4> copyNeighbors(ChunkCoord coord) const;
    //!========================================

    std::mutex m_mutex;
    std::queue<ChunkBuildResult> m_results; //! only access the queue with the mutex

    ThreadPool m_threadPool;
};
