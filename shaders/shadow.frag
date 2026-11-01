#version 330 core
in vec3 worldPosition;
uniform int radialDepth;
uniform vec3 lightPosition;
uniform float shadowFar;
void main()
{
    gl_FragDepth = radialDepth != 0 ? length(worldPosition - lightPosition) / shadowFar : gl_FragCoord.z;
}
