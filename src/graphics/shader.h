#pragma once 

#include <glm/glm.hpp>

class Shader 
{
public:
    Shader(const char* vertPath, const char* fragPath);
    ~Shader();

    void use();
    
    void setMat4(const std::string& name, glm::mat4 value);

private:
    unsigned int m_programID;
    unsigned int m_vertID;
    unsigned int m_fragID;
};