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
    // 1px border duplicated around each tile so mipmap generation blends a tile with itself
    // at its edges instead of bleeding into the neighboring tile (atlas cells are packed with
    // no gap otherwise). must match vertex.glsl's CELL_STRIDE_UV/CELL_PADDING_UV/CELL_CONTENT_UV.
    static constexpr int PADDING = 1;
    static constexpr int CELL_STRIDE = TEXTURE_SIZE + 2 * PADDING;
    static constexpr int ATLAS_COLUMNS = 64;
    static constexpr int ATLAS_SIZE = ATLAS_COLUMNS * CELL_STRIDE;

    std::unordered_map<std::string, uint16_t> m_nameToIndex;

    unsigned int m_textureID;

    BlockTextureAtlas();
    BlockTextureAtlas(BlockTextureAtlas &registry) = delete;
    BlockTextureAtlas &operator=(const BlockTextureAtlas &) = delete;
};