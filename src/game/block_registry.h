#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "util/uv_rect.h"

struct TextureLayer
{
    UVRect uv;
    bool tinted;
};

struct FaceTexture
{
    static constexpr int MAX_LAYERS = 2;
    std::array<TextureLayer, 2> layers{};
    uint count = 0;

    void add(const UVRect &uv, bool tinted = false) { layers[count++] = TextureLayer{uv, tinted}; }
};

struct BlockType
{
    uint16_t id = 0;
    std::string name;
    bool isSolid = true;
    bool isTransparent = false;
    bool isLiquid = false;

    std::array<FaceTexture, 6> textures{};

    // std::function onBreak()

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