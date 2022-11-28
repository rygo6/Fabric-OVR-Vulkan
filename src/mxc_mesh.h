//
// Created by rygo6 on 11/21/2022.
//

#ifndef MOXAIC_MESH_H
#define MOXAIC_MESH_H

#include <vulkan/vulkan.h>
#include "cglm/cglm.h"

typedef struct Vertex {
    vec2 pos;
    vec3 color;
} Vertex;

const uint32_t verticesCount = 4;
const Vertex vertices[] = {
        {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}}
};

const uint32_t indicesCount = 6;
const uint16_t indices[] = {
        0, 1, 2, 2, 3, 0
};

const int attributeDescriptionCount = 2;

VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    return bindingDescription;
}

void getAttributeDescriptions(VkVertexInputAttributeDescription attributeDescriptions[attributeDescriptionCount]) {
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);
}

#endif //MOXAIC_MXC_MESH_H
