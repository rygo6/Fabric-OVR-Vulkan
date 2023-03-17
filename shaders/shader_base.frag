#version 450

layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
//    outColor = vec4(fragColor * texture(texSampler, fragTexCoord).rgb, 1.0);
    outColor = texture(texSampler, fragTexCoord);

//    if (outColor.a <= 0)
//        discard;
}
