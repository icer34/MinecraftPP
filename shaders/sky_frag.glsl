#version 330 core

uniform float time;
uniform vec3 lightDir;
uniform mat4 invProjection;
uniform mat4 invView;

in vec2 vNdc;
out vec4 FragColor;

vec3 sky(vec3 worldDir)
{
    vec3 zenithColor = vec3(0.15, 0.35, 0.7);
    vec3 horizonColor = vec3(0.9, 0.8, 0.7);
    vec3 sunColor = vec3(1.0, 0.95, 0.8);

    float heightFactor = clamp(worldDir.y, 0.0, 1.0);
    vec3 skyColor = mix(horizonColor, zenithColor, heightFactor);

    float sunAmount = max(dot(worldDir, -lightDir), 0.0);
    float sunDisc = pow(sunAmount, 2000.0);
    return skyColor + sunDisc * sunColor;
}

void main()
{
    float temp = time;

    vec4 viewDir4 = invProjection * vec4(vNdc.xy, 1.0, 1.0);
    vec3 viewDir = viewDir4.xyz / viewDir4.w;
    vec3 worldDir = normalize(mat3(invView) * viewDir); 

    FragColor = vec4(sky(worldDir), 1.0);
}