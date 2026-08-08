#include <string>

template <typename T> void Shader::setIfChanged(const std::string &name, T value)
{
    auto &lastVal = m_activeUniformValues[name];

    // check if the value has changed or not
    if (lastVal.has_value() && std::holds_alternative<T>(*lastVal) && std::get<T>(*lastVal) == value)
        return;

    // use the correct OpenGL call to set the new value
    unsigned int loc = m_activeUniformLocations[name];
    if constexpr (std::is_same_v<T, float>)
        glUniform1f(loc, value);
    else if constexpr (std::is_same_v<T, int>)
        glUniform1i(loc, value);
    else if constexpr (std::is_same_v<T, glm::vec2>)
        glUniform2f(loc, value.x, value.y);
    else if constexpr (std::is_same_v<T, glm::vec3>)
        glUniform3f(loc, value.x, value.y, value.z);
    else if constexpr (std::is_same_v<T, glm::mat4>)
        glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(value));
    else if constexpr (std::is_same_v<T, std::vector<float>>)
        glUniform1fv(loc, value.size(), &value[0]);
    else if constexpr (std::is_same_v<T, std::vector<glm::mat4>>)
        glUniformMatrix4fv(loc, value.size(), GL_FALSE, glm::value_ptr(value[0]));

    // update the last-sent value
    lastVal = value;
}