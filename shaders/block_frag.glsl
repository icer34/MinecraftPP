#version 330 core

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vTint;
in float vAO;
in vec4 vFragPosWorld;
in float vViewDepth;

uniform sampler2D atlas;
uniform sampler2DArray shadowMap;
uniform vec3 lightDir;
uniform mat4 lightSpaceMatrices[5];
uniform float cutoffDist[5];

const int CASCADE_COUNT = 5;

out vec4 FragColor;

const vec3 lightColor = vec3(1.0);

// fixed Poisson disk pattern (16 points in [-1,1]) -- sampling these instead of a regular
// NxN grid avoids the visible repeating banding a uniform grid produces at the same sample
// count, since the offsets aren't aligned to any axis.
const vec2 POISSON_DISK[16] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2(0.34495938, 0.29387760),
    vec2(-0.91588581, 0.45771432),
    vec2(-0.81544232, -0.87912464),
    vec2(-0.38277543, 0.27676845),
    vec2(0.97484398, 0.75648379),
    vec2(0.44323325, -0.97511554),
    vec2(0.53742981, -0.47373420),
    vec2(-0.26496911, -0.41893023),
    vec2(0.79197514, 0.19090188),
    vec2(-0.24188840, 0.99706507),
    vec2(-0.81409955, 0.91437590),
    vec2(0.19984126, 0.78641367),
    vec2(0.14383161, -0.14100790));

// PCF sample radius in texels -- how far the Poisson disk spreads, not how many samples.
const float PCF_RADIUS = 1.2;

float random(vec2 seed)
{
    return fract(sin(dot(seed, vec2(12.9898, 78.233))) * 43758.5453);
}

float shadowCalculation(vec3 projCoords, int cascadeIndex, float bias)
{
    if (projCoords.z > 1.0)
        return 0.0;

    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0).xy);

    // random per-fragment rotation of the sample pattern -- turns the fixed Poisson layout
    // into pixel-scale dithering instead of a visible repeating pattern across the screen
    float angle = random(gl_FragCoord.xy) * 6.28318530718;
    float s = sin(angle);
    float c = cos(angle);
    mat2 rotation = mat2(c, -s, s, c);

    float shadow = 0.0;
    for (int i = 0; i < 16; i++)
    {
        vec2 offset = (rotation * POISSON_DISK[i]) * texelSize * PCF_RADIUS;
        float pcfDepth = texture(shadowMap, vec3(projCoords.xy + offset, float(cascadeIndex))).r;
        shadow += (projCoords.z - bias > pcfDepth) ? 1.0 : 0.0;
    }

    return shadow / 16.0;
}

// cutoffDist[i] is the far distance of cascade i -- find the first cascade whose far
// distance is beyond viewDepth. falls back to the last cascade if viewDepth exceeds them all.
int computeCascadeIndex(float viewDepth)
{
    for (int i = 0; i < CASCADE_COUNT - 1; i++)
    {
        if (viewDepth < cutoffDist[i])
            return i;
    }
    return CASCADE_COUNT - 1;
}

void main()
{
    int cascadeIndex = computeCascadeIndex(vViewDepth);

    vec4 fragPosLightSpace = lightSpaceMatrices[cascadeIndex] * vec4(vFragPosWorld.xyz, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = (projCoords + 1.0) * 0.5;

    float bias = max(0.005 * (1.0 - dot(vNormal, -lightDir)), 0.0005);

    float shadowFactor = 1.0 - shadowCalculation(projCoords, cascadeIndex, bias);

    float ambientCoeff = 0.25f;
    float diffuseCoeff = max(dot(-lightDir, vNormal), 0.0);

    vec3 lighting = (ambientCoeff + (diffuseCoeff * shadowFactor)) * lightColor;

    // vec3 cascadeColors[5] = vec3[5](vec3(1,0,0), vec3(0,1,0), vec3(0,0,1), vec3(1,1,0), vec3(1,0,1));
    // FragColor = vec4(cascadeColors[cascadeIndex], 1.0);

    // FragColor = vec4(vec3(shadowFactor), 1.0);

    FragColor = texture(atlas, vTexCoord) * vec4(vTint, 1.0) * vec4(vec3(vAO), 1.0) * vec4(lighting, 1.0);
}
