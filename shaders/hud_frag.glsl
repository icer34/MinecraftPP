#version 460 core

#include "common/texture_units.glsl"

in vec2 vUV;
in vec4 vColor;

layout(binding = TEX_UNIT_HUD_ATLAS) uniform sampler2D atlas;

out vec4 FragColor;

void main()
{
    FragColor = texture(atlas, vUV) * vColor;
}
