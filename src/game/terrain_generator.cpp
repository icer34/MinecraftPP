#include "terrain_generator.h"

#include <algorithm>

TerrainGenerator::TerrainGenerator()
    : m_pvSpline(Spline(-1.0, 1.0, 0.0, (float)Chunk::HEIGHT)),
      m_erosionSpline(Spline(-1.0, 1.0, 0.0, (float)Chunk::HEIGHT)),
      m_continentalnessSpline(Spline(-1.0, 1.0, 0.0, (float)Chunk::HEIGHT)),
      m_continentalnessNoise(PerlinNoise(m_seed)),
      m_erosionNoise(PerlinNoise(m_seed + 1)),
      m_pvNoise(PerlinNoise(m_seed + 2)),
      m_temperatureNoise(PerlinNoise(m_seed + 3)),
      m_humidityNoise(PerlinNoise(m_seed + 4))
{
    m_continentalnessNoise.updateSettings(3, 0.5, 2.0, 0.002);
    m_erosionNoise.updateSettings(3, 0.5, 2.0, 0.003);
    m_pvNoise.updateSettings(5, 0.5, 2.0, 0.01);
    m_temperatureNoise.updateSettings(2, 0.5, 2.0, 0.0015);
    m_humidityNoise.updateSettings(2, 0.5, 2.0, 0.0015);
}

void TerrainGenerator::generateChunk(Chunk &chunk)
{
    ChunkCoord coord = chunk.getCoords();

    for (int x = 0; x < Chunk::SIZE; x++)
    {
        for (int z = 0; z < Chunk::SIZE; z++)
        {
            int worldX = coord.x * Chunk::SIZE + x;
            int worldZ = coord.z * Chunk::SIZE + z;

            int height = getHeight(worldX, worldZ);

            chunk.setTemp(abs(m_temperatureNoise.sample(worldX, worldZ) * 255.0), x, z);
            chunk.setHumidity(abs(m_humidityNoise.sample(worldX, worldZ) * 255.0), x, z);

            for (int y = 0; y <= height; y++)
            {
                if (y == height)
                {
                    chunk.setBlock(Blocks::GRASS, x, y, z);
                }
                else
                {
                    chunk.setBlock(Blocks::DIRT, x, y, z);
                }
            }
        }
    }
}

int TerrainGenerator::getHeight(int worldX, int worldZ)
{
    float noiseVal = m_continentalnessNoise.sample(worldX, worldZ);
    return floor(m_continentalnessSpline.get(noiseVal));
}
