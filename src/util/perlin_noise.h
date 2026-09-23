/**
 * @file perlin_noise.h
 * @brief Seeded 2D Perlin noise with fractal (fBm) octaves.
 */

#pragma once

/**
 * @brief Seeded 2D Perlin noise, summed over several octaves (fractal Brownian motion).
 *
 * Each octave multiplies the frequency by the lacunarity and the amplitude by the gain.
 * The sum is normalized, so the output always stays in [-1, 1].
 */
class PerlinNoise
{
public:
    /**
     * @brief Creates a noise with a single octave and the default settings.
     *
     * @param seed seed of the gradient hash; the same seed always gives the same noise
     */
    PerlinNoise(const unsigned int seed);

    /**
     * @brief Samples the noise at a 2D position (typically world X/Z coordinates).
     *
     * @returns the noise value (always in [-1, 1])
     */
    float sample(float x, float z);

    /**
     * @brief Changes the fractal parameters.
     *
     * @param octaves number of times we sample the noise to add more detail
     * @param gain amplitude gain between octaves (default = 0.5)
     * @param lacunarity change in frequency between octaves (default = 2.0)
     * @param frequency frequency of the first octave, in 1 / world units (default = 0.02)
     */
    void updateSettings(unsigned int octaves, float gain, float lacunarity, float frequency);

    /**
     * @brief Writes a grayscale preview of the noise, with [-1, 1] remapped to [0, 255].
     *
     * @param data output buffer of at least `width * height` bytes (one byte per pixel)
     * @param width image width in pixels
     * @param height image height in pixels
     * @param worldSpan how many world blocks the image covers (image resolution stays
     * width x height pixels -- each pixel just represents worldSpan/width blocks instead
     * of exactly 1)
     */
    void generateImage(unsigned char *data, int width, int height, float worldSpan);

    /**
     * @brief Changes the seed of the gradient hash, keeping the fractal settings.
     */
    void setSeed(unsigned int seed) { _seed = seed; }

private:
    unsigned int _seed;
    unsigned int _octaves = 1;
    float _gain = 0.5f;
    float _lacunarity = 2.0f;
    float _freq = 0.02f;

    float noise(float x, float y);

    unsigned int hash(int x, int z, const unsigned int seed);
    unsigned int squirrel3_hash(int x, const unsigned int seed);

    float lerp(float a, float b, float frac);
    float smoothstep(float t);
};