#include "fbr_pipelines.h"
#include "fbr_descriptors.h"
#include "fbr_vulkan.h"
#include "fbr_log.h"
#include "fbr_mesh.h"

static VkResult createPipeLayoutStandard(const FbrVulkan *pVulkan,
                                         const FbrDescriptors *pDescriptors,
                                         FbrPipeLayoutStandard *pipelineLayout)
{
    const VkDescriptorSetLayout pSetLayouts[] = {
            pDescriptors->setLayoutGlobal,
            pDescriptors->setLayoutPass,
            pDescriptors->setLayoutMaterial,
            pDescriptors->setLayoutObject
    };
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .setLayoutCount = 4,
            .pSetLayouts = pSetLayouts,
            .pPushConstantRanges = NULL,
            .pushConstantRangeCount  = 0
    };
    VK_CHECK(vkCreatePipelineLayout(pVulkan->device,
                                    &pipelineLayoutInfo,
                                    FBR_ALLOCATOR,
                                    pipelineLayout));

    return VK_SUCCESS;
}

static VkResult createPipeLayoutNode(const FbrVulkan *pVulkan,
                                     const FbrDescriptors *pDescriptors,
                                     FbrPipeLayoutNode *pPipeLayout)
{
    const VkDescriptorSetLayout pSetLayouts[] = {
            pDescriptors->setLayoutGlobal,
            pDescriptors->setLayoutPass,
            pDescriptors->setLayoutMaterial,
            pDescriptors->setLayoutNode
    };
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .setLayoutCount = 4,
            .pSetLayouts = pSetLayouts,
            .pPushConstantRanges = NULL,
            .pushConstantRangeCount  = 0
    };
    VK_CHECK(vkCreatePipelineLayout(pVulkan->device,
                                    &pipelineLayoutInfo,
                                    FBR_ALLOCATOR,
                                    pPipeLayout));

    return VK_SUCCESS;
}

static VkResult createPipelineLayouts(const FbrVulkan *pVulkan,
                                      const FbrDescriptors *pDescriptors,
                                      FbrPipelines *pPipelines)
{
    FBR_ACK(createPipeLayoutStandard(pVulkan,
                                      pDescriptors,
                                      &pPipelines->pipeLayoutStandard))
    FBR_ACK(createPipeLayoutNode(pVulkan,
                                      pDescriptors,
                                      &pPipelines->pipeLayoutNode))

    return VK_SUCCESS;
}


static VkResult allocReadFile(const char *filename,
                              uint32_t *length,
                              char **ppContents)
{
    FILE *file = fopen(filename, "rb");

    if (file == NULL) {
        FBR_LOG_DEBUG("File can't be opened!", filename);
        return VK_ERROR_UNKNOWN;
    }

    fseek(file, 0, SEEK_END);
    *length = ftell(file);
    rewind(file);
    *ppContents = calloc(1 + *length, sizeof(char));
    size_t readCount = fread(*ppContents, *length, 1, file);
    if (readCount == 0) {
        FBR_LOG_DEBUG("Failed to read file!", filename);
        return VK_ERROR_UNKNOWN;
    }
    fclose(file);

    return VK_SUCCESS;
}

static FBR_RESULT createShaderModule(const FbrVulkan *pVulkan,
                                   const char *pShaderPath,
                                   VkShaderModule *pShaderModule)
{
    uint32_t codeLength;
    char *pShaderCode;
    FBR_ACK(allocReadFile(pShaderPath,
                           &codeLength,
                           &pShaderCode))

    VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = codeLength,
            .pCode = (const uint32_t *) pShaderCode,
    };
    FBR_ACK(vkCreateShaderModule(pVulkan->device,
                                  &createInfo,
                                  FBR_ALLOCATOR,
                                  pShaderModule))

    free(pShaderCode);

    FBR_SUCCESS
}

static VkResult createOpaqueTrianglePipe(const FbrVulkan *pVulkan,
                                         VkPipelineLayout layout,
                                         VkPrimitiveTopology topology,
                                         uint32_t stageCount,
                                         const VkPipelineShaderStageCreateInfo *pStages,
                                         const VkPipelineTessellationStateCreateInfo *pTessellationState,
                                         VkPipeline *pPipe)
{
    // Vertex Input
    const VkVertexInputBindingDescription pVertexBindingDescriptions[1] = {
            {
                    .binding = 0,
                    .stride = sizeof(Vertex),
                    .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            }
    };
    const VkVertexInputAttributeDescription pVertexAttributeDescriptions[3] = {
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
                    .offset = offsetof(Vertex, normal),
            },
            {
                    .binding = 0,
                    .location = 2,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(Vertex, uv),
            }
    };
    const VkPipelineVertexInputStateCreateInfo vertexInputState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .vertexAttributeDescriptionCount = 3,
            .pVertexBindingDescriptions = pVertexBindingDescriptions,
            .pVertexAttributeDescriptions = pVertexAttributeDescriptions
    };
    const VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = topology,
            .primitiveRestartEnable = VK_FALSE,
    };

    // Fragment
    const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                              VK_COLOR_COMPONENT_G_BIT |
                              VK_COLOR_COMPONENT_B_BIT |
                              VK_COLOR_COMPONENT_A_BIT,
            .blendEnable = VK_FALSE,
    };
    const VkPipelineColorBlendStateCreateInfo colorBlendState = {
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
    const VkPipelineRasterizationStateCreateInfo rasterizationState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode = VK_POLYGON_MODE_FILL,
//            .polygonMode = VK_POLYGON_MODE_LINE,
            .lineWidth = 1.0f,
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
    };
    const VkPipelineMultisampleStateCreateInfo multisampleState = {
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
            .flags = 0,
            .stageCount = stageCount,
            .pStages = pStages,
            .pVertexInputState = &vertexInputState,
            .pInputAssemblyState = &inputAssemblyState,
            .pTessellationState = pTessellationState,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizationState,
            .pMultisampleState = &multisampleState,
            .pDepthStencilState = NULL,
            .pColorBlendState = &colorBlendState,
            .pDynamicState = &dynamicState,
            .layout = layout,
            .renderPass = pVulkan->renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
            .basePipelineIndex = 0,
    };
    FBR_ACK(vkCreateGraphicsPipelines(pVulkan->device,
                                      VK_NULL_HANDLE,
                                      1,
                                      &pipelineInfo,
                                      NULL,
                                      pPipe))

    return VK_SUCCESS;
}

VkResult fbrCreatePipeStandard(const FbrVulkan *pVulkan,
                               FbrPipeLayoutStandard layout,
                               const char *pVertShaderPath,
                               const char *pFragShaderPath,
                               FbrPipeStandard *pPipe)
{
    VkShaderModule vertShaderModule;
    VK_CHECK(createShaderModule(pVulkan,
                                pVertShaderPath,
                                &vertShaderModule));
    VkShaderModule fragShaderModule;
    VK_CHECK(createShaderModule(pVulkan,
                                pFragShaderPath,
                                &fragShaderModule));
    const VkPipelineShaderStageCreateInfo pStages[] = {
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
    FBR_ACK(createOpaqueTrianglePipe(pVulkan,
                                     layout,
                                     VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                     2,
                                     pStages,
                                     NULL,
                                     pPipe))
    vkDestroyShaderModule(pVulkan->device, vertShaderModule, NULL);
    vkDestroyShaderModule(pVulkan->device, fragShaderModule, NULL);

    return VK_SUCCESS;
}

VkResult fbrCreatePipeNode(const FbrVulkan *pVulkan,
                           FbrPipeLayoutNode layout,
                           const char *pVertShaderPath,
                           const char *pFragShaderPath,
                           const char *pTessCShaderPath,
                           const char *pTessEShaderPath,
                           FbrPipeNode *pPipe)
{
    VkShaderModule vertShaderModule;
    FBR_ACK(createShaderModule(pVulkan,
                                pVertShaderPath,
                                &vertShaderModule))
    VkShaderModule fragShaderModule;
    FBR_ACK(createShaderModule(pVulkan,
                                pFragShaderPath,
                                &fragShaderModule))
    VkShaderModule tessCShaderModule;
    FBR_ACK(createShaderModule(pVulkan,
                                pTessCShaderPath,
                                &tessCShaderModule))
    VkShaderModule tessEShaderModule;
    FBR_ACK(createShaderModule(pVulkan,
                                pTessEShaderPath,
                                &tessEShaderModule))
    const VkPipelineShaderStageCreateInfo pStages[] = {
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
            },
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
                    .module = tessCShaderModule,
                    .pName = "main",
            },
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
                    .module = tessEShaderModule,
                    .pName = "main",
            }
    };
    const VkPipelineTessellationStateCreateInfo pipelineTessellationStateCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .patchControlPoints = 4,
    };
    FBR_ACK(createOpaqueTrianglePipe(pVulkan,
                                     layout,
                                     VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
                                     4,
                                     pStages,
                                     &pipelineTessellationStateCreateInfo,
                                     pPipe))
    vkDestroyShaderModule(pVulkan->device, vertShaderModule, NULL);
    vkDestroyShaderModule(pVulkan->device, fragShaderModule, NULL);
    vkDestroyShaderModule(pVulkan->device, tessCShaderModule, NULL);
    vkDestroyShaderModule(pVulkan->device, tessEShaderModule, NULL);
    FBR_SUCCESS
}

static VkResult createPipes(const FbrVulkan *pVulkan,
                            FbrPipelines *pPipes)
{
    FBR_ACK(fbrCreatePipeStandard(pVulkan,
                                  pPipes->pipeLayoutStandard,
                                  "./shaders/vert.spv",
                                  "./shaders/frag.spv",
                                  &pPipes->pipeStandard))

    FBR_ACK(fbrCreatePipeNode(pVulkan,
                                   pPipes->pipeLayoutNode,
                                   "./shaders/node_vert.spv",
                                   "./shaders/node_frag.spv",
                                   "./shaders/node_tesc.spv",
                                   "./shaders/node_tese.spv",
                                   &pPipes->pipeNode));

    FBR_SUCCESS
}

VkResult fbrCreatePipelines(const FbrVulkan *pVulkan,
                            const FbrDescriptors *pDescriptors,
                            FbrPipelines **ppAllocPipes)
{
    *ppAllocPipes = calloc(1, sizeof(FbrPipelines));
    FbrPipelines *pPipes = *ppAllocPipes;

    FBR_ACK(createPipelineLayouts(pVulkan, pDescriptors, pPipes))
    FBR_ACK(createPipes(pVulkan, pPipes))

    return VK_SUCCESS;
}

void fbrDestroyPipelines(const FbrVulkan *pVulkan,
                         FbrPipelines *pPipelines)
{
    vkDestroyPipelineLayout(pVulkan->device, pPipelines->pipeLayoutStandard, NULL);
    vkDestroyPipeline(pVulkan->device, pPipelines->pipeStandard, NULL);

    free(pPipelines);
}