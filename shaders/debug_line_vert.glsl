#version 460 core

#include "common/frame_data.glsl"

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 color;

// same trick as outline_vert.glsl: pulls the line toward the camera, so that it wins the depth
// test against the block faces it lies on (chunk borders, block edges) instead of z-fighting
const float DEPTH_PULL = 0.995;

void main()
{
    vec4 viewPos = view * vec4(aPos, 1.0);
    viewPos.xyz *= DEPTH_PULL;
    gl_Position = projection * viewPos;
    color = aColor;
}
