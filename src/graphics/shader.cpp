#include "shader.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include "graphics/gl/gl_debug.h"

namespace fs = std::filesystem;

namespace
{
std::string readFile(const fs::path &path)
{
    std::ifstream file(path);
    if (!file)
        throw std::runtime_error("ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " + path.string());

    std::stringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

/**
 * Reads a shader file and recursively replaces its `#include "..."` lines with the content of
 * the included file. `files` lists every file read so far: its index is the source string
 * number used in the `#line` directives, and a file already in it is not included again.
 */
std::string preprocess(const fs::path &path, std::vector<fs::path> &files)
{
    const int fileIndex = static_cast<int>(files.size());
    files.push_back(path);

    std::istringstream source(readFile(path));
    std::string result;
    std::string line;
    int lineNumber = 0;

    while (std::getline(source, line))
    {
        lineNumber++;

        size_t first = line.find_first_not_of(" \t");
        if (first == std::string::npos || line.compare(first, 8, "#include") != 0)
        {
            result += line + '\n';
            continue;
        }

        size_t open = line.find('"', first);
        size_t close = line.find('"', open + 1);
        if (open == std::string::npos || close == std::string::npos)
            throw std::runtime_error("ERROR::SHADER::BAD_INCLUDE: " + path.string() + ":"
                                     + std::to_string(lineNumber));

        fs::path included = path.parent_path() / line.substr(open + 1, close - open - 1);
        if (!fs::exists(included))
            throw std::runtime_error("ERROR::SHADER::INCLUDE_NOT_FOUND: " + included.string()
                                     + " (from " + path.string() + ":"
                                     + std::to_string(lineNumber) + ")");

        bool alreadyIncluded = false;
        for (const fs::path &file : files)
            alreadyIncluded |= fs::equivalent(file, included);

        if (!alreadyIncluded)
        {
            int includedIndex = static_cast<int>(files.size());
            result += "#line 1 " + std::to_string(includedIndex) + '\n';
            result += preprocess(included, files);
        }
        // back to the including file, on the line after the #include
        result += "#line " + std::to_string(lineNumber + 1) + ' ' + std::to_string(fileIndex)
                + '\n';
    }

    return result;
}

std::string shaderInfoLog(GLuint shader)
{
    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string log(std::max(length, 1), '\0');
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    return log;
}

std::string programInfoLog(GLuint program)
{
    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::string log(std::max(length, 1), '\0');
    glGetProgramInfoLog(program, length, nullptr, log.data());
    return log;
}

GLShader compile(GLenum type, const char *path)
{
    std::vector<fs::path> files;
    std::string source = preprocess(path, files);
    const char *sourcePtr = source.c_str();

    GLShader shader = gl::createShader(type);
    glShaderSource(shader.id(), 1, &sourcePtr, nullptr);
    glCompileShader(shader.id());

    GLint success = 0;
    glGetShaderiv(shader.id(), GL_COMPILE_STATUS, &success);
    if (!success)
    {
        std::string message = "ERROR::SHADER::NOT_COMPILED: " + std::string(path) + '\n';
        for (size_t i = 0; i < files.size(); i++)
            message += "  source " + std::to_string(i) + " = " + files[i].string() + '\n';
        throw std::runtime_error(message + shaderInfoLog(shader.id()));
    }

    return shader;
}
} // namespace

Shader::Shader(const char *vertPath, const char *fragPath, const char *geomPath)
    : _program(gl::createProgram())
{
    // the shader objects are only needed until the link: their handles delete them at the end
    // of this constructor, once detached
    GLShader vert = compile(GL_VERTEX_SHADER, vertPath);
    GLShader frag = compile(GL_FRAGMENT_SHADER, fragPath);
    GLShader geom;
    if (geomPath != nullptr)
        geom = compile(GL_GEOMETRY_SHADER, geomPath);

    glAttachShader(_program.id(), vert.id());
    glAttachShader(_program.id(), frag.id());
    if (geom)
        glAttachShader(_program.id(), geom.id());

    glLinkProgram(_program.id());

    GLint success = 0;
    glGetProgramiv(_program.id(), GL_LINK_STATUS, &success);
    if (!success)
    {
        throw std::runtime_error("ERROR::SHADER::NOT_LINKED: " + std::string(vertPath) + " / "
                                 + std::string(fragPath) + '\n' + programInfoLog(_program.id()));
    }

    glDetachShader(_program.id(), vert.id());
    glDetachShader(_program.id(), frag.id());
    if (geom)
        glDetachShader(_program.id(), geom.id());

    std::string label = fs::path(vertPath).stem().string() + " + "
                      + fs::path(fragPath).stem().string();
    if (geomPath != nullptr)
        label += " + " + fs::path(geomPath).stem().string();
    gl::setLabel(GL_PROGRAM, _program.id(), label);
}

void Shader::use() const { glUseProgram(_program.id()); }

GLint Shader::uniformLocation(std::string_view name)
{
    auto it = _uniformLocations.find(name);
    if (it != _uniformLocations.end())
        return it->second;

    std::string key(name);
    GLint location = glGetUniformLocation(_program.id(), key.c_str());
    if (location == -1)
        std::cout << "ERROR::SHADER::UNIFORM_NOT_FOUND [" << name << "]" << std::endl;

    // -1 is cached too: the error is printed once, and glProgramUniform* ignores location -1
    _uniformLocations.emplace(std::move(key), location);
    return location;
}

void Shader::setMat4(std::string_view name, const glm::mat4 &value)
{
    glProgramUniformMatrix4fv(
        _program.id(), uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setVec3(std::string_view name, glm::vec3 value)
{
    setVec3(uniformLocation(name), value);
}

void Shader::setInt(std::string_view name, int value)
{
    glProgramUniform1i(_program.id(), uniformLocation(name), value);
}

void Shader::setFloat(std::string_view name, float value)
{
    glProgramUniform1f(_program.id(), uniformLocation(name), value);
}

void Shader::setVec3(GLint location, glm::vec3 value)
{
    glProgramUniform3f(_program.id(), location, value.x, value.y, value.z);
}
