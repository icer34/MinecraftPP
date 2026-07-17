#include "terrain_generator.h"

#include <algorithm>

TerrainGenerator::TerrainGenerator()
    : m_pvSpline(Spline(-1.0, 1.0, 0.0, (float)Chunk::HEIGHT)),
      // erosion is a 0..1 factor (how much PV is allowed to sculpt the terrain)
      m_erosionSpline(Spline(-1.0, 1.0, 0.0, 1.0)),
      m_continentalnessSpline(Spline(-1.0, 1.0, 0.0, (float)Chunk::HEIGHT)),
      m_continentalnessNoise(PerlinNoise(m_seed)),
      m_erosionNoise(PerlinNoise(m_seed + 1)),
      m_pvNoise(PerlinNoise(m_seed + 2)),
      m_temperatureNoise(PerlinNoise(m_seed + 3)),
      m_humidityNoise(PerlinNoise(m_seed + 4))
{
    m_noises = {
        {"Continentalness", &m_continentalnessNoise},
        {"Erosion", &m_erosionNoise},
        {"Peaks & Valleys", &m_pvNoise},
        {"Temperature", &m_temperatureNoise},
        {"Humidity", &m_humidityNoise},
    };

    m_splines = {
        {"Continentalness", &m_continentalnessSpline},
        {"Erosion", &m_erosionSpline},
        {"Peaks & Valleys", &m_pvSpline},
    };

    auto setPoints = [](Spline &spline, std::initializer_list<std::pair<float, float>> points)
    {
        // the Spline constructor already added 2 default points at (xMin, mid) and (xMax, mid)
        // -- clear them so we can lay down the exact curve below
        spline.removePoint(0);
        spline.removePoint(0);

        for (auto &[x, y] : points)
            spline.addPoint(x, y);
    };

    setPoints(m_continentalnessSpline,
              {
                  {-1.0f, 248.0f},
                  {-0.9f, 10.0f},
                  {-0.25f, 10.0f},
                  {-0.125f, 90.0f},
                  {0.15f, 120.0f},
                  {0.3f, 175.0f},
                  {0.59f, 220.0f},
                  {1.0f, 250.0f},
              });

    setPoints(m_erosionSpline,
              {
                  {-1.0f, 1.0f},
                  {-0.75f, 0.78f},
                  {-0.5f, 0.63f},
                  {-0.35f, 0.75f},
                  {-0.1f, 0.15f},
                  {0.35f, 0.15f},
                  {0.5f, 0.44f},
                  {0.65f, 0.43f},
                  {0.85f, 0.02f},
                  {1.0f, 0.0f},
              });

    setPoints(m_pvSpline,
              {
                  {-1.0f, 5.0f},
                  {-0.75f, 40.0f},
                  {-0.5f, 78.0f},
                  {-0.3f, 93.0f},
                  {-0.1f, 105.0f},
                  {0.12f, 161.0f},
                  {0.35f, 195.0f},
                  {0.6f, 220.0f},
                  {0.8f, 240.0f},
                  {1.0f, 248.0f},
              });

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

            for (int y = 0; y < Chunk::HEIGHT; y++)
            {
                if (y > height && y > m_seaLvl)
                    continue;

                else if (y > height && y <= m_seaLvl)
                    chunk.setBlock(Blocks::STONE, x, y, z);

                else if (y == height)
                    chunk.setBlock(Blocks::GRASS, x, y, z);

                else
                    chunk.setBlock(Blocks::DIRT, x, y, z);
            }
        }
    }
}

int TerrainGenerator::getHeight(int worldX, int worldZ)
{
    float contNoise = m_continentalnessNoise.sample(worldX, worldZ);
    float pvNoise = m_pvNoise.sample(worldX, worldZ);
    float erosionNoise = m_erosionNoise.sample(worldX, worldZ);

    float baseHeight = m_continentalnessSpline.get(contNoise);
    float pvHeight = m_pvSpline.get(pvNoise);
    float erosionFactor = m_erosionSpline.get(erosionNoise); // 0 = flat/eroded, 1 = full jaggedness

    // erosion controls how much peaks & valleys is allowed to pull the terrain away from the
    // continentalness base height, instead of being averaged in independently -- otherwise 3
    // independent noises averaged together regress to the mean and the terrain never reaches
    // the extremes any single spline could produce on its own
    float height = baseHeight + erosionFactor * (pvHeight - baseHeight);

    return static_cast<int>(floor(height));
}
