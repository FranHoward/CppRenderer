#version 330 core
layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 albedo;
uniform mat4 viewProjection;
out vec3 worldPosition;
out vec3 worldNormal;
out vec3 surfaceAlbedo;
void main()
{
    worldPosition = position;
    worldNormal = normal;
    surfaceAlbedo = albedo;
    gl_Position = viewProjection * vec4(position, 1.0);
}
