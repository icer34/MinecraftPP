#include "world.h"

#include <algorithm>
#include <unordered_set>

#include "util/directions.h"

namespace
{
//! Runs on a worker thread. Never touches World's shared maps -- everything it needs
//! (the chunk's own previous data, if any, and its neighbors) is passed in by value.
ChunkBuildResult buildChunk(ChunkCoord coord,
                            std::optional<Chunk> existingChunk,
                            std::array<std::optional<Chunk>, 4> neighborCopies)
{
    bool isNew = !existingChunk.has_value();
    auto chunk =
        isNew ? std::make_unique<Chunk>(coord) : std::make_unique<Chunk>(std::move(*existingChunk));

    if (isNew)
    {
        auto &terrainGenerator = TerrainGenerator::instance();
        terrainGenerator.generateChunk(*chunk);
    }

    std::array<const Chunk *, 4> neighborPtrs{};
    for (size_t i = 0; i < neighborPtrs.size(); i++)
    {
        neighborPtrs[i] = neighborCopies[i].has_value() ? &neighborCopies[i].value() : nullptr;
    }

    auto meshData = std::make_unique<ChunkMeshData>();
    ChunkMesher mesher;
    mesher.mesh(*chunk, neighborPtrs, *meshData);

    return ChunkBuildResult{coord, isNew ? std::move(chunk) : nullptr, std::move(meshData)};
}
} // namespace

World::World()
    : m_threadPool(ThreadPool(10))
{
}

void World::update(glm::vec3 playerPos, float dt)
{
    m_playerCoord =
        ChunkCoord{(int)floor(playerPos.x / Chunk::SIZE), (int)floor(playerPos.z / Chunk::SIZE)};
    ChunkCoord playerCoord = m_playerCoord;

    const auto &chunks = getChunks();

    //* unload chunks if needed (only once they're out of LOAD_DISTANCE, not RENDER_DISTANCE,
    //* otherwise the buffer ring would be unloaded the frame right after it's loaded)
    for (const auto &chunk : chunks)
    {
        ChunkCoord coord = chunk->getCoords();
        int dx = abs(coord.x - playerCoord.x);
        int dz = abs(coord.z - playerCoord.z);
        if (dx > LOAD_DISTANCE || dz > LOAD_DISTANCE)
        {
            m_chunks.erase(coord);
            m_meshes.erase(coord);
        }
    }

    //* load needed chunks, one ring further than what's actually drawn (see LOAD_DISTANCE)
    for (int r = 0; r <= LOAD_DISTANCE; r++)
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
                if (isPending)
                    continue;

                m_pending.push_back(cc);
                scheduleGenerate(cc);
            }
        }
    }

    //* drain the results queue
    std::vector<ChunkBuildResult> results;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        while (!m_results.empty())
        {
            results.push_back(std::move(m_results.front()));
            m_results.pop();
        }
    }

    //* pass 1: insert/update everything from this batch
    std::vector<bool> isNewFlags;
    isNewFlags.reserve(results.size());

    for (auto &res : results)
    {
        bool isNew = res.chunk != nullptr;
        isNewFlags.push_back(isNew);

        if (isNew)
        {
            m_chunks[res.coord] = std::move(res.chunk);
            m_pending.erase(std::remove(m_pending.begin(), m_pending.end(), res.coord),
                            m_pending.end());
        }

        auto mesh = std::make_unique<ChunkMesh>(res.coord);
        mesh->updateSolid(res.meshData->solidData);
        mesh->updateWater(res.meshData->waterData);
        m_meshes[res.coord] = std::move(mesh);
    }

    //* pass 2: a chunk that just appeared may have made an already-loaded neighbor's mesh
    //* stale (that neighbor was meshed without knowing about it), and its own mesh may have
    //* been built before some of its neighbors existed too -- refresh everyone involved.
    //* Only fresh chunks trigger this, never remeshes, so this can't cascade forever.
    std::unordered_set<ChunkCoord> toRemesh;

    for (size_t i = 0; i < results.size(); i++)
    {
        if (!isNewFlags[i])
            continue;

        ChunkCoord coord = results[i].coord;
        toRemesh.insert(coord);

        for (Direction dir : CARDINAL_DIRECTIONS)
        {
            glm::ivec3 offset = getDirectionVector(dir);
            ChunkCoord neighborCoord{coord.x + offset.x, coord.z + offset.z};
            if (m_chunks.contains(neighborCoord))
            {
                toRemesh.insert(neighborCoord);
            }
        }
    }

    for (const auto &coord : toRemesh)
    {
        scheduleRemesh(coord);
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

    //* meshes exist for chunks up to LOAD_DISTANCE, but only draw up to RENDER_DISTANCE --
    //* the buffer ring is only there to give the edge chunks real neighbor data to cull against
    for (const auto &[coord, mesh] : m_meshes)
    {
        int dx = abs(coord.x - m_playerCoord.x);
        int dz = abs(coord.z - m_playerCoord.z);
        if (dx <= RENDER_DISTANCE && dz <= RENDER_DISTANCE)
        {
            meshes.push_back(mesh.get());
        }
    }

    return meshes;
}

std::array<std::optional<Chunk>, 4> World::copyNeighbors(ChunkCoord coord) const
{
    std::array<std::optional<Chunk>, 4> result{};

    for (size_t i = 0; i < CARDINAL_DIRECTIONS.size(); i++)
    {
        glm::ivec3 offset = getDirectionVector(CARDINAL_DIRECTIONS[i]);
        auto it = m_chunks.find(ChunkCoord{coord.x + offset.x, coord.z + offset.z});
        if (it != m_chunks.end())
        {
            result[i].emplace(*it->second);
        }
    }

    return result;
}

void World::scheduleGenerate(ChunkCoord coord)
{
    auto neighborCopies = copyNeighbors(coord);

    m_threadPool.enqueue(
        [this, coord, neighborCopies = std::move(neighborCopies)]() mutable
        {
            ChunkBuildResult result = buildChunk(coord, std::nullopt, std::move(neighborCopies));
            std::unique_lock<std::mutex> lock(m_mutex);
            m_results.push(std::move(result));
        });
}

void World::scheduleRemesh(ChunkCoord coord)
{
    auto it = m_chunks.find(coord);
    if (it == m_chunks.end())
        return;

    Chunk chunkCopy = *it->second;
    auto neighborCopies = copyNeighbors(coord);

    m_threadPool.enqueue(
        [this,
         coord,
         chunkCopy = std::move(chunkCopy),
         neighborCopies = std::move(neighborCopies)]() mutable
        {
            ChunkBuildResult result =
                buildChunk(coord, std::move(chunkCopy), std::move(neighborCopies));
            std::unique_lock<std::mutex> lock(m_mutex);
            m_results.push(std::move(result));
        });
}
