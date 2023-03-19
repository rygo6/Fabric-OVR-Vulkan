#include "fbr_pipelines.h"
#include "fbr_descriptors.h"
#include "fbr_vulkan.h"
#include "fbr_log.h"
#include "fbr_mesh.h"

static VkResult createPipelineLayout(const FbrVulkan *pVulkan,
                                     uint32_t setLayoutCount,
                                     const VkDescriptorSetLayout *pSetLayouts,
                                     VkPipelineLayout *pipelineLayout) {
    const VkPushConstantRange pushConstant = {
            .offset = 0,
            .size = sizeof(mat4),
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .setLayoutCount = setLayoutCount,
            .pSetLayouts = pSetLayouts,
            .pPushConstantRanges = &pushConstant,
            .pushConstantRangeCount  = 1
    };
    VK_CHECK(vkCreatePipelineLayout(pVulkan->device, &pipelineLayoutInfo, NULL, pipelineLayout));
}

static VkResult createPipelineLayouts(const FbrVulkan *pVulkan,
                                  const FbrDescriptors *pDescriptors,
                                  FbrPipelines *pPipelines) {

    VK_CHECK(createPipelineLayout(pVulkan,1,&pDescriptors->setLayout_uvUBO_ufSampler,&pPipelines->pipeLayout_pvMat4_uvUBO_ufSampler));

}


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

static VkResult createShaderModule(const FbrVulkan *pVulkan,
                                   const char *code,
                                   const uint32_t codeLength,
                                   VkShaderModule *pShaderModule) {
    VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = codeLength,
            .pCode = (const uint32_t *) code,
    };
    VK_CHECK(vkCreateShaderModule(pVulkan->device, &createInfo, NULL, pShaderModule));
}

VkResult fbrCreatePipe_pvMat4_uvUBO_ufSampler_ivVertex(const FbrVulkan *pVulkan,
                                                       FbrPipeLayout_pvMat4_uvUBO_ufSampler pipeLayout_pvMat4_uvUBO_ufSampler,
                                                       const char *pVertShaderPath,
                                                       const char *pFragShaderPath,
                                                       FbrPipe_pvMat4_uvUBO_ufSampler_ivVertex *pPipeline) {

    // Shaders
    uint32_t vertLength;
    char *vertShaderCode = readBinaryFile(pVertShaderPath, &vertLength);
    uint32_t fragLength;
    char *fragShaderCode = readBinaryFile(pFragShaderPath, &fragLength);
    VkShaderModule vertShaderModule;
    VK_CHECK(createShaderModule(pVulkan, vertShaderCode, vertLength, &vertShaderModule));
    VkShaderModule fragShaderModule;
    VK_CHECK(createShaderModule(pVulkan, fragShaderCode, fragLength, &fragShaderModule));
    const VkPipelineShaderStageCreateInfo shaderStages[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                    .module = vertShaderModule,
                    .pName = "main",
            },
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = fragShaderModule,
                    .pName = "main",
            }
    };

    // Vertex Input
    const VkVertexInputBindingDescription bindingDescription[1] = {
            {
                    .binding = 0,
                    .stride = sizeof(Vertex),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            }
    };
    const VkVertexInputAttributeDescription attributeDescriptions[3] = {
            {
                    .binding = 0,
                    .location = 0,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
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
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(Vertex, texCoord),
            }
    };
    const VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .vertexAttributeDescriptionCount = 3,
            .pVertexBindingDescriptions = bindingDescription,
            .pVertexAttributeDescriptions = attributeDescriptions
    };
    const VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
    };

    // Fragment
    const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable = VK_FALSE,
    };
    const VkPipelineColorBlendStateCreateInfo colorBlending = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = NULL,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments = &colorBlendAttachment,
            .blendConstants[0] = 0.0f,
            .blendConstants[1] = 0.0f,
            .blendConstants[2] = 0.0f,
            .blendConstants[3] = 0.0f,
    };

    // Rasterizing
    const VkPipelineRasterizationStateCreateInfo rasterizer = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.0f,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
    };
    const VkPipelineMultisampleStateCreateInfo multisampling = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .sampleShadingEnable = VK_FALSE,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    // Viewport/Scissor
    const VkPipelineViewportStateCreateInfo viewportState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1,
    };
    const VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };
    const VkPipelineDynamicStateCreateInfo dynamicState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates = dynamicStates,
    };

    // Robustness
    const VkPipelineRobustnessCreateInfoEXT pipelineRobustnessCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_ROBUSTNESS_CREATE_INFO_EXT,
            .pNext = NULL,
            .storageBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2_EXT,
            .uniformBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2_EXT,
            .vertexInputs = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2_EXT,
            .images = VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS_2_EXT,
    };

    // Create
    const VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &pipelineRobustnessCreateInfo,
            .stageCount = 2,
            .pStages = shaderStages,
            .pVertexInputState = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState = &multisampling,
            .pColorBlendState = &colorBlending,
            .pDynamicState = &dynamicState,
            .layout = pipeLayout_pvMat4_uvUBO_ufSampler,
            .renderPass = pVulkan->renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
    };
    VK_CHECK(vkCreateGraphicsPipelines(pVulkan->device,VK_NULL_HANDLE,1,&pipelineInfo,NULL,pPipeline));

    // Cleanup
    vkDestroyShaderModule(pVulkan->device, fragShaderModule, NULL);
    vkDestroyShaderModule(pVulkan->device, vertShaderModule, NULL);
    free(vertShaderCode);
    free(fragShaderCode);
}

static VkResult createPipes(const FbrVulkan *pVulkan,
                        FbrPipelines *pPipes) {

    VK_CHECK(fbrCreatePipe_pvMat4_uvUBO_ufSampler_ivVertex(pVulkan,
                                                  pPipes->pipeLayout_pvMat4_uvUBO_ufSampler,
                                                  "./shaders/vert.spv",
                                                  "./shaders/frag.spv",
                                                  &pPipes->pipe_pvMat4_uvCamera_ufTexture_ivVertex));
}

VkResult fbrCreatePipelines(const FbrVulkan *pVulkan,
                                const FbrDescriptors *pDescriptors,
                                FbrPipelines **ppAllocPipes) {
    *ppAllocPipes = calloc(1, sizeof(FbrPipelines));
    FbrPipelines *pPipes = *ppAllocPipes;

    VK_CHECK(createPipelineLayouts(pVulkan, pDescriptors, pPipes));
    VK_CHECK(createPipes(pVulkan, pPipes));
}

VkResult fbrDestroyPipelines(const FbrVulkan *pVulkan,
                             FbrPipelines *pPipelines) {
    vkDestroyPipelineLayout(pVulkan->device, pPipelines->pipeLayout_pvMat4_uvUBO_ufSampler, NULL);
    vkDestroyPipeline(pVulkan->device, pPipelines->pipe_pvMat4_uvCamera_ufTexture_ivVertex, NULL);

    free(pPipelines);
}