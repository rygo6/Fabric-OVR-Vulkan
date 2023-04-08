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

layout (set = 3, binding = 2) uniform sampler2D depth;
//layout (set = 3, binding = 3) uniform sampler2D color;

layout(quads, equal_spacing, cw) in;

layout (location = 0) in vec3 inNormal[];
layout (location = 1) in vec2 inUV[];

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;

//vec3 calculate_world_position(vec2 texture_coordinate, float depth_from_depth_buffer)
//{
//    vec4 clip_space_position = vec4(texture_coordinate * 2.0 - vec2(1.0), 2.0 * depth_from_depth_buffer - 1.0, 1.0);
//
//    mat4 inverse_view_projection_matrix = inverse(nodeUBO.view * nodeUBO.proj);
//    //vec4 position = inverse_projection_matrix * clip_space_position; // Use this for view space
//    vec4 position = inverse_view_projection_matrix * clip_space_position; // Use this for world space
//
//    return (position.xyz / position.w);
//}

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

    // Sample the depth buffer at the current pixel coordinates
    float depth = texture(depth, outUV).r;

    // Calculate clip space coordinates
    vec4 clipPos = vec4(outUV * 2.0 - 1.0, depth, 1.0);

    // Convert clip space coordinates to eye space coordinates
    vec4 eyePos = inverse(nodeUBO.proj) * clipPos;

    // Divide by w component to obtain normalized device coordinates
    vec3 ndcPos = eyePos.xyz / eyePos.w;

    // Convert NDC coordinates to world space coordinates
    vec4 worldPos = inverse(nodeUBO.view) * vec4(ndcPos, 1.0);

//    vec3 worldPos = calculate_world_position(outUV, depth);

    gl_Position = globalUBO.proj * globalUBO.view * worldPos;

//    gl_Position = globalUBO.proj * globalUBO.view * objectUBO.model * pos;
}