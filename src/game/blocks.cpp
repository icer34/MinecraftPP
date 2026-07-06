#include "blocks.h"
#include "block_registry.h"

namespace Blocks {
    uint16_t AIR;
    uint16_t STONE;

    void registerAll()
    {
        auto& reg = BlockRegistry::instance();
        AIR = reg.registerBlock({
            .name = "air",
            .isSolid = false,
            .isTransparent = true,
        });
        
        STONE = reg.registerBlock({
            .name = "stone",
        });
    }
}