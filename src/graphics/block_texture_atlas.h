#pragma once

#include <string>
#include <unordered_map>

#include "util/directions.h"

class BlockTextureAtlas
{
  public:
    static BlockTextureAtlas &instance()
    {
        static BlockTextureAtlas atlas;
        return atlas;
    }

    void loadAllTextures();

    uint16_t getIndex(const std::string &fileName) const;
    unsigned int getID() const;

  private:
    static constexpr int TEXTURE_SIZE = 16;
    static constexpr int ATLAS_SIZE = 1024;

    std::unordered_map<std::string, uint16_t> m_nameToIndex;

    unsigned int m_textureID;

    BlockTextureAtlas();
    BlockTextureAtlas(BlockTextureAtlas &registry) = delete;
    BlockTextureAtlas &operator=(const BlockTextureAtlas &) = delete;
};