layout (set = 1, binding = 1) uniform SourceUBO {
        mat4 view;
        mat4 proj;
        int width;
        int height;
} sourceUBO;

layout (set = 1, binding = 2) uniform sampler2D sourceColor;
layout (set = 1, binding = 3) uniform sampler2D sourceCormal;
layout (set = 1, binding = 4) uniform sampler2D sourceGBuffer;
layout (set = 1, binding = 5) uniform sampler2D sourceDepth;