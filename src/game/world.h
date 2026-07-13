#pragma once

#include <array>
#include <glm/glm.hpp>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <unordered_map>
#include <vector>

#include "chunk.h"
#include "graphics/chunk_mesh.h"
#include "graphics/chunk_mesher.h"
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
    World();

    void update(glm::vec3 playerPos, float dt);
    std::vector<ChunkMesh *> getChunkMeshes() const;
    std::vector<Chunk *> getChunks() const;

  private:
    static constexpr int RENDER_DISTANCE = 12;
    static constexpr int LOAD_DISTANCE = RENDER_DISTANCE + 1;

    //! ========== MAIN THREAD ONLY ==========
    ChunkCoord m_playerCoord{};
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>> m_chunks;
    std::unordered_map<ChunkCoord, std::unique_ptr<ChunkMesh>> m_meshes;
    std::vector<ChunkCoord> m_pending;

    // schedule generation of a brand new chunk at coord.
    void scheduleGenerate(ChunkCoord coord);

    // schedule a remesh of an already-loaded chunk (e.g. a neighbor just
    // appeared/changed). The chunk's own data is untouched, only its ChunkMesh is rebuilt.
    void scheduleRemesh(ChunkCoord coord);

    // snapshot of the currently-loaded cardinal neighbors of coord.
    // Returns copies (never pointers into m_chunks) so they're safe to hand to a worker thread.
    std::array<std::optional<Chunk>, 4> copyNeighbors(ChunkCoord coord) const;
    //!========================================

    std::mutex m_mutex;
    std::queue<ChunkBuildResult> m_results; //! only access the queue with the mutex

    ThreadPool m_threadPool;
};
