#version 330 core

in vec3 oNormal;

out vec4 FragColor;

void main()
{
    FragColor = vec4(abs(oNormal), 1.0);  // orange
}
