#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <glm/glm.hpp>
#include <stdexcept>

#include "block_registry.h"
#include "chunk_attrib_registry.h"

struct ChunkCoord
{
    int32_t x, z;
    bool operator==(const ChunkCoord &o) const { return x == o.x && z == o.z; }
};

inline uint64_t computeChunkID(ChunkCoord coord)
{
    constexpr int BITS = 21;
    constexpr uint64_t MASK = (1ull << BITS) - 1;

    return (static_cast<uint64_t>(coord.x) & MASK) | ((static_cast<uint64_t>(coord.z) & MASK) << (2 * BITS));
}

namespace std
{
template <> struct hash<ChunkCoord>
{
    size_t operator()(const ChunkCoord &coord) const noexcept { return static_cast<size_t>(computeChunkID(coord)); }
};
} // namespace std

class Chunk
{
public:
    static constexpr int SIZE = 16;
    static constexpr int HEIGHT = 256;

    explicit Chunk(ChunkCoord coord)
        : m_id(computeId(coord)),
          m_coord(coord)
    {
        m_blocks.fill(EMPTY_BLOCK_ID);

        auto &attribReg = ChunkAttribRegistry::instance();

        auto &attribs = attribReg.getAll();
        for (auto &[name, attrib] : attribs)
        {
            size_t elemSize = attrib.type.size();
            size_t elemCount = (attrib.scope == AttribScope::PerColumn) ? (SIZE * SIZE) : (SIZE * SIZE * HEIGHT);
            std::vector<uint8_t> storage(elemSize * elemCount);
            for (size_t i = 0; i < elemCount; i++)
                std::memcpy(storage.data() + i * elemSize, attrib.defaultValue.data(), elemSize);
            m_attribsData[name] = storage;
        }
    }

    template <typename T> T get(const std::string &name, glm::ivec3 pos) const
    {
        const auto &info = ChunkAttribRegistry::instance().get(name);
        if (info.type != AttribTypeOf<T>::value)
            throw std::runtime_error("non matching type");

        size_t byteOffset = info.type.size() * index(pos);
        auto &data = m_attribsData.at(name);
        return *reinterpret_cast<const T *>(data.data() + byteOffset);
    }

    template <typename T> T get(const std::string &name, glm::ivec2 pos) const
    {
        const auto &info = ChunkAttribRegistry::instance().get(name);
        if (info.type != AttribTypeOf<T>::value)
            throw std::runtime_error("non matching type");

        size_t byteOffset = info.type.size() * index(pos);
        auto &data = m_attribsData.at(name);
        return *reinterpret_cast<const T *>(data.data() + byteOffset);
    }

    template <typename T> void set(const std::string &name, glm::ivec2 pos, T value)
    {
        const auto &info = ChunkAttribRegistry::instance().get(name);
        if (info.type != AttribTypeOf<T>::value)
            throw std::runtime_error("non matching type");

        auto &data = m_attribsData.at(name);
        size_t byteOffset = info.type.size() * index(pos);
        *reinterpret_cast<T *>(m_attribsData.at(name).data() + byteOffset) = value;
    }

    template <typename T> void set(const std::string &name, glm::ivec3 pos, T value)
    {
        const auto &info = ChunkAttribRegistry::instance().get(name);
        if (info.type != AttribTypeOf<T>::value)
            throw std::runtime_error("non matching type");

        size_t byteOffset = info.type.size() * index(pos);
        *reinterpret_cast<T *>(m_attribsData.at(name).data() + byteOffset) = value;
    }

    uint16_t getBlock(glm::ivec3 pos) const { return m_blocks.at(index(pos)); }
    void setBlock(uint16_t id, glm::ivec3 pos)
    {
        m_blocks[index(pos)] = id;
        m_dirty = true;
    }

    ChunkCoord getCoords() const { return m_coord; }
    uint64_t getID() const { return m_id; }

    bool isDirty() { return m_dirty; }
    void clearDirty() { m_dirty = false; }

private:
    const uint64_t m_id;
    ChunkCoord m_coord;
    std::array<uint16_t, SIZE * SIZE * HEIGHT> m_blocks;

    std::unordered_map<std::string, std::vector<uint8_t>> m_attribsData; // raw bytes of attrib data

    bool m_dirty; // a chunk is set dirty if its modified thus needs to be remeshed

    static size_t index(glm::ivec3 pos) { return static_cast<size_t>(pos.x + pos.y * SIZE + pos.z * SIZE * HEIGHT); }

    static size_t index(glm::ivec2 pos) { return static_cast<size_t>(pos.x + pos.y * SIZE); }

    static uint64_t computeId(ChunkCoord coord) { return computeChunkID(coord); }
};