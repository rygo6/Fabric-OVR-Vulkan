#version 450

layout (set = 0, binding = 0) uniform GlobalUBO {
    mat4 view;
    mat4 proj;
} globalUBO;

layout(set = 3, binding = 0) uniform ObjectUBO {
    mat4 model;
} objectUBO;

layout (set = 3, binding = 1) uniform NodeUBO {
    mat4 view;
    mat4 proj;
} nodeUBO;

layout (set = 3, binding = 2) uniform sampler2D color;
layout (set = 3, binding = 3) uniform sampler2D normal;
layout (set = 3, binding = 4) uniform sampler2D depth;

layout(quads, equal_spacing, cw) in;

layout (location = 0) in vec3 inNormal[];
layout (location = 1) in vec2 inUV[];

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;

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

    float alphaValue = texture(color, outUV).a;
    float depthValue = texture(depth, outUV).r;

    /// this doesn't really work
//    if (alphaValue == 0) {
//        float span = 1.0 / 100.0;
//
//        vec2 uvStep[8] = vec2[](
//            vec2(outUV.x, outUV.y + span), //N
//            vec2(outUV.x + span, outUV.y + span), //NE
//            vec2(outUV.x + span, outUV.y), //E
//            vec2(outUV.x + span, outUV.y - span), //SE
//            vec2(outUV.x, outUV.y - span), //S
//            vec2(outUV.x - span, outUV.y - span), //SW
//            vec2(outUV.x - span, outUV.y), //W
//            vec2(outUV.x - span, outUV.y + span)//NW
//        );
//
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

    // Calculate clip space coordinates
    vec4 clipPos = vec4(outUV * 2.0 - 1.0, depthValue, 1.0);
    // Convert clip space coordinates to eye space coordinates
    vec4 eyePos = inverse(nodeUBO.proj) * clipPos;
    // Divide by w component to obtain normalized device coordinates
    vec3 ndcPos = eyePos.xyz / eyePos.w;
    // Convert NDC coordinates to world space coordinates
    vec4 worldPos = inverse(nodeUBO.view) * vec4(ndcPos, 1.0);


    gl_Position = alphaValue > 0 ?
        globalUBO.proj * globalUBO.view * worldPos :
        globalUBO.proj * globalUBO.view * objectUBO.model * pos;

//    gl_Position = globalUBO.proj * globalUBO.view * objectUBO.model * pos;
}