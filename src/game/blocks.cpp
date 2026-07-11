#include "blocks.h"

#include "block_registry.h"
#include "graphics/texture_atlas.h"
#include "util/directions.h"
#include "util/uv_rect.h"

namespace
{
FaceTexture singleLayer(const UVRect &uv, bool tinted = false)
{
    FaceTexture face;
    face.add(uv, tinted);
    return face;
}

std::array<FaceTexture, 6> uniform(const UVRect &uv)
{
    FaceTexture face = singleLayer(uv);
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
    auto &atlas = TextureAtlas::instance();

    AIR = reg.registerBlock({
        .name = "air",
        .isSolid = false,
        .isTransparent = true,
    });

    STONE = reg.registerBlock({
        .name = "stone",
        .textures = uniform(atlas.getUV("stone")),
    });

    DIRT = reg.registerBlock({
        .name = "dirt",
        .textures = uniform(atlas.getUV("dirt")),
    });

    BlockType grass;
    FaceTexture side = singleLayer(atlas.getUV("dirt"));
    side.add(atlas.getUV("grass_block_side_overlay"), true);
    grass.name = "grass_block";
    grass.textures[(size_t)Direction::NORTH] = side;
    grass.textures[(size_t)Direction::SOUTH] = side;
    grass.textures[(size_t)Direction::EAST] = side;
    grass.textures[(size_t)Direction::WEST] = side;
    grass.textures[(size_t)Direction::TOP] = singleLayer(atlas.getUV("grass_block_top"), true);
    grass.textures[(size_t)Direction::BOTTOM] = singleLayer(atlas.getUV("dirt"));
    GRASS = reg.registerBlock(grass);
}
} // namespace Blocks
