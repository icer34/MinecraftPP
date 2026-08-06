#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>

class Chunk;
class ChunkMesh;

// minimal read surface any voxel world needs to expose to engine code (entities,
// raycasting, rendering) -- lets those stay engine-side without depending on the
// concrete World class, which owns game-specific concerns (generation, streaming).
class IVoxelWorld
{
public:
    virtual ~IVoxelWorld() = default;

    virtual uint16_t getBlock(glm::vec3 wPos) const = 0;
    virtual bool isBlockSolid(glm::vec3 wPos) const = 0;

    virtual std::vector<ChunkMesh *> getChunkMeshes() const = 0;
    virtual std::vector<Chunk *> getChunks() const = 0;
};
