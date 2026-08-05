#version 330 core

in float vAO;
in vec4 vFragPosWorld;
in float vViewDepth;

uniform vec3 lightDir;
uniform vec3 camPos;
uniform float time;
uniform float zNear;
uniform float zFar;

uniform sampler2D solidColor;
uniform sampler2D solidDepth;

const int CASCADE_COUNT = 5;

out vec4 FragColor;

const vec3 lightColor = vec3(242.0 / 255.0, 232.0 / 255.0, 174.0 / 255.0);

// must match water_vert.glsl's wave octaves exactly (same constants, same formula) -- this
// is what recovers the EXACT surface normal analytically, without ever sampling neighboring
// fragments (no finite differences): derivative of A*sin(k*dot(dir,xz) + speed*t) w.r.t.
// (x,z) is A*k*dir*cos(...), so the gradient is just a sum of cosines instead of a sum of
// sines. All 4 octaves are used here (unlike the vertex shader's 2), since a per-pixel
// normal has no under-sampling problem the way sparse block-corner vertices do.
const int WAVE_ITERATIONS = 5; // must match water_vert.glsl's WAVE_ITERATIONS
const float GOLDEN_ANGLE = 2.399963;

// must match water_vert.glsl's base/GAIN/LACUNARITY exactly, or this normal won't match
// the actual geometry displaced there.
const float BASE_AMPLITUDE = 0.2;
const float BASE_K = 0.5;
const float BASE_SPEED = 0.2;
const float GAIN = 0.5;       // amplitude multiplier per octave (aka persistence)
const float LACUNARITY = 1.4; // frequency (and speed) multiplier per octave

// scales the normal's response independently of the geometric wave height (BASE_AMPLITUDE
// above stays small so the vertex displacement looks subtle -- this makes the lighting
// react much more strongly to the same underlying waves without moving more geometry)
const float NORMAL_STRENGTH = 1.0;

const float FOG_DENSITY = 0.5;

const float REFRACTION_STRENGTH = 0.2;
// water shallower than this gets proportionally less UV distortion, so refraction fades
// out toward the shore instead of staying at full strength right up to the mesh edge
const float MAX_REFRACTION_DEPTH = 8.0;

// analytic gradient of the gerstner-wave sum in water_vert.glsl's waveHeight():
// h_i = A_i * s_i^2, s_i = sin(x_i)*0.5+0.5, x_i = w_i*t_i - k_i*dot(dir_i, coord)
// => dh_i/dcoord = -A_i * k_i * s_i * cos(x_i) * dir_i
// (note the minus sign here -- x_i subtracts k*dot(dir,coord), unlike the plain sine sum
// used before, which added it; get this backwards and the whole normal flips)
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

    grad *= NORMAL_STRENGTH;
    return normalize(vec3(-grad.x, 1.0, -grad.y));
}

void main()
{
    vec3 normal = waveNormal(vFragPosWorld.xz);
    vec3 viewDir = normalize(camPos - vFragPosWorld.xyz);
    vec3 reflectDir = reflect(lightDir, normal);

    vec2 screenUV = gl_FragCoord.xy / vec2(textureSize(solidColor, 0));
    // non linear depth --> needs to be linearized before comparing to vViewDepth
    float sceneDepthNDC = texture(solidDepth, screenUV).r; 
    float ndc = sceneDepthNDC * 2.0 - 1.0; // [0,1] --> [-1, 1]
    float sceneLinearDepth = (2.0 * zNear * zFar) / (zFar + zNear - ndc * (zFar - zNear));
    float waterDepth = max(sceneLinearDepth - vViewDepth, 0.0);
    float fogFactor = 1.0 - exp(-waterDepth * FOG_DENSITY);

    float specularCoeff = pow(max(dot(viewDir, reflectDir), 0.0), 64);
    vec3 specularLighting = specularCoeff * lightColor * 0.5f;

    float diffuseCoeff = max(dot(-lightDir, normal), 0.0);
    vec3 diffuseLighting = diffuseCoeff * lightColor;
    float ambientCoeff = 0.2;

    vec3 lighting = (ambientCoeff + diffuseCoeff) * lightColor;

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

    vec3 tintedRefraction = mix(refractionColor, shallowColor, 0.3);
    vec3 baseColor = mix(tintedRefraction, deepColor, fogFactor);

    FragColor = vec4(baseColor * lighting + specularLighting, 1.0);
}
