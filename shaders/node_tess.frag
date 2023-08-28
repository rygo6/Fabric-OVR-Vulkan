#version 450

layout (set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 proj;
} globalUBO;

layout (set = 3, binding = 1) uniform NodeUBO {
    mat4 view;
    mat4 proj;
} nodeUBO;

layout (set = 3, binding = 2) uniform sampler2D color;
layout (set = 3, binding = 3) uniform sampler2D normal;
layout (set = 3, binding = 4) uniform sampler2D depth;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec4 inWorldPos;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outGBuffer;

void main()
{
    vec4 color = texture(color, inUV);
    vec4 nodeNormal = texture(normal, inUV);
    float nodeDepth = texture(depth, inUV).r;

    vec4 clipPos = vec4(inUV * 2.0 - 1.0,  nodeDepth, 1.0);
    vec4 eyePos = inverse(nodeUBO.proj) * clipPos;
    vec3 ndcPos = eyePos.xyz / eyePos.w;
    vec4 worldPos = inverse(nodeUBO.view) * vec4(ndcPos, 1.0);

    outColor = color;
//    outColor = vec4(nodeDepth,nodeDepth,nodeDepth,1);

    // Is this right?! https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/subpasses/gbuffer.frag
    vec3 N = normalize(inNormal);
    N.y = -N.y;
    outNormal = vec4(N, 1.0);

    outGBuffer.rgb = worldPos.rgb;
    outGBuffer.a = color.a;
}