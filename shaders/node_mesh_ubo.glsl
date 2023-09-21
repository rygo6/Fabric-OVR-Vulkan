layout (set = 1, binding = 0) uniform NodeUBO {
        mat4 view;
        mat4 proj;
        mat4 invView;
        mat4 invProj;
        int width;
        int height;
} nodeUBO;

layout (set = 1, binding = 1) uniform sampler2D nodeColor;
layout (set = 1, binding = 2) uniform sampler2D nodeNormal;
layout (set = 1, binding = 3) uniform sampler2D nodeGBuffer;
layout (set = 1, binding = 4) uniform sampler2D nodeDepth;