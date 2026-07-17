#include "blocks.h"

#include "block_registry.h"
#include "graphics/block_texture_atlas.h"
#include "util/directions.h"

namespace
{
FaceTexture singleLayer(const uint16_t &index, bool tinted = false)
{
    FaceTexture face;
    face.add(index, tinted);
    return face;
}

std::array<FaceTexture, 6> uniform(const uint16_t &index)
{
    FaceTexture face = singleLayer(index);
    return {face, face, face, face, face, face};
}
} // namespace

namespace Blocks
{
uint16_t AIR;
uint16_t STONE;
uint16_t DIRT;
uint16_t GRASS;

void registerAll()
{
    auto &reg = BlockRegistry::instance();
    auto &atlas = BlockTextureAtlas::instance();

    AIR = reg.registerBlock({
        .name = "air",
        .isSolid = false,
        .isTransparent = true,
    });

    STONE = reg.registerBlock({
        .name = "stone",
        .textures = uniform(atlas.getIndex("stone")),
    });

    DIRT = reg.registerBlock({
        .name = "dirt",
        .textures = uniform(atlas.getIndex("dirt")),
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
}
} // namespace Blocks
