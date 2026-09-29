/**
 * @file block_registry.h
 * @brief Block type definitions and their global registry.
 */

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>

class World;

/**
 * @brief One texture layer of a block face.
 */
struct TextureLayer
{
    uint16_t textureIndex; ///< Index of the texture in the BlockTextureAtlas.
    bool tinted;           ///< Whether the layer is tinted by the biome colormap (e.g. grass).
};

/**
 * @brief Textures of one block face, drawn as up to MAX_LAYERS stacked layers.
 *
 * Several layers let a face combine a base texture with a tinted overlay, like the side of
 * a grass block (dirt + tinted grass overlay).
 */
struct FaceTexture
{
    static constexpr int MAX_LAYERS = 2;  ///< Maximum number of layers per face.
    std::array<TextureLayer, 2> layers{}; ///< The layers, bottom to top.
    unsigned int count = 0;               ///< Number of layers in use.

    /**
     * @brief Adds a layer on top of the existing ones.
     *
     * @param idx texture index in the BlockTextureAtlas
     * @param tinted whether the layer is tinted by the biome colormap
     * @warning No bounds check: adding more than MAX_LAYERS layers is undefined behavior.
     */
    void add(const uint16_t &idx, bool tinted = false)
    {
        layers[count++] = TextureLayer{idx, tinted};
    }
};

/**
 * @brief Definition of a kind of block: properties, textures and behavior.
 */
struct BlockType
{
    uint16_t id = 0;            ///< Block ID, assigned by BlockRegistry::registerBlock().
    std::string name;           ///< Unique name, used for lookups by name.
    bool isSolid = true;        ///< Collides with entities and hides the faces of solid neighbors.
    bool isTransparent = false; ///< Lets light / sight through (not used by the mesher yet).
    bool isLiquid = false;      ///< Meshed in the water pass instead of the opaque pass.

    /// Texture of each face, indexed by Direction.
    std::array<FaceTexture, 6> textures{};

    // takes the world and the wPos of the block that is broken / placed
    /// Called after the block is broken. Receives the world and the block's world position.
    std::function<void(World &, glm::ivec3)> onBreak = [](World &, glm::ivec3) {};
    /// Called after the block is placed. Receives the world and the block's world position.
    std::function<void(World &, glm::ivec3)> onPlace = [](World &, glm::ivec3) {};

    // sounds, drops, ...
    //   ...
};

/**
 * @brief Global registry of all block types (singleton).
 *
 * Block IDs are assigned in registration order, starting at 0, and are used as indices into
 * the registry.
 */
class BlockRegistry
{
public:
    /**
     * @brief Returns the unique registry instance.
     */
    static BlockRegistry &instance()
    {
        static BlockRegistry registry;
        return registry;
    }

    /**
     * @brief Registers a block type and assigns it the next free ID.
     *
     * @param type the block definition (its `id` field is overwritten)
     * @return the ID assigned to the block
     */
    uint16_t registerBlock(BlockType type)
    {
        type.id = static_cast<uint16_t>(_types.size());
        _nameToId[type.name] = type.id;
        _types.push_back(std::move(type));
        return _types.back().id;
    }

    /**
     * @brief Returns the block type with the given ID. No bounds check.
     */
    BlockType &get(uint16_t id) { return _types[id]; }

    /**
     * @brief Returns the block type with the given name.
     *
     * @throws std::out_of_range if no block has that name
     */
    BlockType &get(const std::string &name) { return _types[getIdByName(name)]; }

    /**
     * @brief Returns the ID of the block with the given name.
     *
     * @throws std::out_of_range if no block has that name
     */
    uint16_t getIdByName(const std::string &name) { return _nameToId.at(name); }

private:
    BlockRegistry() = default;
    BlockRegistry(BlockRegistry &registry) = delete;
    BlockRegistry &operator=(const BlockRegistry &) = delete;

    std::vector<BlockType> _types; // index in the vector == block ID
    std::unordered_map<std::string, uint16_t> _nameToId;
};