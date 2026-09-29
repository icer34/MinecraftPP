/**
 * @file biome_registry.h
 * @brief Biome definitions and their global registry (work in progress).
 */

#pragma once

#include <cstdint>
#include <string>

/**
 * @brief Description of a biome.
 */
struct Biome
{
    uint16_t id = 0;  ///< Biome ID, assigned by the registry.
    std::string name; ///< Unique, human-readable name.
};

/**
 * @brief Global registry of all biomes (singleton).
 *
 */
class BiomeRegistry
{
public:
    /**
     * @brief Returns the unique registry instance.
     */
    static BiomeRegistry &instance()
    {
        static BiomeRegistry reg;
        return reg;
    }

    /**
     * @brief Registers a biome and assigns it an ID.
     *
     * @param biome biome to register (its `id` field is ignored)
     * @return the ID assigned to the biome
     */
    uint16_t registerBiome(Biome biome) { return 0; }
};