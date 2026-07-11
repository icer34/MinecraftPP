#include "world.h"

#include <algorithm>

World::World()
    : m_threadPool(ThreadPool(10)),
      m_chunkMesher(ChunkMesher())
{
}

void World::update(glm::vec3 playerPos, float dt)
{
    ChunkCoord playerCoord{(int)floor(playerPos.x / Chunk::SIZE),
                           (int)floor(playerPos.z / Chunk::SIZE)};

    const auto &chunks = getChunks();

    //* unload chunks if needed
    for (const auto &chunk : chunks)
    {
        ChunkCoord coord = chunk->getCoords();
        int dx = abs(coord.x - playerCoord.x);
        int dz = abs(coord.z - playerCoord.z);
        if (dx > RENDER_DISTANCE || dz > RENDER_DISTANCE)
        {
            m_chunks.erase(coord);
            m_meshes.erase(coord);
        }
    }

    //* load needed chunks
    for (int r = 0; r <= RENDER_DISTANCE; r++)
    {
        for (int dx = -r; dx <= r; dx++)
        {
            for (int dz = -r; dz <= r; dz++)
            {
                if (abs(dx) != r && abs(dz) != r)
                    continue;

                ChunkCoord cc{playerCoord.x + dx, playerCoord.z + dz};

                if (m_chunks.contains(cc))
                    continue;

                bool isPending =
                    std::find(m_pending.begin(), m_pending.end(), cc) != m_pending.end();
                if (!isPending)
                {
                    m_pending.push_back(cc);
                }
                else
                {
                    continue;
                }

                m_threadPool.enqueue(
                    [this, cc]
                    {
                        ChunkBuildResult result = generateChunk(cc);
                        {
                            std::unique_lock<std::mutex> lock(m_mutex);
                            m_results.push(std::move(result));
                        }
                    });
            }
        }
    }

    //* build the meshes from the chunk build results
    std::vector<ChunkBuildResult> results;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (!m_results.empty())
        {
            results.push_back(std::move(m_results.front()));
            m_results.pop();
        }
    }

    for (auto &res : results)
    {
        ChunkCoord coord = res.chunk->getCoords();
        auto mesh = std::make_unique<ChunkMesh>(coord);
        mesh->updateSolid(res.meshData->solidData);
        mesh->updateWater(res.meshData->waterData);

        m_chunks[coord] = std::move(res.chunk);
        m_meshes[coord] = std::move(mesh);

        m_pending.erase(std::remove(m_pending.begin(), m_pending.end(), coord), m_pending.end());
    }
}

std::vector<Chunk *> World::getChunks() const
{
    std::vector<Chunk *> chunks;
    chunks.reserve(m_chunks.size());

    for (const auto &[coord, chunk] : m_chunks)
    {
        chunks.push_back(chunk.get());
    }

    return chunks;
}

std::vector<ChunkMesh *> World::getChunkMeshes() const
{
    std::vector<ChunkMesh *> meshes;
    meshes.reserve(m_meshes.size());

    for (const auto &[coord, mesh] : m_meshes)
    {
        meshes.push_back(mesh.get());
    }

    return meshes;
}

ChunkBuildResult World::generateChunk(ChunkCoord coord)
{
    auto chunk = std::make_unique<Chunk>(coord);

    constexpr int MAX_HEIGHT = 64;

    for (int x = 0; x < Chunk::SIZE; x++)
    {
        for (int z = 0; z < Chunk::SIZE; z++)
        {
            for (int y = 0; y < Chunk::HEIGHT; y++)
            {
                if (y < 64)
                {
                    chunk->setBlock(Blocks::DIRT, x, y, z);
                }
            }
        }
    }

    auto meshData = std::make_unique<ChunkMeshData>();
    std::array<const Chunk *, 4> neighbors{};
    neighbors.fill(nullptr);
    m_chunkMesher.mesh(*chunk, neighbors, *meshData);

    return ChunkBuildResult{std::move(chunk), std::move(meshData)};
}
