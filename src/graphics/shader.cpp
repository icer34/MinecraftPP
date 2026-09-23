#include "shader.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

using glm::mat4;
using glm::vec3;

Shader::Shader(const char *vertPath, const char *fragPath)
{
    // load the shader file contents
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // open files
        vShaderFile.open(vertPath);
        fShaderFile.open(fragPath);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (const std::ifstream::failure &e)
    {
        throw std::runtime_error("ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: "
                                 + std::string(vertPath) + " / " + std::string(fragPath));
    }
    const char *vShaderCode = vertexCode.c_str();
    const char *fShaderCode = fragmentCode.c_str();

    // vertex shader compilation
    _vertID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(_vertID, 1, &vShaderCode, NULL);
    glCompileShader(_vertID);

    int success;
    char logBuffer[512];
    glGetShaderiv(_vertID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(_vertID, 512, NULL, logBuffer);
        throw std::runtime_error("ERROR::SHADER::VERTEX_NOT_COMPILED\n" + std::string(logBuffer));
    }

    // fragment shader compilation
    _fragID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(_fragID, 1, &fShaderCode, NULL);
    glCompileShader(_fragID);

    glGetShaderiv(_fragID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(_fragID, 512, NULL, logBuffer);
        throw std::runtime_error("ERROR::SHADER::FRAGMENT_NOT_COMPILED\n" + std::string(logBuffer));
    }

    // program creation and linking
    _programID = glCreateProgram();
    glAttachShader(_programID, _vertID);
    glAttachShader(_programID, _fragID);
    glLinkProgram(_programID);
    glGetProgramiv(_programID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(_programID, 512, NULL, logBuffer);
        throw std::runtime_error("ERROR::SHADER::SHADER_NOT_LINKED\n" + std::string(logBuffer));
    }
}

Shader::~Shader()
{
    glDeleteProgram(_programID);
    glDeleteShader(_vertID);
    glDeleteShader(_fragID);
    glDeleteShader(_geomID);
}

void Shader::addGeometryShader(const char *path)
{
    std::ifstream geomFile;
    geomFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

    std::string geomCodeString;
    try
    {
        // open files
        geomFile.open(path);
        std::stringstream geomShaderStream;
        // read file's buffer contents into streams
        geomShaderStream << geomFile.rdbuf();
        // close file handlers
        geomFile.close();
        // convert stream into string
        geomCodeString = geomShaderStream.str();
    }
    catch (const std::ifstream::failure &e)
    {
        throw std::runtime_error("ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " + std::string(path));
    }

    const char *geomCode = geomCodeString.c_str();

    _geomID = glCreateShader(GL_GEOMETRY_SHADER);
    glShaderSource(_geomID, 1, &geomCode, NULL);
    glCompileShader(_geomID);

    int success;
    char logBuffer[512];
    glGetShaderiv(_geomID, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(_geomID, 512, NULL, logBuffer);
        throw std::runtime_error("ERROR::SHADER::GEOM_NOT_COMPILED\n" + std::string(logBuffer));
    }

    glDeleteProgram(_programID);

    _programID = glCreateProgram();
    glAttachShader(_programID, _vertID);
    glAttachShader(_programID, _geomID);
    glAttachShader(_programID, _fragID);

    glLinkProgram(_programID);

    glGetProgramiv(_programID, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(_programID, 512, NULL, logBuffer);
        throw std::runtime_error("ERROR::SHADER::SHADER_NOT_LINKED\n" + std::string(logBuffer));
    }
}

void Shader::use() { glUseProgram(_programID); }

void Shader::setMat4(const std::string &name, mat4 mat)
{
    int loc = glGetUniformLocation(_programID, name.c_str());
    if (loc == -1)
    {
        std::cout << "ERROR::SHADER::UNIFORM_NOT_FOUND [" << name << "]" << std::endl;
    }

    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setMat4Array(const std::string &name, const std::vector<mat4> &value)
{
    std::string locName = name + "[0]";
    int loc = glGetUniformLocation(_programID, locName.c_str());
    if (loc == -1)
    {
        std::cout << "ERROR::SHADER::UNIFORM_NOT_FOUND [" << name << "]" << std::endl;
    }

    glUniformMatrix4fv(loc, value.size(), GL_FALSE, glm::value_ptr(value[0]));
}

void Shader::setVec3(const std::string &name, vec3 value)
{
    int loc = glGetUniformLocation(_programID, name.c_str());
    if (loc == -1)
    {
        std::cout << "ERROR::SHADER::UNIFORM_NOT_FOUND [" << name << "]" << std::endl;
    }

    glUniform3f(loc, value.x, value.y, value.z);
}

void Shader::setInt(const std::string &name, int value)
{
    int loc = glGetUniformLocation(_programID, name.c_str());
    if (loc == -1)
    {
        std::cout << "ERROR::SHADER::UNIFORM_NOT_FOUND [" << name << "]" << std::endl;
    }

    glUniform1i(loc, value);
}

void Shader::setFloat(const std::string &name, float value)
{
    int loc = glGetUniformLocation(_programID, name.c_str());
    if (loc == -1)
    {
        std::cout << "ERROR::SHADER::UNIFORM_NOT_FOUND [" << name << "]" << std::endl;
    }

    glUniform1f(loc, value);
}

void Shader::setFloatArray(const std::string &name, const std::vector<float> &value)
{
    std::string locName = name + "[0]";
    int loc = glGetUniformLocation(_programID, locName.c_str());
    if (loc == -1)
    {
        std::cout << "ERROR::SHADER::UNIFORM_NOT_FOUND [" << name << "]" << std::endl;
    }

    glUniform1fv(loc, value.size(), &value[0]);
}