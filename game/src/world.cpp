#include "world.h"

#include <algorithm>
#include <chrono>
#include <unordered_set>

#include "core/block_registry.h"
#include "blocks.h"
#include "util/directions.h"

using glm::ivec3;
using glm::vec3;

namespace
{
//! Runs on a worker thread. Never touches World's shared maps -- everything it needs
//! (the chunk's own previous data, if any, and its neighbors) is passed in as shared_ptr
//! handles (cheap refcount bump), not copied -- a Chunk is never mutated once inserted
//! into World::m_chunks, so sharing read-only ownership across threads is safe.
ChunkBuildResult buildChunk(ChunkCoord coord,
                            std::shared_ptr<const Chunk> existingChunk,
                            std::array<std::shared_ptr<const Chunk>, 4> neighborCopies)
{
    bool isNew = existingChunk == nullptr;

    std::shared_ptr<Chunk> newChunk;
    const Chunk *chunkPtr;

    if (isNew)
    {
        newChunk = std::make_shared<Chunk>(coord);
        TerrainGenerator::instance().generateChunk(*newChunk);
        chunkPtr = newChunk.get();
    }
    else
    {
        chunkPtr = existingChunk.get();
    }

    std::array<const Chunk *, 4> neighborPtrs{};
    for (size_t i = 0; i < neighborPtrs.size(); i++)
    {
        neighborPtrs[i] = neighborCopies[i].get();
    }

    auto meshData = std::make_unique<ChunkMeshData>();
    ChunkMesher mesher;
    mesher.mesh(*chunkPtr, neighborPtrs, *meshData);

    return ChunkBuildResult{coord, std::move(newChunk), std::move(meshData)};
}
} // namespace

World::World(unsigned long seed)
    : m_seed(seed),
      m_pool(10)
{
    TerrainGenerator::instance().setSeed(m_seed);
}

std::array<std::shared_ptr<const Chunk>, 4> World::copyNeighbors(ChunkCoord coord) const
{
    std::array<std::shared_ptr<const Chunk>, 4> result{};

    for (size_t i = 0; i < CARDINAL_DIRECTIONS.size(); i++)
    {
        ivec3 offset = getDirectionVector(CARDINAL_DIRECTIONS[i]);
        auto it = m_chunks.find(ChunkCoord{coord.x + offset.x, coord.z + offset.z});
        if (it != m_chunks.end())
        {
            result[i] = it->second.chunk; // cheap: shared_ptr refcount bump, not a data copy
        }
    }

    return result;
}

void World::scheduleGenerate(ChunkCoord coord)
{
    auto neighborCopies = copyNeighbors(coord);

    m_pending[coord] = m_pool.submit_task([coord, neighborCopies]()
                                          { return buildChunk(coord, nullptr, neighborCopies); });
}

void World::scheduleRemesh(ChunkCoord coord)
{
    auto it = m_chunks.find(coord);
    if (it == m_chunks.end())
        return;

    std::shared_ptr<const Chunk> existingChunk = it->second.chunk; // cheap: refcount bump
    auto neighborCopies = copyNeighbors(coord);

    m_pending[coord]
        = m_pool.submit_task([coord, existingChunk, neighborCopies]()
                             { return buildChunk(coord, existingChunk, neighborCopies); });
}

void World::update(vec3 playerPos, float dt)
{
    m_playerCoord
        = ChunkCoord{(int)floor(playerPos.x / Chunk::SIZE), (int)floor(playerPos.z / Chunk::SIZE)};
    ChunkCoord playerCoord = m_playerCoord;

    //* unload chunks out of range (LOAD_DISTANCE, not RENDER_DISTANCE -- see getChunkMeshes)
    for (auto it = m_chunks.begin(); it != m_chunks.end();)
    {
        ChunkCoord coord = it->first;
        int dx = abs(coord.x - playerCoord.x);
        int dz = abs(coord.z - playerCoord.z);
        if (dx > LOAD_DISTANCE || dz > LOAD_DISTANCE)
        {
            it = m_chunks.erase(it);
        }
        else
        {
            ++it;
        }
    }

    //* schedule needed chunks, one ring further than what's actually drawn (see LOAD_DISTANCE).
    //* skip anything already loaded OR already in flight (m_pending doubles as that check).
    for (int r = 0; r <= LOAD_DISTANCE; r++)
    {
        for (int dx = -r; dx <= r; dx++)
        {
            for (int dz = -r; dz <= r; dz++)
            {
                if (abs(dx) != r && abs(dz) != r)
                    continue;

                ChunkCoord coord{playerCoord.x + dx, playerCoord.z + dz};

                if (!m_chunks.contains(coord) && !m_pending.contains(coord))
                {
                    scheduleGenerate(coord);
                }
            }
        }
    }

    //* drain whichever pending futures have finished this frame -- may be several at once,
    //* unlike the sequential version, since worker threads run independently of the main loop.
    std::vector<ChunkBuildResult> results;
    for (auto it = m_pending.begin(); it != m_pending.end();)
    {
        if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            results.push_back(it->second.get());
            it = m_pending.erase(it);
        }
        else
        {
            ++it;
        }
    }

    //* pass 1: insert/update everything from this batch
    std::vector<bool> isNewFlags;
    isNewFlags.reserve(results.size());

    for (auto &res : results)
    {
        bool isNew = res.newChunk != nullptr;
        isNewFlags.push_back(isNew);

        if (isNew)
        {
            auto mesh = std::make_unique<ChunkMesh>(res.coord);
            mesh->updateSolid(res.meshData->solidData);
            mesh->updateWater(res.meshData->waterData);
            m_chunks[res.coord] = ChunkData{std::move(res.newChunk), std::move(mesh)};
        }
        else
        {
            auto it = m_chunks.find(res.coord);
            if (it != m_chunks.end())
            {
                it->second.mesh->updateSolid(res.meshData->solidData);
                it->second.mesh->updateWater(res.meshData->waterData);
            }
        }
    }

    //* pass 2: a chunk that just appeared may have made an already-loaded neighbor's mesh
    //* stale (that neighbor was meshed without knowing about it), and its own mesh may have
    //* been built before some of its neighbors existed too -- refresh everyone involved.
    //* propagated to a fixpoint with a worklist: remeshing a neighbor can itself surface
    //* further neighbors that need refreshing (several new chunks landing in the same batch,
    //* chained together), so we keep draining the worklist until nothing new turns up.
    std::unordered_set<ChunkCoord> toRemesh;
    std::vector<ChunkCoord> worklist;

    for (size_t i = 0; i < results.size(); i++)
    {
        if (!isNewFlags[i])
            continue;

        ChunkCoord coord = results[i].coord;
        toRemesh.insert(coord);
        worklist.push_back(coord);
    }

    for (size_t i = 0; i < worklist.size(); i++)
    {
        ChunkCoord coord = worklist[i];

        for (Direction dir : CARDINAL_DIRECTIONS)
        {
            ivec3 offset = getDirectionVector(dir);
            ChunkCoord neighborCoord{coord.x + offset.x, coord.z + offset.z};

            // .second is true only the first time a coord is inserted -- skip anything
            // already queued/marked so we don't process the same chunk twice
            if (m_chunks.contains(neighborCoord) && toRemesh.insert(neighborCoord).second)
            {
                worklist.push_back(neighborCoord);
            }
        }
    }

    for (const auto &coord : toRemesh)
    {
        if (!m_pending.contains(coord))
        {
            scheduleRemesh(coord);
        }
    }
}

void World::breakBlock(glm::vec3 wPos)
{
    auto localPos = getLocalPos(wPos);
    if (!localPos.has_value())
        return;

    ChunkCoord coord{(int)floor((float)wPos.x / Chunk::SIZE),
                     (int)floor((float)wPos.z / Chunk::SIZE)};

    auto it = m_chunks.find(coord);
    if (it == m_chunks.end())
        return;

    auto block = BlockRegistry::instance().get(getBlock(wPos));

    it->second.chunk->setBlock(Blocks::AIR, ivec3(localPos.value()));

    block.onBreak(*this, wPos);

    scheduleRemesh(coord);

    for (Direction dir : CARDINAL_DIRECTIONS)
    {
        ivec3 offset = getDirectionVector(dir);

        // check if the remesh is needed
        ivec3 newPos = localPos.value() + offset;
        if (newPos.x >= 0 && newPos.x < Chunk::SIZE && newPos.z >= 0 && newPos.z < Chunk::SIZE)
            continue; // newPos is still in chunk --> no remesh needed for its neighbor

        ChunkCoord neighborCoord{coord.x + offset.x, coord.z + offset.z};

        if (m_chunks.contains(neighborCoord) && !m_pending.contains(neighborCoord))
        {
            scheduleRemesh(neighborCoord);
        }
    }
}

void World::placeBlock(uint16_t blockID, glm::vec3 wPos)
{
    auto localPos = getLocalPos(wPos);
    if (!localPos.has_value())
        return;

    ChunkCoord coord{(int)floor((float)wPos.x / Chunk::SIZE),
                     (int)floor((float)wPos.z / Chunk::SIZE)};

    auto it = m_chunks.find(coord);
    if (it == m_chunks.end())
        return;

    auto block = BlockRegistry::instance().get(blockID);

    it->second.chunk->setBlock(blockID, ivec3(localPos.value()));

    block.onPlace(*this, wPos);

    scheduleRemesh(coord);

    for (Direction dir : CARDINAL_DIRECTIONS)
    {
        ivec3 offset = getDirectionVector(dir);

        // check if the remesh is needed
        ivec3 newPos = localPos.value() + offset;
        if (newPos.x >= 0 && newPos.x < Chunk::SIZE && newPos.z >= 0 && newPos.z < Chunk::SIZE)
            continue; // newPos is still in chunk --> no remesh needed for its neighbor

        ChunkCoord neighborCoord{coord.x + offset.x, coord.z + offset.z};

        if (m_chunks.contains(neighborCoord) && !m_pending.contains(neighborCoord))
        {
            scheduleRemesh(neighborCoord);
        }
    }
}

void World::regenerate() { m_chunks.clear(); }

uint16_t World::getBlock(vec3 wPos) const
{
    if (wPos.y < 0 || wPos.y >= Chunk::HEIGHT)
        return Blocks::AIR;

    ChunkCoord coord{(int)floor((float)wPos.x / Chunk::SIZE),
                     (int)floor((float)wPos.z / Chunk::SIZE)};

    auto it = m_chunks.find(coord);
    if (it == m_chunks.end())
        return Blocks::AIR;

    int lx = wPos.x - coord.x * Chunk::SIZE;
    int lz = wPos.z - coord.z * Chunk::SIZE;
    return it->second.chunk->getBlock(ivec3(lx, wPos.y, lz));
}

bool World::isBlockSolid(vec3 wPos) const
{
    if (wPos.y < 0 || wPos.y >= Chunk::HEIGHT)
        return false; // above/below the world -> air

    ChunkCoord coord{(int)floor((float)wPos.x / Chunk::SIZE),
                     (int)floor((float)wPos.z / Chunk::SIZE)};

    auto it = m_chunks.find(coord);
    if (it == m_chunks.end())
        return true; // chunk not loaded yet -- treat as solid so the player can't fall through

    int lx = wPos.x - coord.x * Chunk::SIZE;
    int lz = wPos.z - coord.z * Chunk::SIZE;
    uint16_t blockID = it->second.chunk->getBlock(ivec3(lx, wPos.y, lz));
    return BlockRegistry::instance().get(blockID).isSolid;
}

std::vector<Chunk *> World::getChunks() const
{
    std::vector<Chunk *> chunks;
    chunks.reserve(m_chunks.size());

    for (const auto &[coord, data] : m_chunks)
    {
        chunks.push_back(data.chunk.get());
    }

    return chunks;
}

std::vector<ChunkMesh *> World::getChunkMeshes() const
{
    std::vector<ChunkMesh *> meshes;
    meshes.reserve(m_chunks.size());

    //* meshes exist for chunks up to LOAD_DISTANCE, but only draw up to RENDER_DISTANCE --
    //* the buffer ring is only there to give the edge chunks real neighbor data to cull against
    for (const auto &[coord, data] : m_chunks)
    {
        int dx = abs(coord.x - m_playerCoord.x);
        int dz = abs(coord.z - m_playerCoord.z);
        if (dx <= RENDER_DISTANCE && dz <= RENDER_DISTANCE)
        {
            meshes.push_back(data.mesh.get());
        }
    }

    return meshes;
}

std::optional<glm::ivec3> World::getLocalPos(glm::vec3 wPos) const
{
    if (wPos.y < 0 || wPos.y >= Chunk::HEIGHT)
        return std::nullopt;

    ChunkCoord coord{(int)floor((float)wPos.x / Chunk::SIZE),
                     (int)floor((float)wPos.z / Chunk::SIZE)};

    auto it = m_chunks.find(coord);
    if (it == m_chunks.end())
        return std::nullopt;

    int lx = wPos.x - coord.x * Chunk::SIZE;
    int lz = wPos.z - coord.z * Chunk::SIZE;

    if (lx < 0 || lx >= Chunk::SIZE || lz < 0 || lz >= Chunk::SIZE)
    {
        std::cout << "[world coords conversion error]" << std::format("%d, %d, %d", lx, wPos.y, lz)
                  << std::endl;
        return std::nullopt;
    }

    return glm::vec3(lx, wPos.y, lz);
}
