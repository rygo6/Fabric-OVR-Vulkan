#version 450

layout(set = 3, binding = 0) uniform ObjectUBO {
    mat4 model;
} objectUBO;

layout (set = 3, binding = 1) uniform NodeUBO {
    mat4 view;
    mat4 proj;
} nodeUBO;

layout (vertices = 4) out;

layout (location = 0) in vec3 inNormal[];
layout (location = 1) in vec2 inUV[];

layout (location = 0) out vec3 outNormal[4];
layout (location = 1) out vec2 outUV[4];

void main()
{
    if (gl_InvocationID == 0)
    {
        float tessellationFactor = 64;
        gl_TessLevelOuter[0] = tessellationFactor;
        gl_TessLevelOuter[1] = tessellationFactor;
        gl_TessLevelOuter[2] = tessellationFactor;
        gl_TessLevelOuter[3] = tessellationFactor;

        gl_TessLevelInner[0] = tessellationFactor;
        gl_TessLevelInner[1] = tessellationFactor;
    }

    gl_out[gl_InvocationID].gl_Position =  gl_in[gl_InvocationID].gl_Position;
    outNormal[gl_InvocationID] = inNormal[gl_InvocationID];
    outUV[gl_InvocationID] = inUV[gl_InvocationID];
}