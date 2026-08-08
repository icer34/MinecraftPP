#include "shader.h"

#include <filesystem>
#include <fstream>
namespace fs = std::filesystem;
#include <iostream>
#include <sstream>

#include "core/chunk_attrib_registry.h"

namespace
{
// look if the shader path is written by the game, if it is take the game shader, otherwise take the default engine one
std::vector<std::string> g_shaderIncludePaths = {"game/shaders/", "engine/shaders/"};
std::string resolveShaderPath(const std::string &filename)
{
    for (const auto &includePath : g_shaderIncludePaths)
    {
        std::string candidate = includePath + filename;
        if (fs::exists(candidate))
            return candidate;
    }

    return "";
}

std::string stageExtention(ShaderStage stage)
{
    switch (stage)
    {
    case ShaderStage::Vertex:
        return ".mcpvs";
    case ShaderStage::Fragment:
        return ".mcpfs";
    case ShaderStage::Geometry:
        return ".mcpgs";
    }

    return "";
}

std::string generateVertexFormat()
{
    size_t N = ChunkAttribRegistry::instance().computeVertexWordCount();
    std::string glslType = (N == 2) ? "uvec2" : (N == 3) ? "uvec3" : "uvec4";

    std::stringstream out;
    out << "layout (location = 0) in " << glslType << " packedData;\n";

    out << "uint chunkX = (packedData[0] >> 28) & 0xFu;\n";
    out << "uint chunkY = (packedData[0] >> 20) & 0xFFu;\n";
    out << "uint chunkZ = (packedData[0] >> 16) & 0xFu;\n";
    out << "uint normalIdx = (packedData[0] >> 13) & 0x7u;\n";
    out << "uint textureIdx = (packedData[0] >> 1) & 0xFFFu;\n";
    out << "bool isTinted = bool(packedData[0] & 0x1u);\n";

    uint32_t bitCursor = 0;
    auto unpackBits = [&](const std::string &name, uint32_t width)
    {
        size_t wordIdx = 1 + bitCursor / 32;
        size_t shift = bitCursor % 32;
        uint32_t mask = (1u << width) - 1u;
        out << "uint " << name << " = (packedData[" << wordIdx << "] >> " << shift << "u) & 0x" << std::hex << mask
            << std::dec << "u;\n";
        bitCursor += width;
    };

    unpackBits("cornerIdx", 2);
    unpackBits("aoValue", 2);
    for (const auto &[name, info] : ChunkAttribRegistry::instance().getEnabledAttribs())
        unpackBits(name, 8);

    return out.str();
}

// handles the include directives, while keeping the #version <...> on the first line
std::string resolveShaderSource(const std::string &path, std::vector<std::string> &stack)
{
    // check for circular include
    if (std::find(stack.begin(), stack.end(), path) != stack.end())
        throw std::runtime_error("Circular include detected: " + path);

    stack.push_back(path);

    std::ifstream file(path);

    if (!file.is_open())
        throw std::runtime_error("Could not open shader file: " + path);

    std::string line;
    std::string word, includePath;
    std::stringstream out;
    while (std::getline(file, line))
    {
        if (line.starts_with("#include"))
        {
            std::stringstream ss(line);
            while (ss >> word)
            {
                includePath = word.substr(1, word.size() - 2);
            }

            if (includePath == "chunk_vertex_format")
                out << generateVertexFormat() << '\n';
            else
                out << resolveShaderSource(resolveShaderPath(includePath), stack) << '\n';
        }
        else
        {
            out << line << '\n';
        }
    }

    stack.pop_back();
    return out.str();
}
}; // namespace

void Shader::load(const std::string &role)
{
    //* 1. retrieve the full src code of the shaders
    std::string vertPath = resolveShaderPath(role + stageExtention(ShaderStage::Vertex));
    std::string fragPath = resolveShaderPath(role + stageExtention(ShaderStage::Fragment));
    std::string geomPath = resolveShaderPath(role + stageExtention(ShaderStage::Geometry));

    std::vector<std::string> paths{vertPath, fragPath};
    if (fs::exists(geomPath))
        paths.push_back(geomPath);

    std::vector<std::string> sources;
    for (auto &path : paths)
    {
        std::vector<std::string> stack{};
        sources.push_back(resolveShaderSource(path, stack));
    }

    std::vector<const char *> shaderSources;
    for (auto &source : sources)
        shaderSources.push_back(source.c_str());

    //* 2. create the actual OpenGL shader
    unsigned int vertID, fragID, geomID = 0;
    int success;
    char infoLog[512];

    m_programID = glCreateProgram();

    vertID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertID, 1, &shaderSources[0], nullptr);
    glCompileShader(vertID);
    glGetShaderiv(vertID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertID, 512, nullptr, infoLog);
        std::cout << "[" << role << "] " << "vertex shader compilation error: " << infoLog << std::endl;
    }
    glAttachShader(m_programID, vertID);

    fragID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragID, 1, &shaderSources[1], nullptr);
    glCompileShader(fragID);
    glGetShaderiv(fragID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragID, 512, nullptr, infoLog);
        std::cout << "[" << role << "] " << "fragment shader compilation error: " << infoLog << std::endl;
    }
    glAttachShader(m_programID, fragID);

    // compile geometry shader only if it exists
    if (shaderSources.size() >= 3)
    {
        geomID = glCreateShader(GL_GEOMETRY_SHADER);
        glShaderSource(geomID, 1, &shaderSources[2], nullptr);
        glCompileShader(geomID);
        glGetShaderiv(geomID, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(geomID, 512, nullptr, infoLog);
            std::cout << "[" << role << "] " << "geometry shader compilation error: " << infoLog << std::endl;
        }
        glAttachShader(m_programID, geomID);
    }

    glLinkProgram(m_programID);
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(m_programID, 512, nullptr, infoLog);
        std::cout << "[" << role << "] " << "shader linking error: " << infoLog << std::endl;
    }

    glDeleteShader(vertID);
    glDeleteShader(fragID);
    glDeleteShader(geomID);

    //* 3. cache the active uniforms and their location
    int count;
    glGetProgramiv(m_programID, GL_ACTIVE_UNIFORMS, &count);
    for (int i = 0; i < count; i++)
    {
        char name[256];
        int length, size;
        GLenum type;
        glGetActiveUniform(m_programID, i, 256, &length, &size, &type, name);

        // if the uniform is an array, openGL sends back "<varName>[0]" as the variable name instead of <varName>. so we
        // strip the bracket part before saving the uniform name
        std::string cleanName = name;
        size_t bracketPos = cleanName.find('[');
        if (bracketPos != std::string::npos)
            cleanName = cleanName.substr(0, bracketPos);

        m_activeUniforms.push_back({cleanName, type});
        m_activeUniformLocations[cleanName] = glGetUniformLocation(m_programID, name);

        // first value is default for every uniform
        m_activeUniformValues[cleanName] = std::nullopt;
    }
}

void Shader::bind() { glUseProgram(m_programID); }
const std::vector<std::pair<std::string, GLenum>> &Shader::getActiveUniforms() const { return m_activeUniforms; }