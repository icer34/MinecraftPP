#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

class World;

// convention: ID 0 is always the empty/air block, regardless of which game registers it
// first -- lets generic engine code (meshing, raycasting, chunk storage) check for "no
// block here" without depending on the game's specific block roster.
constexpr uint16_t EMPTY_BLOCK_ID = 0;

struct TextureLayer
{
    uint16_t textureIndex;
    bool tinted;
};

struct FaceTexture
{
    static constexpr int MAX_LAYERS = 2;
    std::array<TextureLayer, 2> layers{};
    unsigned int count = 0;

    void add(const uint16_t &idx, bool tinted = false)
    {
        layers[count++] = TextureLayer{idx, tinted};
    }
};

struct BlockType
{
    uint16_t id = 0;
    std::string name;
    bool isSolid = true;
    bool isTransparent = false;
    bool isLiquid = false;

    std::array<FaceTexture, 6> textures{};

    // takes the world and the wPos of the block that is broken / placed
    std::function<void(World &, glm::ivec3)> onBreak = [](World &, glm::ivec3) {};
    std::function<void(World &, glm::ivec3)> onPlace = [](World &, glm::ivec3) {};

    // sounds, drops, ...
    //   ...
};

class BlockRegistry
{
public:
    static BlockRegistry &instance()
    {
        static BlockRegistry registry;
        return registry;
    }

    uint16_t registerBlock(BlockType type)
    {
        type.id = static_cast<uint16_t>(m_types.size());
        m_nameToId[type.name] = type.id;
        m_types.push_back(std::move(type));
        return m_types.back().id;
    }

    BlockType &get(uint16_t id) { return m_types[id]; }

    BlockType &get(const std::string &name) { return m_types[getIdByName(name)]; }

    uint16_t getIdByName(const std::string &name) { return m_nameToId.at(name); }

private:
    BlockRegistry() = default;
    BlockRegistry(BlockRegistry &registry) = delete;
    BlockRegistry &operator=(const BlockRegistry &) = delete;

    std::vector<BlockType> m_types; // index in the vector == block ID
    std::unordered_map<std::string, uint16_t> m_nameToId;
};