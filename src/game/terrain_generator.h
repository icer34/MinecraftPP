/**
 * @file terrain_generator.h
 * @brief Noise-based terrain generation.
 */

#pragma once

#include "chunk.h"
#include "util/perlin_noise.h"
#include "util/spline.h"

#include <vector>

/**
 * @brief A noise and its display name, for generic iteration (e.g. in the settings UI).
 */
struct NamedNoise
{
    const char *name;   ///< Display name.
    PerlinNoise *noise; ///< Non-owning pointer to the noise, owned by TerrainGenerator.
};

/**
 * @brief A spline and its display name, for generic iteration (e.g. in the settings UI).
 */
struct NamedSpline
{
    const char *name; ///< Display name.
    Spline *spline;   ///< Non-owning pointer to the spline, owned by TerrainGenerator.
};

/**
 * @brief Fills chunks with terrain (singleton).
 *
 * The surface height of each column comes from three noises, each remapped by its own
 * spline:
 * - continentalness gives the base height;
 * - peaks & valleys gives a second, more rugged height;
 * - erosion (in [0, 1]) blends between the two: 0 keeps the base height, 1 fully applies
 *   peaks & valleys.
 *
 * Temperature and humidity noises are stored per column and only used for block tinting.
 * The noises and splines are exposed so they can be tweaked at runtime.
 *
 * generateChunk() is called from the World's worker threads.
 */
class TerrainGenerator
{
public:
    /**
     * @brief Returns the unique generator instance.
     */
    static TerrainGenerator &instance()
    {
        static TerrainGenerator terrainGenerator;
        return terrainGenerator;
    }

    /** @brief Maps the continentalness noise to the base terrain height. */
    Spline &getContinentalnessSpline() { return _continentalnessSpline; }
    /** @brief Maps the erosion noise to a [0, 1] blend factor. */
    Spline &getErosionSpline() { return _erosionSpline; }
    /** @brief Maps the peaks & valleys noise to a terrain height. */
    Spline &getPvSpline() { return _pvSpline; }

    /** @brief Continentalness noise (large-scale land / ocean shape). */
    PerlinNoise &getContinentalnessNoise() { return _continentalnessNoise; }
    /** @brief Erosion noise (how flat or jagged the terrain is). */
    PerlinNoise &getErosionNoise() { return _erosionNoise; }
    /** @brief Peaks & valleys noise (local height variation). */
    PerlinNoise &getPvNoise() { return _pvNoise; }
    /** @brief Temperature noise, used for block tinting. */
    PerlinNoise &getTemperatureNoise() { return _temperatureNoise; }
    /** @brief Humidity noise, used for block tinting. */
    PerlinNoise &getHumidityNoise() { return _humidityNoise; }

    // grouped view over all noises/splines above, for code that just needs to iterate
    // (e.g. the settings UI) without caring about which one is which
    /** @brief All the noises above, with their display names. */
    const std::vector<NamedNoise> &getNoises() const { return _noises; }
    /** @brief All the splines above, with their display names. */
    const std::vector<NamedSpline> &getSplines() const { return _splines; }

    /**
     * @brief Fills an empty chunk with terrain.
     *
     * Each column gets grass at its surface height and dirt below it. Columns below sea level
     * are filled with water up to sea level. The temperature and humidity of each column are
     * stored in the chunk.
     *
     * @param chunk the chunk to fill; its coordinates decide which part of the world it is
     */
    void generateChunk(Chunk &chunk);

    /**
     * @brief Sets the world seed and reseeds every noise from it.
     *
     * Only affects chunks generated afterwards: call World::regenerate() to apply it to the
     * chunks that are already loaded.
     */
    void setSeed(unsigned int seed);

private:
    unsigned int _seed = 67;
    unsigned int _seaLvl = 102;

    Spline _pvSpline;
    Spline _erosionSpline;
    Spline _continentalnessSpline;

    PerlinNoise _erosionNoise;
    PerlinNoise _pvNoise;
    PerlinNoise _continentalnessNoise;
    PerlinNoise _temperatureNoise;
    PerlinNoise _humidityNoise;

    std::vector<NamedNoise> _noises;
    std::vector<NamedSpline> _splines;

    int getHeight(int worldX, int worldZ);

    TerrainGenerator();
    TerrainGenerator(TerrainGenerator &registry) = delete;
    TerrainGenerator &operator=(const TerrainGenerator &) = delete;
};