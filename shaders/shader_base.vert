#version 450

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 proj;
} globalUBO;

layout(set = 3, binding = 0) uniform ObjectUBO {
    mat4 model;
} objectUBO;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = globalUBO.proj * globalUBO.view * objectUBO.model * vec4(inPosition, 1.0);
    gl_Position.z = .5;

    fragColor = inColor;

    fragTexCoord = inTexCoord;
}