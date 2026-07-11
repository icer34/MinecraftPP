#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <vector>

#include "chunk.h"
#include "graphics/chunk_mesh.h"
#include "graphics/chunk_mesher.h"
#include "util/thread_pool.h"

struct ChunkBuildResult
{
    std::unique_ptr<Chunk> chunk;
    std::unique_ptr<ChunkMeshData> meshData;
};

class World
{
  public:
    World();

    ChunkBuildResult generateChunk(ChunkCoord coord);
    void update(glm::vec3 playerPos, float dt);
    std::vector<ChunkMesh *> getChunkMeshes() const;
    std::vector<Chunk *> getChunks() const;

  private:
    static constexpr int RENDER_DISTANCE = 12;

    //! MAIN THREAD ONLY
    std::unordered_map<ChunkCoord, std::unique_ptr<Chunk>> m_chunks;
    std::unordered_map<ChunkCoord, std::unique_ptr<ChunkMesh>> m_meshes;
    std::vector<ChunkCoord> m_pending;

    //! critical part: only access the queue with mutex
    std::mutex m_mutex;
    std::queue<ChunkBuildResult> m_results;

    ThreadPool m_threadPool;

    ChunkMesher m_chunkMesher;
};