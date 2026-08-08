#pragma once

#include "terrain_generator.h"

// engine-side fallback used when a game doesn't supply its own ITerrainGenerator -- chunks
// are already all-EMPTY_BLOCK_ID by construction (see Chunk's constructor), so there's
// nothing to fill in here.
class DefaultTerrainGenerator : public ITerrainGenerator
{
public:
    void generateChunk(Chunk &chunk) const override {}
};
