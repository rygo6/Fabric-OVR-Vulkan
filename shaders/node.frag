#version 450
layout (set = 3, binding = 2) uniform sampler2D depth;
layout (set = 3, binding = 3) uniform sampler2D color;

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

//    if (alphaValue == 0) {
//        float span = 1.0 / 10.0;

//        vec2 uvStep[8] = vec2[](
//        vec2(inUV.x, inUV.y + span), //N
//        vec2(inUV.x + span, inUV.y + span), //NE
//        vec2(inUV.x + span, inUV.y), //E
//        vec2(inUV.x + span, inUV.y - span), //SE
//        vec2(inUV.x, inUV.y - span), //S
//        vec2(inUV.x - span, inUV.y - span), //SW
//        vec2(inUV.x - span, inUV.y), //W
//        vec2(inUV.x - span, inUV.y + span)//NW
//        );

//        int addedCount = 0;
//        for (int i = 0; i < 8; ++i) {
//            float subAlpha = texture(color, uvStep[i]).a;
//            float subDepth = texture(depth, uvStep[i]).r;
//            if (subAlpha != 0) {
//                if (addedCount == 0) {
//                    depthValue = subDepth;
//                }
//                else{
//                    depthValue += subDepth;
//                }
//                addedCount++;
//            }
//        }
//        depthValue /= addedCount;
//    }

//    depthValue = linearize_depth(depthValue, 0.01, 10000.0f);
//    outFragColor = vec4(depthValue, depthValue, depthValue, 1.0);
    outColor = texture(color, inUV);

    // Is this right?! https://github.com/SaschaWillems/Vulkan/blob/master/data/shaders/glsl/subpasses/gbuffer.frag
    vec3 N = normalize(inNormal);
    N.y = -N.y;
    outNormal = vec4(N, 1.0);
}