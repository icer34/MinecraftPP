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
    static constexpr int MIPMAP_LEVELS = 5;
    // must match vertex.glsl's CELL_STRIDE_UV/CELL_PADDING_UV/CELL_CONTENT_UV.
    static constexpr int PADDING = 1 << (MIPMAP_LEVELS - 1);
    static constexpr int CELL_STRIDE = TEXTURE_SIZE + 2 * PADDING;
    static constexpr int ATLAS_COLUMNS = 64;
    static constexpr int ATLAS_SIZE = ATLAS_COLUMNS * CELL_STRIDE;

    std::unordered_map<std::string, uint16_t> m_nameToIndex;

    unsigned int m_textureID;

    BlockTextureAtlas();
    BlockTextureAtlas(BlockTextureAtlas &registry) = delete;
    BlockTextureAtlas &operator=(const BlockTextureAtlas &) = delete;
};
/**
 * @brief downsamoples an texture of size WxW to a new texture of size (W/2)x(W/2)
 * where each pixel is the average of the 4 corresponding pixels in the starting texture.
 *
 * @param buff raw texture buffered data (straight from stbi_load)
 */
std::vector<unsigned char> downsample(const std::vector<unsigned char> &src, int w, int h);