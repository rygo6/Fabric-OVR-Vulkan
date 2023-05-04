#version 450
layout (set = 3, binding = 2) uniform sampler2D color;
layout (set = 3, binding = 3) uniform sampler2D normal;
layout (set = 3, binding = 4) uniform sampler2D depth;

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;

layout (location = 0) out vec4 outColor;
layout (location = 1) out vec4 outNormal;

float linearize_depth(float d,float zNear,float zFar)
{
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

void main()
{
//    float alphaValue = texture(color, inUV).a;


//    float depthValue = texture(depth, inUV).r;
//    depthValue = linearize_depth(depthValue, 0.01, 10000.0f);
//    outColor = vec4(depthValue, depthValue, depthValue, 1.0);
    outColor = texture(color, inUV);
//    outColor = texture(normal, inUV);

    // Is this right?! https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/subpasses/gbuffer.frag
//    vec3 N = normalize(inNormal);
//    N.y = -N.y;
//    outNormal = vec4(N, 1.0);
    outNormal = texture(normal, inUV);
}