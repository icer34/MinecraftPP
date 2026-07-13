#pragma once

#include "chunk.h"
#include "util/perlin_noise.h"

class TerrainGenerator
{
  public:
    static TerrainGenerator &instance()
    {
        static TerrainGenerator terrainGenerator;
        return terrainGenerator;
    }

    void generateChunk(Chunk &chunk);

    void setSeed(unsigned int seed) { m_seed = seed; }

  private:
    unsigned int m_seed = 67;

    TerrainGenerator() = default;
    TerrainGenerator(TerrainGenerator &registry) = delete;
    TerrainGenerator &operator=(const TerrainGenerator &) = delete;
};