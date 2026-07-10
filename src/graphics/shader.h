#pragma once 

#include <glm/glm.hpp>
#include <string>

class Shader 
{
public:
    Shader(const char* vertPath, const char* fragPath);
    ~Shader();

    void use();
    
    void setMat4(const std::string& name, glm::mat4 value);
    void setInt(const std::string& name, int value);

private:
    unsigned int m_programID;
    unsigned int m_vertID;
    unsigned int m_fragID;
};