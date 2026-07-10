#include "chunk_mesher.h"

#include "game/block_registry.h"
#include "graphics/texture_atlas.h"

#include <glm/glm.hpp>

//* vertices of a unit cube (0..1), grouped per face, CCW winding as seen from outside the cube.
//* indexed in the same order as the Direction enum (directions.h), so CUBE_FACE_VERTICES[(size_t)dir] works.
//* each face is triangulated as (0,1,2) + (0,2,3).
static constexpr std::array<std::array<glm::vec3, 4>, 6> CUBE_FACE_VERTICES = {{
    {{ {0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {1, 0, 0} }}, // NORTH (-z)
    {{ {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1} }}, // SOUTH (+z)
    {{ {1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1} }}, // EAST  (+x)
    {{ {0, 0, 0}, {0, 0, 1}, {0, 1, 1}, {0, 1, 0} }}, // WEST  (-x)
    {{ {0, 1, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 0} }}, // TOP   (+y)
    {{ {0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1} }}, // BOTTOM(-y)
}};

//* indices of UV coords in the FaceTextures of the blocks
static constexpr std::array<std::array<glm::vec2, 4>, 6> CUBE_FACE_UVS = {{
    {{ {0, 0}, {0, 1}, {1, 1}, {1, 0} }}, // NORTH
    {{ {0, 0}, {1, 0}, {1, 1}, {0, 1} }}, // SOUTH
    {{ {0, 0}, {0, 1}, {1, 1}, {1, 0} }}, // EAST
    {{ {0, 0}, {1, 0}, {1, 1}, {0, 1} }}, // WEST
    {{ {0, 0}, {0, 1}, {1, 1}, {1, 0} }}, // TOP
    {{ {0, 0}, {1, 0}, {1, 1}, {0, 1} }}, // BOTTOM
}};

//! neighbor chunks must be in order north, south, east, west, top, bottom
void ChunkMesher::mesh(const Chunk &chunk, std::array<const Chunk *, 6> neighbors, ChunkMesh &mesh)
{
    auto& registry = BlockRegistry::instance();
    auto& atlas = TextureAtlas::instance();

    std::vector<float> solidVert{};
    std::vector<float> waterVert{};
    std::vector<unsigned int> solidIdx{};
    std::vector<unsigned int> waterIdx{};

    for (int x = 0; x < Chunk::SIZE; x++)
    {
        for (int y = 0; y < Chunk::SIZE; y++)
        {
            for (int z = 0; z < Chunk::SIZE; z++)
            {
                auto blockID = chunk.getBlock(x, y, z);
                auto& block = registry.get(blockID);

                if (blockID == Blocks::AIR)
                    continue;

                for (Direction dir : ALL_DIRECTIONS)
                {
                    glm::ivec3 offset = getDirectionVector(dir);
                    glm::ivec3 neighborPos = glm::ivec3(x, y, z) + offset;

                    auto neighborChunkDir = posInChunk(neighborPos.x, neighborPos.y, neighborPos.z);
                    uint16_t neighborBlockID = Blocks::AIR;
                    if (neighborChunkDir.has_value())
                    {
                        //neighbor is in another chunk
                        //convert coords into new chunk local coords
                        neighborPos = wrapCoords(neighborPos.x, neighborPos.y, neighborPos.z);
                        auto neighborChunk = neighbors.at(static_cast<size_t>(neighborChunkDir.value()));

                        if (neighborChunk != nullptr)
                        {
                            neighborBlockID = neighborChunk->getBlock(neighborPos.x, neighborPos.y, neighborPos.z);
                        }

                    } else {
                        neighborBlockID = chunk.getBlock(neighborPos.x, neighborPos.y, neighborPos.z);
                    }

                    auto neighborBlock = registry.get(neighborBlockID);

                    //cull useless faces
                    if (block.isLiquid && neighborBlockID != Blocks::AIR) continue;
                    if (block.isSolid && neighborBlock.isSolid) continue;

                    //add the face to the vectors
                    if (block.isSolid) addFace(block, dir, glm::ivec3(x, y, z), solidVert, solidIdx);
                    if (block.isLiquid) addFace(block, dir, glm::ivec3(x, y, z), waterVert, waterIdx);
                }
            }
        }
    }

    mesh.updateSolid(MeshData{std::move(solidVert), std::move(solidIdx)});
    mesh.updateWater(MeshData{std::move(waterVert), std::move(waterIdx)});
}


//TODO: add another quad if the block is multi textured -> add it with a lil offset to avoid z-fighting
void ChunkMesher::addFace(const BlockType& block, Direction dir, glm::ivec3 localPos, 
                          std::vector<float>& vert, std::vector<unsigned int>& indices)
{
    auto normal = getDirectionVector(dir);
    auto& facePos = CUBE_FACE_VERTICES[static_cast<size_t>(dir)];
    auto& faceUVs = CUBE_FACE_UVS[static_cast<size_t>(dir)];
    unsigned int addedFaces = static_cast<unsigned int>(vert.size() / 8); //* 8 = number of floats per vertex

    const UVRect& rect = block.textures[static_cast<size_t>(dir)].layers[0].uv;

    for (int i = 0; i < 4; i++)
    {
        //position
        glm::vec3 pos = glm::vec3(localPos) + facePos[i]; //shift the vertex pos by the position in the chunk
        vert.push_back(pos.x);
        vert.push_back(pos.y);
        vert.push_back(pos.z);

        //normal
        vert.push_back(normal.x);
        vert.push_back(normal.y);
        vert.push_back(normal.z);

        //uv
        vert.push_back(faceUVs[i].x == 0 ? rect.x0 : rect.x1);
        vert.push_back(faceUVs[i].y == 0 ? rect.y0 : rect.y1);
    }

    indices.insert(indices.end(), {addedFaces, addedFaces + 1, addedFaces + 2, addedFaces, addedFaces + 2, addedFaces + 3});
}

std::optional<Direction> ChunkMesher::posInChunk(int x, int y, int z)
{
    if (x >= Chunk::SIZE)
        return Direction::EAST;
    else if (x < 0)
        return Direction::WEST;

    if (y >= Chunk::SIZE)
        return Direction::TOP;
    else if (y < 0)
        return Direction::BOTTOM;

    if (z >= Chunk::SIZE)
        return Direction::SOUTH;
    else if (z < 0)
        return Direction::NORTH;

    return std::nullopt;
}

glm::ivec3 ChunkMesher::wrapCoords(int x, int y, int z)
{
    int newX = ((x % Chunk::SIZE) + Chunk::SIZE) % Chunk::SIZE;
    int newY = ((y % Chunk::SIZE) + Chunk::SIZE) % Chunk::SIZE;
    int newZ = ((z % Chunk::SIZE) + Chunk::SIZE) % Chunk::SIZE;

    return glm::ivec3(newX, newY, newZ);
}