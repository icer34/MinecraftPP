#pragma once

#include <array>
#include <cstdint>

#include "block_registry.h"
#include "graphics/block_texture_atlas.h"
#include "util/directions.h"

namespace
{
inline FaceTexture singleLayer(const uint16_t &index, bool tinted = false)
{
    FaceTexture face;
    face.add(index, tinted);
    return face;
}

inline std::array<FaceTexture, 6> uniform(const uint16_t &index)
{
    FaceTexture face = singleLayer(index);
    return {face, face, face, face, face, face};
}
} // namespace

namespace Blocks
{
inline uint16_t AIR;
inline uint16_t STONE;
inline uint16_t DIRT;
inline uint16_t GRASS;

// add a new block here: declare its id above, then register it below.
inline void registerAll()
{
    auto &reg = BlockRegistry::instance();
    auto &atlas = BlockTextureAtlas::instance();

    AIR = reg.registerBlock({
        .name = "air",
        .isSolid = false,
        .isTransparent = true,
    });

    BlockType grass;
    FaceTexture side = singleLayer(atlas.getIndex("dirt"));
    side.add(atlas.getIndex("grass_block_side_overlay"), true);
    grass.name = "grass_block";
    grass.textures[(size_t)Direction::NORTH] = side;
    grass.textures[(size_t)Direction::SOUTH] = side;
    grass.textures[(size_t)Direction::EAST] = side;
    grass.textures[(size_t)Direction::WEST] = side;
    grass.textures[(size_t)Direction::TOP] = singleLayer(atlas.getIndex("grass_block_top"), true);
    grass.textures[(size_t)Direction::BOTTOM] = singleLayer(atlas.getIndex("dirt"));
    GRASS = reg.registerBlock(grass);

    STONE = reg.registerBlock({
        .name = "stone",
        .textures = uniform(atlas.getIndex("stone")),
    });

    DIRT = reg.registerBlock({
        .name = "dirt",
        .textures = uniform(atlas.getIndex("dirt")),
    });
}
} // namespace Blocks
