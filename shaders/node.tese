#version 450

layout (set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 proj;
} globalUBO;

layout(set = 3, binding = 0) uniform ObjectUBO {
    mat4 model;
} objectUBO;
layout (set = 3, binding = 1) uniform sampler2D depth;
//layout (set = 3, binding = 2) uniform sampler2D color;

layout(quads, equal_spacing, cw) in;

layout (location = 0) in vec3 inNormal[];
layout (location = 1) in vec2 inUV[];

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;

vec4 WorldPosFromDepth(float depth, vec2 uv) {
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(uv * 2.0 - 1.0, z, 1.0);

    mat4 projMatrixInv = inverse(globalUBO.proj);
    vec4 viewSpacePosition = projMatrixInv * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    mat4 viewMatrixInv = inverse(globalUBO.view);
    vec4 worldSpacePosition = viewMatrixInv * viewSpacePosition;

    return worldSpacePosition;
}

void main()
{
    outUV = mix(
        mix(inUV[0], inUV[1], gl_TessCoord.x),
        mix(inUV[3], inUV[2], gl_TessCoord.x),
        gl_TessCoord.y);

    outNormal = mix(
        mix(inNormal[0], inNormal[1], gl_TessCoord.x),
        mix(inNormal[3], inNormal[2], gl_TessCoord.x),
        gl_TessCoord.y);

    vec4 pos = mix(
        mix(gl_in[0].gl_Position, gl_in[1].gl_Position, gl_TessCoord.x),
        mix(gl_in[3].gl_Position, gl_in[2].gl_Position, gl_TessCoord.x),
        gl_TessCoord.y);


//    vec4 originCameraSpacePosition = globalUBO.view * objectUBO.model * vec4(0,0,0,0);

    float depth = textureLod(depth, outUV, 0.0).r;
//    vec4 cameraSpacePosition = globalUBO.view * objectUBO.model * pos;
//    cameraSpacePosition.z = depthToLinear(depth, 0, 1);
//    gl_Position = globalUBO.proj * cameraSpacePosition;
//    gl_Position.z = clipSpaceDepth(depth, 0, 1);
//    gl_Position.z = .5;

    vec4 worldPosDepth = WorldPosFromDepth(depth, outUV);
    gl_Position = globalUBO.proj * globalUBO.view * worldPosDepth;

}