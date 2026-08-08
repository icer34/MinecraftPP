#pragma once

#include <algorithm>
#include <assert.h>
#include <cstdint>
#include <cstring>
#include <exception>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

enum class AttribScope
{
    PerColumn,
    PerBlock,
};

class AttribType
{
public:
    enum Value : uint8_t
    {
        Uint8,
        Uint16,
        Uint32,
        Float
    };

    // exists only so AttribInfo (and containers of it) stay default-constructible --
    // never meant to be relied on semantically, every real attribute goes through the
    // Value constructor below.
    constexpr AttribType()
        : m_value(Uint8)
    {
    }

    constexpr AttribType(Value value)
        : m_value(value)
    {
    }

    constexpr size_t size() const
    {
        switch (m_value)
        {
        case Uint8:
            return sizeof(uint8_t);
        case Uint16:
            return sizeof(uint16_t);
        case Uint32:
            return sizeof(uint32_t);
        case Float:
            return sizeof(float);
        }
        return 0;
    }

    constexpr operator Value() const { return m_value; }

private:
    Value m_value;
};

struct AttribInfo
{
    AttribType type;
    AttribScope scope;
    std::vector<uint8_t> defaultValue; // raw bytes of the default value of the attrib
};

template <typename T> struct AttribTypeOf;
template <> struct AttribTypeOf<uint8_t>
{
    static constexpr AttribType value = AttribType::Uint8;
};
template <> struct AttribTypeOf<uint16_t>
{
    static constexpr AttribType value = AttribType::Uint16;
};
template <> struct AttribTypeOf<uint32_t>
{
    static constexpr AttribType value = AttribType::Uint32;
};
template <> struct AttribTypeOf<float>
{
    static constexpr AttribType value = AttribType::Float;
};

class ChunkAttribRegistry
{
public:
    static ChunkAttribRegistry &instance()
    {
        static ChunkAttribRegistry reg;
        return reg;
    }

    template <typename T> void registerAttrib(const std::string &name, AttribScope scope, T defaultValue = T{})
    {
        if (!isValidGlslIdentifier(name))
            throw std::runtime_error("attrib name '" + name + "' is not a valid GLSL identifier");
        if (GLSL_RESERVED.contains(name))
            throw std::runtime_error("attrib name '" + name + "' is already reserved by GLSL");
        AttribInfo info;
        info.type = AttribTypeOf<T>::value;
        info.scope = scope;
        info.defaultValue.resize(sizeof(T));
        std::memcpy(info.defaultValue.data(), &defaultValue, sizeof(T));

        m_attribs[name] = std::move(info);
    }

    void enableAttrib(const std::string &name)
    {
        const AttribInfo &info = m_attribs.at(name);
        assert(info.type == AttribType::Uint8 && "only uint8 attribs can be enabled for rendering");

        constexpr size_t FIXED_ATTRIB_REGION_BITS = 4; // cornerIdx(2) + aoValue(2)
        size_t projectedBits = FIXED_ATTRIB_REGION_BITS + (m_enabledAttribs.size() + 1) * 8;
        assert(1 + (projectedBits + 31) / 32 <= 4
               && "enabling this attrib would exceed the maximum data per vertex (4 * 32 bits)");

        m_enabledAttribs.emplace_back(name, info);
    }

    size_t computeVertexWordCount() const
    {
        constexpr size_t FIXED_ATTRIB_REGION_BITS = 4; // cornerIdx(2) + aoValue(2)

        size_t attribBits = m_enabledAttribs.size() * 8;
        size_t extraWords = (FIXED_ATTRIB_REGION_BITS + attribBits + 31) / 32; // ceil division

        return 1 + extraWords; // data1 + le reste
    }

    const std::vector<std::pair<std::string, AttribInfo>> &getEnabledAttribs() const { return m_enabledAttribs; }
    const std::unordered_map<std::string, AttribInfo> &getAll() const { return m_attribs; }
    const AttribInfo &get(const std::string &name) const { return m_attribs.at(name); }

private:
    std::unordered_map<std::string, AttribInfo> m_attribs;
    std::vector<std::pair<std::string, AttribInfo>> m_enabledAttribs;

    const std::unordered_set<std::string> GLSL_RESERVED
        = {"in",   "out",     "inout",  "uniform", "attribute", "varying", "const", "void", "bool", "float", "int",
           "uint", "discard", "return", "struct",  "true",      "false",   "if",    "else", "for",  "while"};

    bool isValidGlslIdentifier(const std::string &name)
    {
        if (name.empty() || (!std::isalpha(name[0]) && name[0] != '_'))
            return false;
        return std::all_of(name.begin(), name.end(), [](char c) { return std::isalnum(c) || c == '_'; });
    }
};