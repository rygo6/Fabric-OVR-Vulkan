#version 450

//layout(set = 1, binding = 0) uniform sampler2D normal;

layout(set = 2, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inUV;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outNormal;

void main() {
    outColor = texture(texSampler, inUV);
//    outColor = texture(normal, inUV);

    // Is this right?! https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/subpasses/gbuffer.frag
    vec3 N = normalize(inNormal);
    N.y = -N.y;
    outNormal = vec4(N, 1.0);
//    outColor = outNormal;
}
