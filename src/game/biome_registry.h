#pragma once

#include <cstdint>
#include <string>

struct Biome
{
    uint16_t id = 0;
    std::string name;
};

class BiomeRegistry
{
  public:
    static BiomeRegistry &instance()
    {
        static BiomeRegistry reg;
        return reg;
    }

    uint16_t registerBiome(Biome biome) {}
};