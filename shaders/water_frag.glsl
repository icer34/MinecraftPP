#version 330 core

in vec4 vFragPosWorld;
in float vViewDepth;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 lightDir;
uniform vec3 camPos;
uniform float time;
uniform float zNear;
uniform float zFar;

uniform sampler2D solidColor;
uniform sampler2D solidDepth;

out vec4 FragColor;

const vec3 lightColor = vec3(242.0 / 255.0, 232.0 / 255.0, 174.0 / 255.0);

const int WAVE_ITERATIONS = 5; // must match water_vert.glsl's WAVE_ITERATIONS
const float GOLDEN_ANGLE = 2.399963;

// must match water_vert.glsl's base/GAIN/LACUNARITY exactly, or this normal won't match
// the actual geometry displaced there.
const float BASE_AMPLITUDE = 0.2;
const float BASE_K = 0.5;
const float BASE_SPEED = 0.2;
const float GAIN = 0.5;       // amplitude multiplier per octave (aka persistence)
const float LACUNARITY = 1.4; // frequency (and speed) multiplier per octave

const float FOG_DENSITY = 0.3;

const float REFRACTION_STRENGTH = 0.5;
// water shallower than this gets proportionally less UV distortion, so refraction fades
// out toward the shore instead of staying at full strength right up to the mesh edge
const float MAX_REFRACTION_DEPTH = 8.0;

const float F0 = 0.02; //refractance at normal incidence of water (derived from refraction index)

// analytic gradient of the gerstner-wave sum in water_vert.glsl's waveHeight():
// h_i = A_i * s_i^2, s_i = sin(x_i)*0.5+0.5, x_i = w_i*t_i - k_i*dot(dir_i, coord)
// => dh_i/dcoord = -A_i * k_i * s_i * cos(x_i) * dir_i
vec3 waveNormal(vec2 worldXZ)
{
    const float g = 9.8;

    float amplitude = BASE_AMPLITUDE;
    float k = BASE_K;
    float speed = BASE_SPEED;
    vec2 grad = vec2(0.0);

    for (int i = 0; i < WAVE_ITERATIONS; i++)
    {
        float angle = float(i) * GOLDEN_ANGLE;
        vec2 waveDir = vec2(cos(angle), sin(angle));

        float w = sqrt(g * k);
        float x = w * (speed * time) - k * dot(waveDir, worldXZ);
        float s = sin(x) * 0.5 + 0.5;

        float slope = -amplitude * k * s * cos(x);
        grad += slope * waveDir;

        amplitude *= GAIN;
        k *= LACUNARITY;
        speed *= LACUNARITY;
    }

    return normalize(vec3(-grad.x, 1.0, -grad.y));
}

struct RayHit
{
    bool hit;
    vec2 uv;
};

RayHit marchScene(vec3 viewOrigin, vec3 viewDir, int numSteps, float stepSize)
{
    RayHit result;
    result.hit = false;
    result.uv = vec2(0.0);

    vec3 pos = viewOrigin + stepSize * viewDir; // small offset to not hit the water frag itself
    for(int i = 0; i < numSteps; i++)
    {
        vec4 proj = projection * vec4(pos, 1.0);
        // perspective division
        vec3 projectedNDC = proj.xyz / proj.w;
        projectedNDC = projectedNDC * 0.5 + 0.5; // --> [0,1]
        float sceneDepthNDC = texture(solidDepth, projectedNDC.xy).r;

        //check if the projected uv is out of screen
        if(projectedNDC.x < 0 || projectedNDC.x > 1.0 || projectedNDC.y < 0 || projectedNDC.y > 1.0)
        {
            break;
        }

        if(sceneDepthNDC < 0.9999 && projectedNDC.z > sceneDepthNDC)
        {
            result.hit = true;
            result.uv = projectedNDC.xy;
            break;
        }

        pos += viewDir * stepSize;
        stepSize *= 1.15;
    }

    return result;
}

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
    vec3 normal = waveNormal(vFragPosWorld.xz);
    vec3 viewDir = normalize(camPos - vFragPosWorld.xyz);
    vec3 reflectDir = reflect(lightDir, normal);

    //* ===== LIGHTING =====
    float NoV = max(dot(normal, viewDir), 0.0);
    float fresnel = F0 + (1.0 - F0) * pow(1.0 - NoV, 5.0); //Schlick approximation of fresnel's law

    float specularCoeff = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    vec3 specularLighting = specularCoeff * lightColor * fresnel;

    float diffuseCoeff = max(dot(-lightDir, normal), 0.0);
    vec3 diffuseLighting = diffuseCoeff * lightColor;
    float ambientCoeff = 0.2;

    vec3 lighting = (ambientCoeff + diffuseCoeff) * lightColor;

    //* ===== DEPTH CALCULATIONS =====
    vec2 screenUV = gl_FragCoord.xy / vec2(textureSize(solidColor, 0));
    // non linear depth --> needs to be linearized before comparing to vViewDepth
    float sceneDepthNDC = texture(solidDepth, screenUV).r; 
    float ndc = sceneDepthNDC * 2.0 - 1.0; // [0,1] --> [-1, 1]
    float sceneLinearDepth = (2.0 * zNear * zFar) / (zFar + zNear - ndc * (zFar - zNear));
    float waterDepth = max(sceneLinearDepth - vViewDepth, 0.0);
    float fogFactor = 1.0 - exp(-waterDepth * FOG_DENSITY);

    //* ===== REFRACTION =====
    vec3 viewDirView = mat3(view) * viewDir;
    vec3 normalView = mat3(view) * normal;

    float refractionScale = REFRACTION_STRENGTH * clamp(waterDepth / MAX_REFRACTION_DEPTH, 0.0, 1.0);
    vec2 distortedUV = screenUV + normal.xz * refractionScale;
    distortedUV = clamp(distortedUV, vec2(0.001), vec2(0.999));

    // reject the distortion if it lands on something in front of the water surface itself
    // (e.g. a wall poking up at the shore) -- that's not something the water should be
    // "seeing through", so fall back to the undistorted UV instead of showing that mismatch
    float distortedSceneDepthNDC = texture(solidDepth, distortedUV).r;
    float distortedNdc = distortedSceneDepthNDC * 2.0 - 1.0;
    float distortedLinearDepth =
        (2.0 * zNear * zFar) / (zFar + zNear - distortedNdc * (zFar - zNear));
    if (distortedLinearDepth < vViewDepth)
        distortedUV = screenUV;

    vec3 refractionColor = texture(solidColor, distortedUV).rgb;

    vec3 shallowColor = vec3(0.1, 0.35, 0.45);
    vec3 deepColor = vec3(0.02, 0.08, 0.15);
    vec3 tintedRefraction = mix(refractionColor, shallowColor, 0.1);
    vec3 baseColor = mix(tintedRefraction, deepColor, fogFactor);

    //* ===== REFLECTION =====
    RayHit reflectionHit = marchScene((view * vFragPosWorld).xyz, reflect(-viewDirView, normalView), 50, 0.1);

    vec3 fallBackColor =  sky(reflect(-viewDir, normal));
    // blend the fallback toward the water's own tone instead of the raw sky color, so the
    // transition near screen edges stays in the same tonal range as its surroundings
    vec3 edgeFallback = mix(baseColor, fallBackColor, 0.2);

    float edgeFade = smoothstep(0.0, 0.03, min(reflectionHit.uv.x, 1.0 - reflectionHit.uv.x))
               * smoothstep(0.0, 0.03, min(reflectionHit.uv.y, 1.0 - reflectionHit.uv.y));

    vec3 reflectionColor = reflectionHit.hit
        ? mix(edgeFallback, texture(solidColor, reflectionHit.uv).rgb, edgeFade)
        : fallBackColor;

    vec3 finalColor = mix(baseColor * lighting + specularLighting, reflectionColor, fresnel);

    FragColor = vec4(finalColor, 1.0);
}
