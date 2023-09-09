#include "fbr_pipelines.h"
#include "fbr_descriptors.h"
#include "fbr_vulkan.h"
#include "fbr_log.h"
#include "fbr_mesh.h"

static FBR_RESULT createPipeLayoutStandard(const FbrVulkan *pVulkan,
                                         const FbrDescriptors *pDescriptors,
                                         FbrPipeLayoutStandard *pipelineLayout)
{
    const VkDescriptorSetLayout pSetLayouts[] = {
            pDescriptors->setLayoutGlobal.layout,
            pDescriptors->setLayoutPass.layout,
            pDescriptors->setLayoutMaterial.layout,
            pDescriptors->setLayoutObject.layout
    };
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .setLayoutCount = COUNT(pSetLayouts),
            .pSetLayouts = pSetLayouts,
            .pPushConstantRanges = NULL,
            .pushConstantRangeCount  = 0
    };
    FBR_ACK(vkCreatePipelineLayout(pVulkan->device,
                                   &pipelineLayoutInfo,
                                   FBR_ALLOCATOR,
                                   pipelineLayout));

    return FBR_SUCCESS;
}

static FBR_RESULT createPipeLayoutNodeTess(const FbrVulkan *pVulkan,
                                           const FbrDescriptors *pDescriptors,
                                           FbrPipeLayoutNodeTess *pPipeLayout)
{
    const VkDescriptorSetLayout pSetLayouts[] = {
            pDescriptors->setLayoutGlobal.layout,
            pDescriptors->setLayoutPass.layout,
            pDescriptors->setLayoutMaterial.layout,
            pDescriptors->setLayoutNode.layout
    };
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .setLayoutCount = COUNT(pSetLayouts),
            .pSetLayouts = pSetLayouts,
            .pPushConstantRanges = NULL,
            .pushConstantRangeCount  = 0
    };
    FBR_ACK(vkCreatePipelineLayout(pVulkan->device,
                                   &pipelineLayoutInfo,
                                   FBR_ALLOCATOR,
                                   pPipeLayout));

    return FBR_SUCCESS;
}

static FBR_RESULT createPipeLayoutNodeMesh(const FbrVulkan *pVulkan,
                                           const FbrDescriptors *pDescriptors,
                                           FbrPipeLayoutNodeTess *pPipeLayout)
{
    const VkDescriptorSetLayout pSetLayouts[] = {
            pDescriptors->setLayoutGlobal.layout,
            pDescriptors->setLayoutPass.layout,
            pDescriptors->setLayoutMaterial.layout,
            pDescriptors->setLayoutNode.layout
    };
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .setLayoutCount = COUNT(pSetLayouts),
            .pSetLayouts = pSetLayouts,
            .pPushConstantRanges = NULL,
            .pushConstantRangeCount  = 0
    };
    FBR_ACK(vkCreatePipelineLayout(pVulkan->device,
                                   &pipelineLayoutInfo,
                                   FBR_ALLOCATOR,
                                   pPipeLayout));

    return FBR_SUCCESS;
}

static FBR_RESULT createPipeLayoutComposite(const FbrVulkan *pVulkan,
                                            const FbrDescriptors *pDescriptors,
                                            FbrComputePipeLayoutComposite *pPipeLayout)
{
    const VkDescriptorSetLayout pSetLayouts[] = {
            pDescriptors->setLayoutGlobal.layout,
            pDescriptors->setLayoutComposite.layout,
    };
    const VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = NULL,
            .setLayoutCount = COUNT(pSetLayouts),
            .pSetLayouts = pSetLayouts,
            .pPushConstantRanges = NULL,
            .pushConstantRangeCount  = 0
    };
    FBR_ACK(vkCreatePipelineLayout(pVulkan->device,
                                   &pipelineLayoutInfo,
                                   FBR_ALLOCATOR,
                                   pPipeLayout));

    return FBR_SUCCESS;
}

static FBR_RESULT createPipelineLayouts(const FbrVulkan *pVulkan,
                                      const FbrDescriptors *pDescriptors,
                                      FbrPipelines *pPipelines)
{
    FBR_ACK(createPipeLayoutStandard(pVulkan,
                                      pDescriptors,
                                      &pPipelines->graphicsPipeLayoutStandard));
    FBR_ACK(createPipeLayoutNodeTess(pVulkan,
                                     pDescriptors,
                                     &pPipelines->graphicsPipeLayoutNodeTess));
    FBR_ACK(createPipeLayoutNodeMesh(pVulkan,
                                     pDescriptors,
                                     &pPipelines->graphicsPipeLayoutNodeMesh));
    FBR_ACK(createPipeLayoutComposite(pVulkan,
                                      pDescriptors,
                                      &pPipelines->computePipeLayoutComposite));

    return FBR_SUCCESS;
}


static FBR_RESULT allocReadFile(const char *filename,
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

    return FBR_SUCCESS;
}

static FBR_RESULT createShaderModule(const FbrVulkan *pVulkan,
                                     const char *pShaderPath,
                                     VkShaderModule *pShaderModule)
{
    uint32_t codeLength;
    char *pShaderCode;
    FBR_ACK(allocReadFile(pShaderPath,
                           &codeLength,
                           &pShaderCode));

    VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = codeLength,
            .pCode = (const uint32_t *) pShaderCode,
    };
    FBR_ACK(vkCreateShaderModule(pVulkan->device,
                                  &createInfo,
                                  FBR_ALLOCATOR,
                                  pShaderModule));

    free(pShaderCode);

    return FBR_SUCCESS;
}

static FBR_RESULT createOpaquePipe(const FbrVulkan *pVulkan,
                                   VkPipelineLayout layout,
                                   VkPrimitiveTopology topology,
                                   uint32_t stageCount,
                                   const VkPipelineShaderStageCreateInfo *pStages,
                                   const VkPipelineVertexInputStateCreateInfo *pVertexInputState,
                                   const VkPipelineInputAssemblyStateCreateInfo *pInputAssemblyState,
                                   const VkPipelineTessellationStateCreateInfo *pTessellationState,
                                   VkPipeline *pPipe)
{
    // Fragment
    const VkPipelineColorBlendAttachmentState pPipelineColorBlendAttachmentStates[] = {
            {
                    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                      VK_COLOR_COMPONENT_G_BIT |
                                      VK_COLOR_COMPONENT_B_BIT |
                                      VK_COLOR_COMPONENT_A_BIT,
                    .blendEnable = VK_FALSE,
            },
            {
                    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                      VK_COLOR_COMPONENT_G_BIT |
                                      VK_COLOR_COMPONENT_B_BIT |
                                      VK_COLOR_COMPONENT_A_BIT,
                    .blendEnable = VK_FALSE,
            },
            {
                    .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                      VK_COLOR_COMPONENT_G_BIT |
                                      VK_COLOR_COMPONENT_B_BIT |
                                      VK_COLOR_COMPONENT_A_BIT,
                    .blendEnable = VK_FALSE,
            }
    };
    const VkPipelineColorBlendStateCreateInfo colorBlendState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext = NULL,
            .logicOpEnable = VK_FALSE,
            .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = COUNT(pPipelineColorBlendAttachmentStates),
            .pAttachments = pPipelineColorBlendAttachmentStates,
            .blendConstants[0] = 0.0f,
            .blendConstants[1] = 0.0f,
            .blendConstants[2] = 0.0f,
            .blendConstants[3] = 0.0f,
    };
    const VkPipelineDepthStencilStateCreateInfo depthStencilState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable = VK_TRUE,
            .depthWriteEnable =  VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL, // vktut has VK_COMPARE_OP_LESS ?
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable = VK_FALSE,
//            .minDepthBounds = 0.0f,
//            .maxDepthBounds = 1.0f,
    };

    // Rasterizing
    const VkPipelineRasterizationStateCreateInfo rasterizationState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .depthClampEnable = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
#ifdef FBR_DEBUG_WIREFRAME
            .polygonMode = pVulkan->isChild ? VK_POLYGON_MODE_FILL : VK_POLYGON_MODE_LINE,
#else
            .polygonMode = VK_POLYGON_MODE_FILL,
#endif
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable = VK_FALSE,
            .depthBiasConstantFactor = 0,
            .depthBiasClamp = 0,
            .depthBiasSlopeFactor = 0,
            .lineWidth = 1.0f,
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
            .storageBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT_EXT,
//            .storageBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2_EXT,
            .uniformBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT_EXT,
//            .uniformBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2_EXT,
            .vertexInputs = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_DEVICE_DEFAULT_EXT,
//            .vertexInputs = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2_EXT,
            .images = VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_DEVICE_DEFAULT_EXT,
//            .images = VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS_2_EXT,
    };

    // Create
    const VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .pNext = &pipelineRobustnessCreateInfo,
            .flags = 0,
            .stageCount = stageCount,
            .pStages = pStages,
            .pVertexInputState = pVertexInputState,
            .pInputAssemblyState = pInputAssemblyState,
            .pTessellationState = pTessellationState,
            .pViewportState = &viewportState,
            .pRasterizationState = &rasterizationState,
            .pMultisampleState = &multisampleState,
            .pDepthStencilState = &depthStencilState,
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
                                      FBR_ALLOCATOR,
                                      pPipe));

    return FBR_SUCCESS;
}

static FBR_RESULT createVertexInputOpaquePipe(const FbrVulkan *pVulkan,
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
            },
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
            .pVertexBindingDescriptions = pVertexBindingDescriptions,
            .vertexAttributeDescriptionCount = 3,
            .pVertexAttributeDescriptions = pVertexAttributeDescriptions
    };
    const VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology = topology,
            .primitiveRestartEnable = VK_FALSE,
    };
    FBR_ACK(createOpaquePipe(pVulkan,
                             layout,
                             topology,
                             stageCount,
                             pStages,
                             &vertexInputState,
                             &inputAssemblyState,
                             pTessellationState,
                             pPipe));

    return FBR_SUCCESS;
}

FBR_RESULT createPipeStandard(const FbrVulkan *pVulkan,
                              const FbrPipelines *pPipes,
                              const char *pVertShaderPath,
                              const char *pFragShaderPath,
                              FbrPipeStandard *pPipe)
{
    VkShaderModule vertShaderModule;
    FBR_ACK(createShaderModule(pVulkan,
                               pVertShaderPath,
                               &vertShaderModule));
    VkShaderModule fragShaderModule;
    FBR_ACK(createShaderModule(pVulkan,
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
    FBR_ACK(createVertexInputOpaquePipe(pVulkan,
                                        pPipes->graphicsPipeLayoutStandard,
                                        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                                        COUNT(pStages),
                                        pStages,
                                        NULL,
                                        pPipe));
    vkDestroyShaderModule(pVulkan->device, vertShaderModule, FBR_ALLOCATOR);
    vkDestroyShaderModule(pVulkan->device, fragShaderModule, FBR_ALLOCATOR);

    return FBR_SUCCESS;
}

FBR_RESULT createGraphicsPipeNodeTess(const FbrVulkan *pVulkan,
                                      const FbrPipelines *pPipes,
                                      FbrPipeNodeTess *pPipe)
{
    VkShaderModule vertShaderModule;
    FBR_ACK(createShaderModule(pVulkan,
                               "./shaders/node_tess_vert.spv",
                                &vertShaderModule));
    VkShaderModule fragShaderModule;
    FBR_ACK(createShaderModule(pVulkan,
                               "./shaders/node_tess_frag.spv",
                                &fragShaderModule));
    VkShaderModule tessCShaderModule;
    FBR_ACK(createShaderModule(pVulkan,
                               "./shaders/node_tess_tesc.spv",
                                &tessCShaderModule));
    VkShaderModule tessEShaderModule;
    FBR_ACK(createShaderModule(pVulkan,
                               "./shaders/node_tess_tese.spv",
                                &tessEShaderModule));
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
    FBR_ACK(createVertexInputOpaquePipe(pVulkan,
                                        pPipes->graphicsPipeLayoutNodeTess,
                                        VK_PRIMITIVE_TOPOLOGY_PATCH_LIST,
                                        COUNT(pStages),
                                        pStages,
                                        &pipelineTessellationStateCreateInfo,
                                        pPipe));
    vkDestroyShaderModule(pVulkan->device, vertShaderModule, FBR_ALLOCATOR);
    vkDestroyShaderModule(pVulkan->device, fragShaderModule, FBR_ALLOCATOR);
    vkDestroyShaderModule(pVulkan->device, tessCShaderModule, FBR_ALLOCATOR);
    vkDestroyShaderModule(pVulkan->device, tessEShaderModule, FBR_ALLOCATOR);

    return FBR_SUCCESS;
}

FBR_RESULT createGraphicsPipeNodeMesh(const FbrVulkan *pVulkan,
                                       const FbrPipelines *pPipes,
                                       FbrPipeNodeTess *pPipe)
{
    VkShaderModule taskShaderModule;
    FBR_ACK(createShaderModule(pVulkan,
                               "./shaders/node_mesh_task.spv",
                               &taskShaderModule));
    VkShaderModule meshShaderModule;
    FBR_ACK(createShaderModule(pVulkan,
                               "./shaders/node_mesh_mesh.spv",
                               &meshShaderModule));
    VkShaderModule fragShaderModule;
    FBR_ACK(createShaderModule(pVulkan,
                               "./shaders/node_mesh_frag.spv",
                               &fragShaderModule));
    const VkPipelineShaderStageCreateInfo pStages[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_TASK_BIT_EXT,
                    .module = taskShaderModule,
                    .pName = "main",
            },
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_MESH_BIT_EXT,
                    .module = meshShaderModule,
                    .pName = "main",
            },
            {
                    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .module = fragShaderModule,
                    .pName = "main",
            },
    };
    FBR_ACK(createOpaquePipe(pVulkan,
                             pPipes->graphicsPipeLayoutNodeMesh,
                             VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                             COUNT(pStages),
                             pStages,
                             NULL,
                             NULL,
                             NULL,
                             pPipe));
    vkDestroyShaderModule(pVulkan->device, taskShaderModule, FBR_ALLOCATOR);
    vkDestroyShaderModule(pVulkan->device, meshShaderModule, FBR_ALLOCATOR);
    vkDestroyShaderModule(pVulkan->device, fragShaderModule, FBR_ALLOCATOR);

    return FBR_SUCCESS;
}

FBR_RESULT createComputePipeComposite(const FbrVulkan *pVulkan,
                                      const FbrPipelines *pPipes,
                                      const char *pCompositeShaderPath,
                                      FbrComputePipeComposite *pPipe)
{
    VkShaderModule compositeShaderModule;
    FBR_ACK(createShaderModule(pVulkan,
                               pCompositeShaderPath,
                               &compositeShaderModule));
    const VkPipelineShaderStageCreateInfo stage = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = compositeShaderModule,
            .pName = "main",
    };
    const VkComputePipelineCreateInfo pipelineInfo = {
            .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
            .layout = pPipes->computePipeLayoutComposite,
            .stage = stage,
    };
    FBR_ACK(vkCreateComputePipelines(pVulkan->device,
                                     VK_NULL_HANDLE,
                                     1,
                                     &pipelineInfo,
                                     FBR_ALLOCATOR,
                                     pPipe));

    return FBR_SUCCESS;
}

static FBR_RESULT createPipes(const FbrVulkan *pVulkan,
                              FbrPipelines *pPipes)
{
    FBR_ACK(createPipeStandard(pVulkan,
                               pPipes,
                               "./shaders/vert.spv",
//                                  pVulkan->isChild ?
//                                  "./shaders/frag_crasher.spv":
                               "./shaders/frag.spv",
                               &pPipes->graphicsPipeStandard));
    FBR_ACK(createGraphicsPipeNodeTess(pVulkan,
                                       pPipes,
                                       &pPipes->graphicsPipeNodeTess));
    FBR_ACK(createGraphicsPipeNodeMesh(pVulkan,
                                       pPipes,
                                       &pPipes->graphicsPipeNodeMesh));
    FBR_ACK(createComputePipeComposite(pVulkan,
                                       pPipes,
                                       "./shaders/composite_depthoffset_comp.spv",
                                       &pPipes->computePipeComposite));

    return FBR_SUCCESS;
}

FBR_RESULT fbrCreatePipelines(const FbrVulkan *pVulkan,
                            const FbrDescriptors *pDescriptors,
                            FbrPipelines **ppAllocPipes)
{
    *ppAllocPipes = calloc(1, sizeof(FbrPipelines));
    FbrPipelines *pPipes = *ppAllocPipes;

    FBR_ACK(createPipelineLayouts(pVulkan, pDescriptors, pPipes));
    FBR_ACK(createPipes(pVulkan, pPipes));

    return FBR_SUCCESS;
}

void fbrDestroyPipelines(const FbrVulkan *pVulkan,
                         FbrPipelines *pPipelines)
{
    vkDestroyPipelineLayout(pVulkan->device, pPipelines->graphicsPipeLayoutStandard, FBR_ALLOCATOR);
    vkDestroyPipelineLayout(pVulkan->device, pPipelines->graphicsPipeLayoutNodeTess, FBR_ALLOCATOR);
    vkDestroyPipelineLayout(pVulkan->device, pPipelines->computePipeLayoutComposite, FBR_ALLOCATOR);

    vkDestroyPipeline(pVulkan->device, pPipelines->graphicsPipeStandard, FBR_ALLOCATOR);
    vkDestroyPipeline(pVulkan->device, pPipelines->graphicsPipeNodeTess, FBR_ALLOCATOR);
    vkDestroyPipeline(pVulkan->device, pPipelines->computePipeComposite, FBR_ALLOCATOR);

    free(pPipelines);
}