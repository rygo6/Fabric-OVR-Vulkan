#version 450

layout (set = 1, binding = 1) uniform sampler2D nodeColor;

layout (location = 0) in VertexInput {
    vec2 uv;
} vertexInput;

layout(location = 0) out vec4 outFragColor;

void main()
{
    const vec4 colorValue = texture(nodeColor, vertexInput.uv);
//    if (vertexInput.color.a < .99)
//        discard;
    outFragColor = colorValue;
}