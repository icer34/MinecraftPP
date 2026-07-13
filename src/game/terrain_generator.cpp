#include "terrain_generator.h"

#include <algorithm>

void TerrainGenerator::generateChunk(Chunk &chunk)
{
    auto noise = PerlinNoise(m_seed);

    constexpr float FREQUENCY = 0.02f; // smaller = smoother/larger hills
    constexpr int BASE_HEIGHT = 64;    // average terrain height
    constexpr int AMPLITUDE = 32;      // max variation above/below BASE_HEIGHT

    ChunkCoord coord = chunk.getCoords();

    for (int x = 0; x < Chunk::SIZE; x++)
    {
        for (int z = 0; z < Chunk::SIZE; z++)
        {
            int worldX = coord.x * Chunk::SIZE + x;
            int worldZ = coord.z * Chunk::SIZE + z;

            float n = noise.sample(worldX * FREQUENCY, worldZ * FREQUENCY);
            int height = BASE_HEIGHT + static_cast<int>(n * AMPLITUDE);
            height = std::clamp(height, 0, Chunk::HEIGHT);

            for (int y = 0; y <= height; y++)
            {
                chunk.setBlock(Blocks::DIRT, x, y, z);

                if (y == height)
                    chunk.setBlock(Blocks::GRASS, x, y, z);
            }
        }
    }
}
