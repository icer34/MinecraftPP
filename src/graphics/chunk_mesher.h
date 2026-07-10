#pragma once

#include "chunk_mesh.h"
#include "game/chunk.h"
#include "util/directions.h"
#include "game/block_registry.h"

#include <array>
#include <optional>

class ChunkMesher
{
public:
    void mesh(const Chunk &chunk, std::array<const Chunk *, 6> neighbors, ChunkMesh &mesh);

private:
    /**
     * @brief determines if a given position is in the boundaries of the chunk or not.
     * if not, it returns the direction of the neighbor chunk where the position is.
     *
     * @return Direction of the neighbor chunk if out of bounds, std::nullopt otherwise
     */
    std::optional<Direction> posInChunk(int x, int y, int z);
    
    /**
     * @param localPos position of the block inside the chunk (in [0, Chunk::SIZE[)
     */
    void addFace(BlockType block, Direction dir, glm::ivec3 localPos, std::vector<float>& vert, std::vector<unsigned int>& indices);

    glm::ivec3 wrapCoords(int x, int y, int z);
};