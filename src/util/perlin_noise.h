#pragma once

class PerlinNoise
{
  public:
    /**
     * @param octaves number of times we sample the noise to add more detail
     * @param gain amplitude gain between octaves (default = 0.5)
     * @param lacunarity change in frequency between octaves (default = 2.0)
     */
    PerlinNoise(const unsigned int seed);

    /**
     * @returns the noise value (always in [-1, 1])
     */
    float sample(float x, float z);
    void updateSettings(unsigned int octaves, float gain, float lacunarity, float frequency);
    /**
     * @param worldSpan how many world blocks the image covers (image resolution stays
     * width x height pixels -- each pixel just represents worldSpan/width blocks instead
     * of exactly 1)
     */
    void generateImage(unsigned char *data, int width, int height, float worldSpan);

  private:
    unsigned int m_seed;
    unsigned int m_octaves = 1;
    float m_gain = 0.5f;
    float m_lacunarity = 2.0f;
    float m_freq = 0.02f;

    float noise(float x, float y);

    unsigned int hash(int x, int z, const unsigned int seed);
    unsigned int squirrel3_hash(int x, const unsigned int seed);

    float lerp(float a, float b, float frac);
    float smoothstep(float t);
};