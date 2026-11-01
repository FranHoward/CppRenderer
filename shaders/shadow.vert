#version 330 core
layout(location = 0) in vec3 position;
uniform mat4 lightMatrix;
out vec3 worldPosition;
void main()
{
    worldPosition = position;
    gl_Position = lightMatrix * vec4(position, 1.0);
}
