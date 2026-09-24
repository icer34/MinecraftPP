#version 460 core

out vec2 vNdc;

void main()
{
    vec2 pos = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    vNdc = pos * 2.0 -  1.0;
    // depth 0: the far plane with reverse-Z, so the sky only covers the pixels where no
    // geometry was drawn (the depth test is GL_GEQUAL, the depth buffer is cleared to 0)
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}