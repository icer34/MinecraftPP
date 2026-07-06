#include "shader.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

Shader::Shader(const char* vertPath, const char* fragPath)
{
    //load the shader file contents
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    // ensure ifstream objects can throw exceptions:
    vShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions (std::ifstream::failbit | std::ifstream::badbit);
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
        vertexCode   = vShaderStream.str();
        fragmentCode = fShaderStream.str();		
    }
    catch(std::ifstream::failure e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    //vertex shader compilation
    m_vertID = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(m_vertID, 1, &vShaderCode, NULL);
    glCompileShader(m_vertID);

    int success;
    char logBuffer[512];
    glGetShaderiv(m_vertID, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(m_vertID, 512, NULL, logBuffer);
        std::cout << "ERROR::SHADER::SHADER_NOT_COMPILED\n" << logBuffer << std::endl;
    }

    //fragment shader compilation
    m_fragID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(m_fragID, 1, &fShaderCode, NULL);
    glCompileShader(m_fragID);

    glGetShaderiv(m_fragID, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(m_fragID, 512, NULL, logBuffer);
        std::cout << "ERROR::SHADER::SHADER_NOT_COMPILED\n" << logBuffer << std::endl;
    }

    //program creation and linking
    m_programID = glCreateProgram();
    glAttachShader(m_programID, m_vertID);
    glAttachShader(m_programID, m_fragID);
    glLinkProgram(m_programID);
    glGetProgramiv(m_programID, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(m_programID, 512, NULL, logBuffer);
        std::cout << "ERROR::SHADER::SHADER_NOT_LINKED\n" << logBuffer << std::endl;
    }

    //delete shader once in the program
    glDeleteShader(m_vertID);
    glDeleteShader(m_fragID);
}

Shader::~Shader()
{
    glDeleteProgram(m_programID);
}

void Shader::use()
{
    glUseProgram(m_programID);
}

void Shader::setMat4(const std::string& name, glm::mat4 mat)
{
    int loc = glGetUniformLocation(m_programID, name.c_str());
    if(loc == -1)
    {
        std::cout << "ERROR::SHADER::UNIFORM_NOT_FOUND [" << name << "]" << std::endl;
    }

    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(mat));
}