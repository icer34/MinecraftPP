#pragma once

#include <glm/glm.hpp>
#include <string>

class Shader
{
public:
    Shader(const char *vertPath, const char *fragPath);
    ~Shader();

    void addGeometryShader(const char *path);

    void use();

    void setMat4Array(const std::string &name, const std::vector<glm::mat4> &value);
    void setMat4(const std::string &name, glm::mat4 value);
    void setVec3(const std::string &name, glm::vec3 value);
    void setInt(const std::string &name, int value);
    void setFloatArray(const std::string &name, const std::vector<float> &value);

private:
    unsigned int m_programID;
    unsigned int m_vertID;
    unsigned int m_fragID;
    unsigned int m_geomID = 0;
};