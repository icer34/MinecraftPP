#version 330 core

uniform sampler2D atlas;

in vec2 oTexCoord;

out vec4 FragColor;

void main()
{
    FragColor = texture(atlas, oTexCoord);  // orange
}
