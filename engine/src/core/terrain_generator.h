#pragma once

#include "chunk.h"

class ITerrainGenerator
{
public:
    virtual ~ITerrainGenerator() = default;
    virtual void generateChunk(Chunk &chunk) const = 0;
};