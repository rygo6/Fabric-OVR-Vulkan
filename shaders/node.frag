#version 450
layout (set = 3, binding = 2) uniform sampler2D depth;
layout (set = 3, binding = 3) uniform sampler2D color;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outFragColor;

void main()
{
    outFragColor = texture(color, inUV);
}