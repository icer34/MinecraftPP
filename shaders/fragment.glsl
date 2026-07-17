#version 330 core

uniform sampler2D atlas;

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vTint;
in float vAO;

out vec4 FragColor;

void main()
{
    FragColor = texture(atlas, vTexCoord) * vec4(vTint, 1.0) * vec4(vec3(vAO), 1.0);
}
