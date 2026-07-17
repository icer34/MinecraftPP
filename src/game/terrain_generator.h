#pragma once

#include "chunk.h"
#include "util/perlin_noise.h"
#include "util/spline.h"

#include <vector>

struct NamedNoise
{
    const char *name;
    PerlinNoise *noise;
};

struct NamedSpline
{
    const char *name;
    Spline *spline;
};

class TerrainGenerator
{
  public:
    static TerrainGenerator &instance()
    {
        static TerrainGenerator terrainGenerator;
        return terrainGenerator;
    }

    Spline &getContinentalnessSpline() { return m_continentalnessSpline; }
    Spline &getErosionSpline() { return m_erosionSpline; }
    Spline &getPvSpline() { return m_pvSpline; }

    PerlinNoise &getContinentalnessNoise() { return m_continentalnessNoise; }
    PerlinNoise &getErosionNoise() { return m_erosionNoise; }
    PerlinNoise &getPvNoise() { return m_pvNoise; }
    PerlinNoise &getTemperatureNoise() { return m_temperatureNoise; }
    PerlinNoise &getHumidityNoise() { return m_humidityNoise; }

    // grouped view over all noises/splines above, for code that just needs to iterate
    // (e.g. the settings UI) without caring about which one is which
    const std::vector<NamedNoise> &getNoises() const { return m_noises; }
    const std::vector<NamedSpline> &getSplines() const { return m_splines; }

    void generateChunk(Chunk &chunk);

    void setSeed(unsigned int seed) { m_seed = seed; }

  private:
    unsigned int m_seed = 67;
    unsigned int m_seaLvl = 102;

    Spline m_pvSpline;
    Spline m_erosionSpline;
    Spline m_continentalnessSpline;

    PerlinNoise m_erosionNoise;
    PerlinNoise m_pvNoise;
    PerlinNoise m_continentalnessNoise;
    PerlinNoise m_temperatureNoise;
    PerlinNoise m_humidityNoise;

    std::vector<NamedNoise> m_noises;
    std::vector<NamedSpline> m_splines;

    int getHeight(int worldX, int worldZ);

    TerrainGenerator();
    TerrainGenerator(TerrainGenerator &registry) = delete;
    TerrainGenerator &operator=(const TerrainGenerator &) = delete;
};