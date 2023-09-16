layout(set = 0, binding = 0) uniform GlobalUBO {
        mat4 view;
        mat4 proj;
        int width;
        int height;
} globalUBO;