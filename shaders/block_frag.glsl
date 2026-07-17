#version 330 core

in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vTint;
in float vAO;
in vec4 vFragPosLightSpace;

uniform sampler2D atlas;
uniform sampler2D shadowMap;
uniform vec3 lightDir;

out vec4 FragColor;

const vec3 lightColor = vec3(1.0);

// PCF radius in texels: 0 = a single hard sample (sharp, blocky edges, Minecraft-like).
// raise it (1 = 3x3, 2 = 5x5, ...) to average more neighbors for softer shadow edges.
const int PCF_RADIUS = 2;

float shadowCalculation(vec3 projCoords, float bias)
{
    if (projCoords.z > 1.0)
        return 0.0;

    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));

    float shadow = 0.0;
    int sampleCount = 0;
    for (int x = -PCF_RADIUS; x <= PCF_RADIUS; x++)
    {
        for (int y = -PCF_RADIUS; y <= PCF_RADIUS; y++)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (projCoords.z - bias > pcfDepth) ? 1.0 : 0.0;
            sampleCount++;
        }
    }

    return shadow / float(sampleCount);
}

void main()
{
    vec3 projCoords = vFragPosLightSpace.xyz / vFragPosLightSpace.w;
    projCoords = (projCoords + 1.0) * 0.5;

    float bias = max(0.005 * (1.0 - dot(vNormal, -lightDir)), 0.0005);
    float shadowFactor = 1.0 - shadowCalculation(projCoords, bias);

    float ambientCoeff = 0.25f;
    float diffuseCoeff = max(dot(-lightDir, vNormal), 0.0);

    vec3 lighting = (ambientCoeff + (diffuseCoeff * shadowFactor)) * lightColor;

    FragColor = texture(atlas, vTexCoord) * vec4(vTint, 1.0) * vec4(vec3(vAO), 1.0) * vec4(lighting, 1.0);
}
