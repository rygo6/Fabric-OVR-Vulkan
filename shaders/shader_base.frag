#version 450

#define FRAME_WIDTH 1920
#define FRAME_HEIGHT 1080

layout(set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 proj;
    int width;
    int height;
} globalUBO;

//layout(set = 1, binding = 0) uniform sampler2D normal;

layout(set = 2, binding = 0) uniform sampler2D texSampler;

layout(set = 3, binding = 0) uniform ObjectUBO {
    mat4 model;
} objectUBO;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV;
layout(location = 2) in vec3 inWorldPos;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;
layout(location = 2) out vec4 outGBuffer;

void main()
{
    float depth = gl_FragCoord.z;
    vec2 normalizedFragCoord = vec2(gl_FragCoord.x / FRAME_WIDTH, gl_FragCoord.y / FRAME_HEIGHT);
    vec4 clipPos = vec4(normalizedFragCoord * 2.0 - 1.0,  depth, 1.0);
    vec4 eyePos = inverse(globalUBO.proj) * clipPos;
    vec3 ndcPos = eyePos.xyz / eyePos.w;
    vec4 worldPos = inverse(globalUBO.view) * vec4(ndcPos, 1.0);

    outColor = texture(texSampler, inUV);
//    outColor = vec4(inWorldPos, 1.0);
//    outColor = vec4(worldPos.xyz, 1);

    // Is this right?! https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/subpasses/gbuffer.frag
    vec3 N = normalize(inNormal);
    N.y = -N.y;
    outNormal = vec4(N, 1.0);

    outGBuffer = vec4(0,0,0,0);
}
