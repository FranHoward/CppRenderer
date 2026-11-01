#version 330 core
in vec3 worldPosition;
in vec3 worldNormal;
in vec3 surfaceAlbedo;
out vec4 fragmentColor;
uniform int lightType;
uniform vec3 lightPosition;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 attenuations;
uniform float cosInner;
uniform float cosOuter;
uniform int shadowsEnabled;
uniform sampler2D shadow2D;
uniform samplerCube shadowCube;
uniform mat4 lightMatrix;
uniform float shadowFar;
uniform float shadowBias;

float visibility(vec3 n, vec3 l)
{
    if (shadowsEnabled == 0) return 1.0;
    float bias = shadowBias * (1.0 + 2.0 * (1.0 - max(dot(n, l), 0.0)));
    if (lightType != 0)
    {
        vec3 offset = worldPosition - lightPosition;
        float closest = texture(shadowCube, offset).r * shadowFar;
        // A cube texel covers more world space farther from the light. At grazing
        // angles its radial depth also changes rapidly across a receiver plane.
        // Account for both instead of using a fixed bias (which causes acne).
        float footprint = 2.0 * length(offset) / float(textureSize(shadowCube, 0).x);
        bias = max(bias, footprint / max(dot(n, l), 0.1));
        return length(offset) - bias > closest ? 0.0 : 1.0;
    }
    vec4 projected = lightMatrix * vec4(worldPosition, 1.0);
    vec3 p = projected.xyz / projected.w * 0.5 + 0.5;
    if (p.z > 1.0 || p.z < 0.0 || any(lessThan(p.xy, vec2(0.0))) ||
        any(greaterThan(p.xy, vec2(1.0)))) return 1.0;
    // 3x3 PCF, with bias converted from world units to directional depth units.
    vec2 texel = 1.0 / vec2(textureSize(shadow2D, 0));
    // Cover the neighboring PCF texels as well as receiver slope. The orthographic
    // shadow width is half shadowFar (see BuildShadows).
    float footprint = shadowFar * 0.5 * texel.x;
    bias = max(bias, 2.5 * footprint / max(dot(n, l), 0.1));
    float sum = 0.0;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            sum += p.z - bias / shadowFar <= texture(shadow2D, p.xy + vec2(x, y) * texel).r ? 1.0 : 0.0;
    return sum / 9.0;
}

void main()
{
    vec3 n = normalize(worldNormal);
    vec3 l = -lightDirection;
    float attenuation = 1.0;
    if (lightType != 0)
    {
        vec3 offset = lightPosition - worldPosition;
        float distanceToLight = length(offset);
        l = offset / max(distanceToLight, 0.00001);
        attenuation = 1.0 / max(dot(attenuations,
            vec3(distanceToLight * distanceToLight, distanceToLight, 1.0)), 0.00001);
        if (lightType == 2)
            attenuation *= clamp((dot(-l, lightDirection) - cosOuter) / (cosInner - cosOuter), 0.0, 1.0);
    }
    float cosine = max(dot(n, l), 0.0);
    float visible = cosine > 0.0 ? visibility(n, l) : 0.0;
    // Same Lambert BRDF and linear output as the CPU direct-lighting path.
    fragmentColor = vec4(surfaceAlbedo / 3.141592653589793 * lightColor * attenuation * cosine * visible, 1.0);
}
