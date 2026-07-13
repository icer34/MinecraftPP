#pragma once

class PerlinNoise
{
  public:
    PerlinNoise(const unsigned int seed) { m_seed = seed; }

    float sample(float x, float z);

  private:
    unsigned int m_seed;

    unsigned int hash(int x, int z, const unsigned int seed);
    unsigned int squirrel3_hash(int x, const unsigned int seed);

    float lerp(float a, float b, float frac);
    float smoothstep(float t);
};