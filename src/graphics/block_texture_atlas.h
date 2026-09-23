/**
 * @file block_texture_atlas.h
 * @brief Texture atlas holding every block texture, with hand-built mipmaps.
 */

#pragma once

#include <string>
#include <unordered_map>

#include "util/directions.h"

/**
 * @brief Single GL texture containing every block texture, arranged in a grid (singleton).
 *
 * Each texture is stored in a cell surrounded by padding, so that mipmapping does not bleed
 * neighboring textures into each other. Mip levels are built manually with downsample().
 * Blocks refer to their textures by index in the grid (see getIndex()).
 *
 * The GL texture is created on first access to instance(), so that first access must happen
 * while a GL context is current.
 */
class BlockTextureAtlas
{
public:
    /**
     * @brief Returns the unique atlas instance.
     */
    static BlockTextureAtlas &instance()
    {
        static BlockTextureAtlas atlas;
        return atlas;
    }

    /**
     * @brief Loads every `.png` file of `assets/textures/block` into the atlas, along with its
     * mip levels.
     *
     * Must be called once, before registering blocks and before any getIndex() call.
     */
    void loadAllTextures();

    /**
     * @brief Returns the atlas index of a texture.
     *
     * @param fileName file name of the texture, without folder or extension (e.g. `"dirt"`)
     * @throws std::out_of_range if no texture with that name was loaded
     */
    uint16_t getIndex(const std::string &fileName) const;

    /**
     * @brief OpenGL name of the atlas texture.
     */
    unsigned int getID() const;

private:
    static constexpr int TEXTURE_SIZE = 16;
    static constexpr int MIPMAP_LEVELS = 5;
    // must match vertex.glsl's CELL_STRIDE_UV/CELL_PADDING_UV/CELL_CONTENT_UV.
    static constexpr int PADDING = 1 << (MIPMAP_LEVELS - 1);
    static constexpr int CELL_STRIDE = TEXTURE_SIZE + 2 * PADDING;
    static constexpr int ATLAS_COLUMNS = 64;
    static constexpr int ATLAS_SIZE = ATLAS_COLUMNS * CELL_STRIDE;

    std::unordered_map<std::string, uint16_t> _nameToIndex;

    unsigned int _textureID;

    BlockTextureAtlas();
    BlockTextureAtlas(BlockTextureAtlas &registry) = delete;
    BlockTextureAtlas &operator=(const BlockTextureAtlas &) = delete;
};
/**
 * @brief Downsamples an RGBA texture of size WxH to a new texture of size (W/2)x(H/2),
 * where each pixel is the average of the 4 corresponding pixels in the source texture.
 *
 * @param src raw RGBA texture data (straight from stbi_load)
 * @param w width of the source texture in pixels
 * @param h height of the source texture in pixels
 * @return the downsampled RGBA data
 */
std::vector<unsigned char> downsample(const std::vector<unsigned char> &src, int w, int h);