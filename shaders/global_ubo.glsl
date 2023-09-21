layout(set = 0, binding = 0) uniform GlobalUBO {
        mat4 view;
        mat4 proj;
        mat4 invView;
        mat4 invProj;
        int width;
        int height;
} globalUBO;