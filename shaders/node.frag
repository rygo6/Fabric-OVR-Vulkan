#version 450

layout (set = 3, binding = 2) uniform sampler2D color;
layout (set = 3, binding = 3) uniform sampler2D normal;
layout (set = 3, binding = 4) uniform sampler2D depth;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outGBuffer;

void main()
{
    vec4 color = texture(color, inUV);
    vec4 nodeNormal = texture(normal, inUV);
    float nodeDepth = texture(depth, inUV).r;
    float fragDepth = gl_FragCoord.z;

    float depthDiff = fragDepth - nodeDepth;

//    if (color.a == 0)
//        outColor = vec4(fragDepth, fragDepth, fragDepth, 1);
//    else
//        outColor = vec4(depthDiff, depthDiff, depthDiff, 1);

    if (color.a > 0)
        outColor = color;
    else
        discard;

    // Is this right?! https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/subpasses/gbuffer.frag
    vec3 N = normalize(inNormal);
    N.y = -N.y;
    outNormal = vec4(N, 1.0);


//    float linearNodeDepth = linearize_depth(nodeDepth, 0.01, 10000.0f);
//    outGBuffer.rgb = nodeNormal;
//    outGBuffer.r = linearNodeDepth;
    outGBuffer.r = depthDiff;
    outGBuffer.a = outColor.a;
//    outGBuffer.a = linearNodeDepth;
}