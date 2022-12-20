#include "fbr_pipeline.h"
#include "fbr_mesh.h"
#include "fbr_texture.h"
#include "fbr_camera.h"
#include "fbr_log.h"
#include "fbr_vulkan.h"

#include <memory.h>

static char *readBinaryFile(const char *filename, uint32_t *length) {
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        FBR_LOG_DEBUG("File can't be opened!", filename);
    }

    fseek(file, 0, SEEK_END);
    *length = ftell(file);
    rewind(file);
    char *contents = calloc(1 + *length, sizeof(char));
    size_t readCount = fread(contents, *length, 1, file);
    if (readCount == 0) {
        FBR_LOG_DEBUG("Failed to read file!", filename);
    }
    fclose(file);

    return contents;
}

static VkShaderModule createShaderModule(const FbrVulkan *pVulkan, const char *code, const uint32_t codeLength) {
    VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = codeLength,
            .pCode = (const uint32_t *) code,
    };

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(pVulkan->device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        FBR_LOG_DEBUG("Failed to create shader module!", code);
    }

    return shaderModule;
}

static void initDescriptorSetLayout(const FbrVulkan *pVulkan, FbrPipeline *pPipeline) {
    VkDescriptorSetLayoutBinding uboLayoutBinding = {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            // we use it from the vertex shader
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {
            .binding = 1,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImmutableSamplers = NULL,
            .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
    };

    const uint32_t bindingsCount = 2;
    VkDescriptorSetLayoutBinding bindings[] = {uboLayoutBinding, samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = bindingsCount,
            .pBindings = bindings,
    };

    if (vkCreateDescriptorSetLayout(pVulkan->device, &layoutInfo, NULL, &pPipeline->descriptorSetLayout) != VK_SUCCESS) {
        FBR_LOG_DEBUG("Failed to create descriptor set layout!");
    }
}

static void initPipelineLayout(const FbrVulkan *pVulkan, FbrPipeline *pPipeline) {
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &pPipeline->descriptorSetLayout,
    };

    if (vkCreatePipelineLayout(pVulkan->device, &pipelineLayoutInfo, NULL, &pPipeline->pipelineLayout) != VK_SUCCESS) {
        FBR_LOG_DEBUG("Failed to create pipeline layout!");
    }
}

static void initDescriptorSets(const FbrVulkan *pVulkan, const FbrCamera *pCameraState, const FbrTexture *pTexture, FbrPipeline *pPipeline) {
    VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = pVulkan->descriptorPool,
            .descriptorSetCount = FBR_DESCRIPTOR_SET_COUNT,
            .pSetLayouts = &pPipeline->descriptorSetLayout,
    };

    if (vkAllocateDescriptorSets(pVulkan->device, &allocInfo, &pPipeline->descriptorSet) != VK_SUCCESS) {
        FBR_LOG_DEBUG("Failed to allocate descriptor sets!");
    }

    VkDescriptorBufferInfo bufferInfo = {
            .buffer = pCameraState->mvpUBO.uniformBuffer,
            .offset = 0,
            .range = sizeof(FbrMVP),
    };

    VkDescriptorImageInfo imageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = pTexture->imageView,
            .sampler = pTexture->sampler,
    };

    VkWriteDescriptorSet descriptorWrites[FBR_DESCRIPTOR_COUNT] = {
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = pPipeline->descriptorSet,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .pBufferInfo = &bufferInfo,
            },
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = pPipeline->descriptorSet,
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .pImageInfo = &imageInfo,
            },
    };

    vkUpdateDescriptorSets(pVulkan->device, FBR_DESCRIPTOR_COUNT, descriptorWrites, 0, NULL);
}

static void initPipeline(const FbrVulkan *pVulkan, FbrPipeline *pPipeline) {
    uint32_t vertLength;
    char *vertShaderCode = readBinaryFile("./shaders/vert.spv", &vertLength);
    uint32_t fragLength;
    char *fragShaderCode = readBinaryFile("./shaders/frag.spv", &fragLength);

    VkShaderModule vertShaderModule = createShaderModule(pVulkan, vertShaderCode, vertLength);
    VkShaderModule fragShaderModule = createShaderModule(pVulkan, fragShaderCode, fragLength);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vertShaderModule,
            .pName = "main",
    };

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = fragShaderModule,
            .pName = "main",
    };

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkVertexInputBindingDescription bindingDescription[FBR_BINDING_DESCRIPTOR_COUNT] = {
            {
                    .binding = 0,
                    .stride = sizeof(Vertex),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            }
    };

    VkVertexInputAttributeDescription attributeDescriptions[FBR_ATTRIBUTE_DESCRIPTOR_COUNT] = {
            {
                    .binding = 0,
                    .location = 0,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(Vertex, pos),
            },
            {
                    .binding = 0,
                    .location = 1,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, color),
            },
            {
                    .binding = 0,
                    .location = 2,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, texCoord),
            }
    };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = FBR_BINDING_DESCRIPTOR_COUNT,
            .vertexAttributeDescriptionCount = FBR_ATTRIBUTE_DESCRIPTOR_COUNT,
            .pVertexBindingDescriptions = bindingDescription,
            .pVertexAttributeDescriptions = attributeDescriptions
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineViewportStateCreateInfo viewportState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1,
    };

    VkPipelineRasterizationStateCreateInfo rasterizer = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.0f,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
    };

    VkPipelineMultisampleStateCreateInfo multisampling = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .sampleShadingEnable = VK_FALSE,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable = VK_FALSE,
    };

    VkPipelineColorBlendStateCreateInfo colorBlending = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
            .blendConstants[0] = 0.0f,
            .blendConstants[1] = 0.0f,
            .blendConstants[2] = 0.0f,
            .blendConstants[3] = 0.0f,
    };

    uint32_t dynamicStateCount = 2;
    VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };
    VkPipelineDynamicStateCreateInfo dynamicState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = dynamicStateCount,
            .pDynamicStates = dynamicStates,
    };

    VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = pPipeline->pipelineLayout,
            .renderPass = pVulkan->renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
    };

    if (vkCreateGraphicsPipelines(pVulkan->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pPipeline->graphicsPipeline) != VK_SUCCESS) {
        FBR_LOG_DEBUG("Failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(pVulkan->device, fragShaderModule, NULL);
    vkDestroyShaderModule(pVulkan->device, vertShaderModule, NULL);
    free(vertShaderCode);
    free(fragShaderCode);
}

void fbrCreatePipeline(const FbrVulkan *pVulkan,
                       const FbrCamera *pCameraState,
                       const FbrTexture *pTexture,
                       FbrPipeline **ppAllocPipeline) {
    *ppAllocPipeline = calloc(1, sizeof(FbrPipeline));
    FbrPipeline *pPipeline = *ppAllocPipeline;
    initDescriptorSetLayout(pVulkan, pPipeline);
    initPipelineLayout(pVulkan, pPipeline);
    initPipeline(pVulkan, pPipeline);
    initDescriptorSets(pVulkan, pCameraState, pTexture, pPipeline);
}

void fbrCleanupPipeline(const FbrVulkan *pVulkan, FbrPipeline *pPipeline) {
    vkDestroyDescriptorSetLayout(pVulkan->device, pPipeline->descriptorSetLayout, NULL);
    vkDestroyPipeline(pVulkan->device, pPipeline->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(pVulkan->device, pPipeline->pipelineLayout, NULL);
    free(pPipeline);
}