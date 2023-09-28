layout (set = 1, binding = 0) uniform NodeUBO {
        mat4 view;
        mat4 proj;
        mat4 invView;
        mat4 invProj;
        int width;
        int height;
} nodeUBO;