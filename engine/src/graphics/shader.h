#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <glad/glad.h>

enum class ShaderStage
{
    Vertex,
    Fragment,
    Geometry,
};

using UniformValue
    = std::variant<float, int, glm::vec2, glm::vec3, glm::mat4, std::vector<float>, std::vector<glm::mat4>>;

class Shader
{
public:
    Shader() = default;
    // the shader role represents what the shader will draw (ex: block, water, hud, outline, ...)
    void load(const std::string &role);
    void bind();
    const std::vector<std::pair<std::string, GLenum>> &getActiveUniforms() const;
    template <typename T> void setIfChanged(const std::string &name, T value);

private:
    unsigned int m_programID = 0;

    std::vector<std::pair<std::string, GLenum>> m_activeUniforms;
    std::unordered_map<std::string, unsigned int> m_activeUniformLocations;
    std::unordered_map<std::string, std::optional<UniformValue>> m_activeUniformValues;
};

#include "shader.inl"