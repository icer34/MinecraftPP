#include "chunk_mesher.h"

#include "game/block_registry.h"
#include "graphics/block_texture_atlas.h"

#include <glm/glm.hpp>

// (u_sign, v_sign) of each corner along a face's tangent axes -- same order/meaning
// as the Corner enum, and the same for every face direction (only the tangent axes
// themselves, picked in getTangentAxes(), differ per direction).
static constexpr std::array<glm::ivec2, 4> CORNER_SIGNS = {{
    {-1, -1}, // BOTTOM_LEFT
    {-1, 1},  // TOP_LEFT
    {1, 1},   // TOP_RIGHT
    {1, -1},  // BOTTOM_RIGHT
}};

// the two axes that span a face's plane (perpendicular to its normal), used to walk
// towards a corner from the face's center when sampling AO neighbors.
static void getTangentAxes(Direction dir, glm::ivec3 &uAxis, glm::ivec3 &vAxis)
{
    switch (dir)
    {
    case Direction::NORTH:
    case Direction::SOUTH:
        uAxis = {1, 0, 0};
        vAxis = {0, 1, 0};
        break;
    case Direction::EAST:
    case Direction::WEST:
        uAxis = {0, 0, 1};
        vAxis = {0, 1, 0};
        break;
    case Direction::TOP:
    case Direction::BOTTOM:
        uAxis = {1, 0, 0};
        vAxis = {0, 0, 1};
        break;
    }
}

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
                                chunk,
                                neighbors,
                                solidVert,
                                solidIdx);

                    if (block.isLiquid)
                        addFace(block,
                                humidity,
                                temperature,
                                dir,
                                glm::ivec3(x, y, z),
                                chunk,
                                neighbors,
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
 *? chunkX - chunkY - chunkZ - normalIdx - textureIdx - isTinted --> 4 - 8 - 4 - 3 - 12 - 1 = 32
 ** humidity - temperature - cornerIdx - ambientOcclusion - future use --> 8 - 8 - 2 - 2 - 12 = 32
 * - the final position of a vertex is computed in the vertex shader: (chunkPos + facePos)
 * modelMatrix where the model matrix shifts the vertex to the correct world coords
 */
void ChunkMesher::addFace(const BlockType &block,
                          uint8_t humidity,
                          uint8_t temperature,
                          Direction dir,
                          glm::ivec3 localPos,
                          const Chunk &chunk,
                          std::array<const Chunk *, 4> neighbors,
                          std::vector<uint32_t> &vert,
                          std::vector<unsigned int> &indices)
{
    auto &faceCorners = CUBE_FACE_CORNERS[static_cast<size_t>(dir)];
    uint8_t normalIdx = static_cast<unsigned int>(dir);

    const FaceTexture &face = block.textures[static_cast<size_t>(dir)];
    for (uint8_t layerIdx = 0; layerIdx < face.count; layerIdx++)
    {
        const uint16_t &textureIdx = face.layers[layerIdx].textureIndex;
        bool isTinted = face.layers[layerIdx].tinted;
        unsigned int addedFaces
            = static_cast<unsigned int>(vert.size() / 2); //* 2 = number of uint32 per vertex

        for (int i = 0; i < 4; i++)
        {
            Corner corner = faceCorners[i];
            uint8_t cornerIdx = static_cast<uint8_t>(corner);
            uint8_t aoValue = cornerAO(chunk, neighbors, localPos, dir, corner);

            uint32_t data1 = 0;
            data1 |= (static_cast<uint32_t>(localPos.x) & 0xF) << 28;
            data1 |= (static_cast<uint32_t>(localPos.y) & 0xFF) << 20;
            data1 |= (static_cast<uint32_t>(localPos.z) & 0xF) << 16;
            data1 |= (normalIdx & 0x7) << 13;
            data1 |= (textureIdx & 0xFFF) << 1;
            data1 |= (static_cast<uint8_t>(isTinted) & 0x1);

            uint32_t data2 = 0;
            data2 |= (humidity & 0xFF) << 24;
            data2 |= (temperature & 0xFF) << 16;
            data2 |= (cornerIdx & 0x3) << 14;
            data2 |= (aoValue & 0x3) << 12;

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

bool ChunkMesher::isSolidNeighbor(glm::ivec3 pos,
                                  const Chunk &chunk,
                                  std::array<const Chunk *, 4> neighbors)
{
    if (pos.y < 0 || pos.y >= Chunk::HEIGHT)
        return false; // above/below the world -> air

    bool outX = pos.x < 0 || pos.x >= Chunk::SIZE;
    bool outZ = pos.z < 0 || pos.z >= Chunk::SIZE;

    if (outX && outZ)
        return false; // diagonal chunk neighbor, not tracked -- treat as air (see chunk_mesher.h)

    uint16_t blockID;
    if (!outX && !outZ)
    {
        blockID = chunk.getBlock(pos.x, pos.y, pos.z);
    }
    else
    {
        auto neighborChunkDir = posInChunk(pos.x, pos.y, pos.z);
        glm::ivec3 wrapped = wrapCoords(pos.x, pos.y, pos.z);
        const Chunk *neighborChunk = neighbors.at(static_cast<size_t>(neighborChunkDir.value()));

        if (neighborChunk == nullptr)
            return false;

        blockID = neighborChunk->getBlock(wrapped.x, wrapped.y, wrapped.z);
    }

    return BlockRegistry::instance().get(blockID).isSolid;
}

uint8_t ChunkMesher::cornerAO(const Chunk &chunk,
                              std::array<const Chunk *, 4> neighbors,
                              glm::ivec3 localPos,
                              Direction dir,
                              Corner corner)
{
    glm::ivec3 normalOffset = getDirectionVector(dir);

    glm::ivec3 uAxis, vAxis;
    getTangentAxes(dir, uAxis, vAxis);

    glm::ivec2 signs = CORNER_SIGNS[static_cast<size_t>(corner)];
    glm::ivec3 tangentOffset = signs.x * uAxis + signs.y * vAxis;

    bool side1 = isSolidNeighbor(localPos + normalOffset + signs.x * uAxis, chunk, neighbors);
    bool side2 = isSolidNeighbor(localPos + normalOffset + signs.y * vAxis, chunk, neighbors);
    bool cornerSolid = isSolidNeighbor(localPos + normalOffset + tangentOffset, chunk, neighbors);

    if (side1 && side2)
        return 3; // fully occluded regardless of the diagonal, avoids lighting seams

    return static_cast<uint8_t>(side1) + static_cast<uint8_t>(side2)
         + static_cast<uint8_t>(cornerSolid);
}