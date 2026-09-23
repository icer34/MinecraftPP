#include "terrain_generator.h"

#include "settings_registry.h"

#include <algorithm>

TerrainGenerator::TerrainGenerator()
    : _pvSpline(Spline(-1.0, 1.0, 0.0, (float)Chunk::HEIGHT)),
      // erosion is a 0..1 factor (how much PV is allowed to sculpt the terrain)
      _erosionSpline(Spline(-1.0, 1.0, 0.0, 1.0)),
      _continentalnessSpline(Spline(-1.0, 1.0, 0.0, (float)Chunk::HEIGHT)),
      _continentalnessNoise(PerlinNoise(_seed)),
      _erosionNoise(PerlinNoise(_seed + 1)),
      _pvNoise(PerlinNoise(_seed + 2)),
      _temperatureNoise(PerlinNoise(_seed + 3)),
      _humidityNoise(PerlinNoise(_seed + 4))
{
    _noises = {
        {"Continentalness", &_continentalnessNoise},
        {"Erosion", &_erosionNoise},
        {"Peaks & Valleys", &_pvNoise},
        {"Temperature", &_temperatureNoise},
        {"Humidity", &_humidityNoise},
    };

    _splines = {
        {"Continentalness", &_continentalnessSpline},
        {"Erosion", &_erosionSpline},
        {"Peaks & Valleys", &_pvSpline},
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

    setPoints(_continentalnessSpline,
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

    setPoints(_erosionSpline,
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

    setPoints(_pvSpline,
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

    _continentalnessNoise.updateSettings(3, 0.5, 2.0, 0.002);
    _erosionNoise.updateSettings(3, 0.5, 2.0, 0.003);
    _pvNoise.updateSettings(5, 0.5, 2.0, 0.01);
    _temperatureNoise.updateSettings(2, 0.5, 2.0, 0.0015);
    _humidityNoise.updateSettings(2, 0.5, 2.0, 0.0015);

    auto &reg = SettingsRegistry::instance();
    reg.addSpline(
        SettingCategory::WorldGen, "", "Continentalness spline", &_continentalnessSpline, "TODO");
    reg.addSpline(SettingCategory::WorldGen, "", "Erosion spline", &_erosionSpline, "TODO");
    reg.addSpline(SettingCategory::WorldGen, "", "Peaks & Valleys spline", &_pvSpline, "TODO");
}

void TerrainGenerator::setSeed(unsigned int seed)
{
    _seed = seed;

    // same per-noise offsets as in the constructor, so a given seed always gives the same world
    _continentalnessNoise.setSeed(_seed);
    _erosionNoise.setSeed(_seed + 1);
    _pvNoise.setSeed(_seed + 2);
    _temperatureNoise.setSeed(_seed + 3);
    _humidityNoise.setSeed(_seed + 4);
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

            chunk.setTemp(std::abs(_temperatureNoise.sample(worldX, worldZ) * 255.0),
                          glm::ivec2(x, z));
            chunk.setHumidity(std::abs(_humidityNoise.sample(worldX, worldZ) * 255.0),
                              glm::ivec2(x, z));

            for (int y = 0; y < Chunk::HEIGHT; y++)
            {
                if (y > height && y > _seaLvl)
                    continue;

                else if (y > height && y <= _seaLvl)
                    chunk.setBlock(Blocks::WATER, glm::ivec3(x, y, z));

                else if (y == height)
                    chunk.setBlock(Blocks::GRASS, glm::ivec3(x, y, z));

                else
                    chunk.setBlock(Blocks::DIRT, glm::ivec3(x, y, z));
            }
        }
    }
}

int TerrainGenerator::getHeight(int worldX, int worldZ)
{
    float contNoise = _continentalnessNoise.sample(worldX, worldZ);
    float pvNoise = _pvNoise.sample(worldX, worldZ);
    float erosionNoise = _erosionNoise.sample(worldX, worldZ);

    float baseHeight = _continentalnessSpline.get(contNoise);
    float pvHeight = _pvSpline.get(pvNoise);
    float erosionFactor = _erosionSpline.get(erosionNoise); // 0 = flat/eroded, 1 = full jaggedness

    // erosion controls how much peaks & valleys is allowed to pull the terrain away from the
    // continentalness base height, instead of being averaged in independently -- otherwise 3
    // independent noises averaged together regress to the mean and the terrain never reaches
    // the extremes any single spline could produce on its own
    float height = baseHeight + erosionFactor * (pvHeight - baseHeight);

    return static_cast<int>(floor(height));
}
