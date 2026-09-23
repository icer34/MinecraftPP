#include "world.h"

#include <algorithm>
#include <chrono>
#include <unordered_set>

#include "game/block_registry.h"
#include "game/blocks.h"
#include "util/directions.h"

using glm::ivec3;
using glm::vec3;

namespace
{
//! Runs on a worker thread. Never touches World's shared maps -- everything it needs
//! (the chunk's own previous data, if any, and its neighbors) is passed in as shared_ptr
//! handles (cheap refcount bump), not copied -- a Chunk is never mutated once inserted
//! into World::_chunks, so sharing read-only ownership across threads is safe.
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
    uint8_t neighborMask = 0;
    for (size_t i = 0; i < neighborPtrs.size(); i++)
    {
        neighborPtrs[i] = neighborCopies[i].get();
        if (neighborPtrs[i])
            neighborMask |= 1 << i;
    }

    auto meshData = std::make_unique<ChunkMeshData>();
    ChunkMesher mesher;
    mesher.mesh(*chunkPtr, neighborPtrs, *meshData);

    return ChunkBuildResult{coord, std::move(newChunk), std::move(meshData), neighborMask};
}
} // namespace

World::World(unsigned long seed)
    : _seed(seed),
      _pool(10)
{
    TerrainGenerator::instance().setSeed(_seed);
}

std::array<std::shared_ptr<const Chunk>, 4> World::copyNeighbors(ChunkCoord coord) const
{
    std::array<std::shared_ptr<const Chunk>, 4> result{};

    for (size_t i = 0; i < CARDINAL_DIRECTIONS.size(); i++)
    {
        ivec3 offset = getDirectionVector(CARDINAL_DIRECTIONS[i]);
        auto it = _chunks.find(ChunkCoord{coord.x + offset.x, coord.z + offset.z});
        if (it != _chunks.end())
        {
            result[i] = it->second.chunk; // cheap: shared_ptr refcount bump, not a data copy
        }
    }

    return result;
}

uint8_t World::currentNeighborMask(ChunkCoord coord) const
{
    uint8_t mask = 0;
    for (size_t i = 0; i < CARDINAL_DIRECTIONS.size(); i++)
    {
        ivec3 offset = getDirectionVector(CARDINAL_DIRECTIONS[i]);
        if (_chunks.contains(ChunkCoord{coord.x + offset.x, coord.z + offset.z}))
            mask |= 1 << i;
    }
    return mask;
}

void World::scheduleGenerate(ChunkCoord coord)
{
    auto neighborCopies = copyNeighbors(coord);

    _pending[coord] = _pool.submit_task([coord, neighborCopies]()
                                        { return buildChunk(coord, nullptr, neighborCopies); });
}

void World::scheduleRemesh(ChunkCoord coord)
{
    auto it = _chunks.find(coord);
    if (it == _chunks.end())
        return;

    std::shared_ptr<const Chunk> existingChunk = it->second.chunk; // cheap: refcount bump
    auto neighborCopies = copyNeighbors(coord);

    _pending[coord]
        = _pool.submit_task([coord, existingChunk, neighborCopies]()
                            { return buildChunk(coord, existingChunk, neighborCopies); });
}

void World::update(vec3 playerPos, float dt)
{
    _playerCoord
        = ChunkCoord{(int)floor(playerPos.x / Chunk::SIZE), (int)floor(playerPos.z / Chunk::SIZE)};
    ChunkCoord playerCoord = _playerCoord;

    //* unload chunks out of range (LOAD_DISTANCE, not RENDER_DISTANCE -- see getChunkMeshes)
    for (auto it = _chunks.begin(); it != _chunks.end();)
    {
        ChunkCoord coord = it->first;
        int dx = abs(coord.x - playerCoord.x);
        int dz = abs(coord.z - playerCoord.z);
        if (dx > LOAD_DISTANCE || dz > LOAD_DISTANCE)
        {
            it = _chunks.erase(it);
        }
        else
        {
            ++it;
        }
    }

    //* schedule needed chunks, one ring further than what's actually drawn (see LOAD_DISTANCE).
    //* skip anything already loaded OR already in flight (_pending doubles as that check).
    for (int r = 0; r <= LOAD_DISTANCE; r++)
    {
        for (int dx = -r; dx <= r; dx++)
        {
            for (int dz = -r; dz <= r; dz++)
            {
                if (abs(dx) != r && abs(dz) != r)
                    continue;

                ChunkCoord coord{playerCoord.x + dx, playerCoord.z + dz};

                if (!_chunks.contains(coord) && !_pending.contains(coord))
                {
                    scheduleGenerate(coord);
                }
            }
        }
    }

    //* drain the pending futures that have finished, capped at MAX_MESH_UPLOADS_PER_FRAME --
    //* each one costs a GPU upload on this thread, the others just wait for the next frame.
    std::vector<ChunkBuildResult> results;
    for (auto it = _pending.begin();
         it != _pending.end() && results.size() < size_t(MAX_MESH_UPLOADS_PER_FRAME);)
    {
        if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            results.push_back(it->second.get());
            it = _pending.erase(it);
        }
        else
        {
            ++it;
        }
    }

    //* insert/update everything from this batch, and collect which chunks now hold a stale mesh
    std::unordered_set<ChunkCoord> toRemesh;

    for (auto &res : results)
    {
        // the player moved away while this was being built -- don't upload it just to have
        // the unload loop throw it away next frame
        int dx = abs(res.coord.x - playerCoord.x);
        int dz = abs(res.coord.z - playerCoord.z);
        if (dx > LOAD_DISTANCE || dz > LOAD_DISTANCE)
            continue;

        bool isNew = res.newChunk != nullptr;

        if (isNew)
        {
            auto mesh = std::make_unique<ChunkMesh>(res.coord);
            mesh->updateSolid(res.meshData->solidData);
            mesh->updateWater(res.meshData->waterData);
            _chunks[res.coord] = ChunkData{std::move(res.newChunk), std::move(mesh)};

            // the already-loaded neighbors were meshed without this chunk -- their border
            // faces are stale. only one level: their own neighbors didn't change.
            for (Direction dir : CARDINAL_DIRECTIONS)
            {
                ivec3 offset = getDirectionVector(dir);
                ChunkCoord neighborCoord{res.coord.x + offset.x, res.coord.z + offset.z};
                if (_chunks.contains(neighborCoord))
                    toRemesh.insert(neighborCoord);
            }
        }
        else
        {
            auto it = _chunks.find(res.coord);
            if (it == _chunks.end())
                continue;
            it->second.mesh->updateSolid(res.meshData->solidData);
            it->second.mesh->updateWater(res.meshData->waterData);
        }

        // a neighbor showed up while this mesh was being built -- it was meshed against
        // a missing neighbor, so rebuild it now that the neighbor is there
        if ((currentNeighborMask(res.coord) & ~res.neighborMask) != 0)
            toRemesh.insert(res.coord);
    }

    //* an in-flight remesh for one of these was snapshotted before the new neighbor existed,
    //* so replace it rather than skipping it (its stale result is simply dropped)
    for (const auto &coord : toRemesh)
    {
        scheduleRemesh(coord);
    }
}

void World::breakBlock(glm::vec3 wPos)
{
    auto localPos = getLocalPos(wPos);
    if (!localPos.has_value())
        return;

    ChunkCoord coord{(int)floor((float)wPos.x / Chunk::SIZE),
                     (int)floor((float)wPos.z / Chunk::SIZE)};

    auto it = _chunks.find(coord);
    if (it == _chunks.end())
        return;

    auto block = BlockRegistry::instance().get(getBlock(wPos));

    auto modified = std::make_shared<Chunk>(*it->second.chunk);
    modified->setBlock(Blocks::AIR, ivec3(localPos.value()));
    it->second.chunk = std::move(modified);

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

        if (_chunks.contains(neighborCoord) && !_pending.contains(neighborCoord))
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

    auto it = _chunks.find(coord);
    if (it == _chunks.end())
        return;

    auto block = BlockRegistry::instance().get(blockID);

    auto modified = std::make_shared<Chunk>(*it->second.chunk);
    modified->setBlock(blockID, ivec3(localPos.value()));
    it->second.chunk = std::move(modified);

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

        if (_chunks.contains(neighborCoord) && !_pending.contains(neighborCoord))
        {
            scheduleRemesh(neighborCoord);
        }
    }
}

void World::regenerate()
{
    // drop the in-flight futures too -- their results were built from the old chunks and
    // would otherwise be re-inserted by update(). the tasks still run to completion on the
    // pool, their results are just discarded.
    _pending.clear();
    _chunks.clear();
}

uint16_t World::getBlock(vec3 wPos) const
{
    if (wPos.y < 0 || wPos.y >= Chunk::HEIGHT)
        return Blocks::AIR;

    ChunkCoord coord{(int)floor((float)wPos.x / Chunk::SIZE),
                     (int)floor((float)wPos.z / Chunk::SIZE)};

    auto it = _chunks.find(coord);
    if (it == _chunks.end())
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

    auto it = _chunks.find(coord);
    if (it == _chunks.end())
        return true; // chunk not loaded yet -- treat as solid so the player can't fall through

    int lx = wPos.x - coord.x * Chunk::SIZE;
    int lz = wPos.z - coord.z * Chunk::SIZE;
    uint16_t blockID = it->second.chunk->getBlock(ivec3(lx, wPos.y, lz));
    return BlockRegistry::instance().get(blockID).isSolid;
}

std::vector<Chunk *> World::getChunks() const
{
    std::vector<Chunk *> chunks;
    chunks.reserve(_chunks.size());

    for (const auto &[coord, data] : _chunks)
    {
        chunks.push_back(data.chunk.get());
    }

    return chunks;
}

std::vector<ChunkMesh *> World::getChunkMeshes() const
{
    std::vector<ChunkMesh *> meshes;
    meshes.reserve(_chunks.size());

    //* meshes exist for chunks up to LOAD_DISTANCE, but only draw up to RENDER_DISTANCE --
    //* the buffer ring is only there to give the edge chunks real neighbor data to cull against
    for (const auto &[coord, data] : _chunks)
    {
        int dx = abs(coord.x - _playerCoord.x);
        int dz = abs(coord.z - _playerCoord.z);
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

    auto it = _chunks.find(coord);
    if (it == _chunks.end())
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
