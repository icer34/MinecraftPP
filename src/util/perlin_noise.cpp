#include "perlin_noise.h"

#include <array>
#include <cmath>
#include <glm/glm.hpp>

float PerlinNoise::sample(float x, float y)
{
    int cellX = std::floor(x);
    int cellZ = std::floor(y);

    std::array<glm::ivec2, 4> cellPoints{
        glm::ivec2{cellX, cellZ},         // bot left
        glm::ivec2{cellX, cellZ + 1},     // top left
        glm::ivec2{cellX + 1, cellZ + 1}, // top right
        glm::ivec2{cellX + 1, cellZ},     // bot right
    };

    std::array<glm::vec2, 4> gradients{};
    std::array<glm::vec2, 4> directions{};
    std::array<float, 4> dotProd{};
    for (size_t i = 0; i < 4; i++)
    {
        //* generate random gradient for each cell point
        auto point = cellPoints[i];
        unsigned int h = hash(point.x, point.y, m_seed);
        float r = (h & 0x7fffffff) / (float)0x80000000;

        float a = 2.0f * (float)M_PI * r;
        gradients[i] = glm::vec2{cos(a), sin(a)};

        //* compute the direction from the cell point to the sampled point
        directions[i] = glm::vec2{x - cellPoints[i].x, y - cellPoints[i].y};

        //* compute the dot product of the direction with the random gradient
        dotProd[i] = glm::dot(gradients[i], directions[i]);
    }

    //* biliniear interpolation of the 4 dot products values
    return lerp(lerp(dotProd[0], dotProd[3], smoothstep(x - cellX)),
                lerp(dotProd[1], dotProd[2], smoothstep(x - cellX)),
                smoothstep(y - cellZ));
}

unsigned int PerlinNoise::hash(int x, int z, const unsigned int seed)
{
    unsigned int firstHash = squirrel3_hash(z, seed);
    return squirrel3_hash(x, firstHash);
}

unsigned int PerlinNoise::squirrel3_hash(int x, const unsigned int seed)
{
    const unsigned int BIT_NOISE_1 = 0xB5297A4D;
    const unsigned int BIT_NOISE_2 = 0x68561969;
    const unsigned int BIT_NOISE_3 = 0x56B3A953;

    unsigned int mangled = x;
    mangled *= BIT_NOISE_1;
    mangled += seed;
    mangled ^= (mangled >> 8);
    mangled += BIT_NOISE_2;
    mangled ^= (mangled << 8);
    mangled *= BIT_NOISE_3;
    mangled ^= (mangled >> 8);
    return mangled;
}

float PerlinNoise::lerp(float a, float b, float frac) { return a + frac * (b - a); }

float PerlinNoise::smoothstep(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }