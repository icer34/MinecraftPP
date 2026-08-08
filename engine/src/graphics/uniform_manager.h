#pragma once

#include <glm/glm.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "shader.h"

class UniformManager
{
public:
    static UniformManager &instance()
    {
        static UniformManager manager;
        return manager;
    }

    template <typename T> void setValue(const std::string &name, T value) { m_uniforms[name] = value; }

    void applyTo(Shader &shader)
    {
        for (const auto &[name, type] : shader.getActiveUniforms())
        {
            auto it = m_uniforms.find(name);
            if (it == m_uniforms.end())
                throw std::runtime_error("uniform '" + name + "' is active but not registered in the manager");

            std::visit([&](auto &&value) { shader.setIfChanged(name, value); }, it->second);
        }
    }

    bool has(const std::string &name) const { return m_uniforms.count(name) > 0; }

private:
    std::unordered_map<std::string, UniformValue> m_uniforms;

    UniformManager() = default;
    UniformManager(const UniformManager &) = delete;
    UniformManager &operator=(const UniformManager &) = delete;
};