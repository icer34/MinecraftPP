#include "chunk_mesher.h"

#include "game/block_registry.h"
#include "graphics/block_texture_atlas.h"

#include <glm/glm.hpp>

//* which corner of the texture (in the atlas cell) each vertex of a face maps to.
//* the vertex shader turns this into an actual UV offset from the cell origin --
//* nothing CPU-side ever deals with a raw UV value.
enum class Corner : uint8_t
{
    BOTTOM_LEFT = 0,
    TOP_LEFT = 1,
    TOP_RIGHT = 2,
    BOTTOM_RIGHT = 3,
};

static constexpr std::array<std::array<Corner, 4>, 6> CUBE_FACE_CORNERS = {{
    {{Corner::BOTTOM_LEFT, Corner::TOP_LEFT, Corner::TOP_RIGHT, Corner::BOTTOM_RIGHT}}, // NORTH
    {{Corner::BOTTOM_LEFT, Corner::BOTTOM_RIGHT, Corner::TOP_RIGHT, Corner::TOP_LEFT}}, // SOUTH
    {{Corner::BOTTOM_LEFT, Corner::TOP_LEFT, Corner::TOP_RIGHT, Corner::BOTTOM_RIGHT}}, // EAST
    {{Corner::BOTTOM_LEFT, Corner::BOTTOM_RIGHT, Corner::TOP_RIGHT, Corner::TOP_LEFT}}, // WEST
    {{Corner::BOTTOM_LEFT, Corner::TOP_LEFT, Corner::TOP_RIGHT, Corner::BOTTOM_RIGHT}}, // TOP
    {{Corner::BOTTOM_LEFT, Corner::BOTTOM_RIGHT, Corner::TOP_RIGHT, Corner::TOP_LEFT}}, // BOTTOM
}};

//! neighbor chunks must be in order north, south, east, west, top, bottom
void ChunkMesher::mesh(const Chunk &chunk,
                       std::array<const Chunk *, 4> neighbors,
                       ChunkMeshData &meshData)
{
    auto &registry = BlockRegistry::instance();
    auto &atlas = BlockTextureAtlas::instance();

    std::vector<uint32_t> solidVert{};
    std::vector<uint32_t> waterVert{};
    std::vector<unsigned int> solidIdx{};
    std::vector<unsigned int> waterIdx{};

    for (int x = 0; x < Chunk::SIZE; x++)
    {
        for (int z = 0; z < Chunk::SIZE; z++)
        {
            uint8_t temperature = chunk.getTemp(x, z);
            uint8_t humidity = chunk.getHumidity(x, z);

            for (int y = 0; y < Chunk::HEIGHT; y++)
            {
                auto blockID = chunk.getBlock(x, y, z);
                auto &block = registry.get(blockID);

                if (blockID == Blocks::AIR)
                    continue;

                for (Direction dir : ALL_DIRECTIONS)
                {
                    glm::ivec3 offset = getDirectionVector(dir);
                    glm::ivec3 neighborPos = glm::ivec3(x, y, z) + offset;

                    auto neighborChunkDir = posInChunk(neighborPos.x, neighborPos.y, neighborPos.z);
                    uint16_t neighborBlockID = Blocks::AIR;
                    if (neighborPos.y < 0 || neighborPos.y >= Chunk::HEIGHT)
                    {
                        // neighbor is out of bouds -> we treat it as air
                    }
                    else if (neighborChunkDir.has_value())
                    {
                        // neighbor is in another chunk
                        // convert coords into new chunk local coords
                        neighborPos = wrapCoords(neighborPos.x, neighborPos.y, neighborPos.z);
                        auto neighborChunk
                            = neighbors.at(static_cast<size_t>(neighborChunkDir.value()));

                        if (neighborChunk != nullptr)
                        {
                            neighborBlockID = neighborChunk->getBlock(
                                neighborPos.x, neighborPos.y, neighborPos.z);
                        }
                    }
                    else
                    {
                        neighborBlockID
                            = chunk.getBlock(neighborPos.x, neighborPos.y, neighborPos.z);
                    }

                    auto neighborBlock = registry.get(neighborBlockID);

                    // cull useless faces
                    if (block.isLiquid && neighborBlockID != Blocks::AIR)
                        continue;
                    if (block.isSolid && neighborBlock.isSolid)
                        continue;

                    // add the face to the vectors
                    if (block.isSolid)
                        addFace(block,
                                humidity,
                                temperature,
                                dir,
                                glm::ivec3(x, y, z),
                                solidVert,
                                solidIdx);

                    if (block.isLiquid)
                        addFace(block,
                                humidity,
                                temperature,
                                dir,
                                glm::ivec3(x, y, z),
                                waterVert,
                                waterIdx);
                }
            }
        }
    }

    meshData.solidData = MeshData{std::move(solidVert), std::move(solidIdx)};
    meshData.waterData = MeshData{std::move(waterVert), std::move(waterIdx)};
}
/**
 * the vertex data is packed in 2 32-bit integers in the following way:
 *? chunkX - chunkY - chunkZ - normalIdx - textureIdx - cornerIdx --> 4 - 7 - 4 - 3 - 12 - 2 = 32
 *? humidity - temperature - isTinted - future use --> 8 - 8 - 1 - 15 = 32
 * - the final position of a vertex is computed in the vertex shader: (chunkPos + facePos)
 * modelMatrix where the model matrix shifts the vertex to the correct world coords
 */
void ChunkMesher::addFace(const BlockType &block,
                          uint8_t humidity,
                          uint8_t temperature,
                          Direction dir,
                          glm::ivec3 localPos,
                          std::vector<uint32_t> &vert,
                          std::vector<unsigned int> &indices)
{
    auto &faceCorners = CUBE_FACE_CORNERS[static_cast<size_t>(dir)];
    uint8_t normalIdx = static_cast<uint>(dir);

    const FaceTexture &face = block.textures[static_cast<size_t>(dir)];
    for (uint8_t layerIdx = 0; layerIdx < face.count; layerIdx++)
    {
        const uint16_t &textureIdx = face.layers[layerIdx].textureIndex;
        bool isTinted = face.layers[layerIdx].tinted;
        unsigned int addedFaces
            = static_cast<unsigned int>(vert.size() / 2); //* 2 = number of uint32 per vertex

        for (int i = 0; i < 4; i++)
        {
            uint8_t cornerIdx = static_cast<uint8_t>(faceCorners[i]);

            uint32_t data1 = 0;
            data1 |= (static_cast<uint32_t>(localPos.x) & 0xF) << 28;
            data1 |= (static_cast<uint32_t>(localPos.y) & 0x7F) << 21;
            data1 |= (static_cast<uint32_t>(localPos.z) & 0xF) << 17;
            data1 |= (normalIdx & 0x7) << 14;
            data1 |= (textureIdx & 0xFFF) << 2;
            data1 |= (cornerIdx & 0x3);

            uint32_t data2 = 0;
            data2 |= (humidity & 0xFF) << 24;
            data2 |= (temperature & 0xFF) << 16;
            data2 |= (static_cast<uint8_t>(isTinted) & 0x1) << 15;

            vert.push_back(data1);
            vert.push_back(data2);
        }

        indices.insert(indices.end(),
                       {addedFaces,
                        addedFaces + 1,
                        addedFaces + 2,
                        addedFaces,
                        addedFaces + 2,
                        addedFaces + 3});
    }
}

std::optional<Direction> ChunkMesher::posInChunk(int x, int y, int z)
{
    if (x >= Chunk::SIZE)
        return Direction::EAST;
    else if (x < 0)
        return Direction::WEST;

    if (z >= Chunk::SIZE)
        return Direction::SOUTH;
    else if (z < 0)
        return Direction::NORTH;

    return std::nullopt;
}

glm::ivec3 ChunkMesher::wrapCoords(int x, int y, int z)
{
    int newX = ((x % Chunk::SIZE) + Chunk::SIZE) % Chunk::SIZE;
    int newZ = ((z % Chunk::SIZE) + Chunk::SIZE) % Chunk::SIZE;

    return glm::ivec3(newX, y, newZ);
}