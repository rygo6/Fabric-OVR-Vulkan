#version 450

layout (location = 0) in VertexInput {
    vec4 color;
} vertexInput;

layout(location = 0) out vec4 outFragColor;


void main()
{
//    if (vertexInput.color.a < .99)
//        discard;
    outFragColor = vertexInput.color;
}