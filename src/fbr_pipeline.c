#include "fbr_pipeline.h"
#include "fbr_mesh.h"
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

// Describes the layout of inputs for shaders
static VkResult initDescriptorSetLayout(const FbrVulkan *pVulkan, FbrPipeline *pPipeline) {
    VkDescriptorSetLayoutBinding bindings[] = {
            {
                    .binding = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pImmutableSamplers = NULL,
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
            },
            {
                    .binding = 1,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .pImmutableSamplers = NULL,
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
            }
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 2,
            .pBindings = bindings,
    };

    FBR_VK_CHECK_RETURN(vkCreateDescriptorSetLayout(pVulkan->device, &layoutInfo, NULL, &pPipeline->descriptorSetLayout));
}

// what actually gets fed to shaders
void fbrInitDescriptorSet(const FbrVulkan *pVulkan,
                          const FbrCamera *pCameraState,
                          VkDescriptorSetLayout descriptorSetLayout,
                          VkImageView imageView,
                          VkDescriptorSet *pDescriptorSet) {

    VkDescriptorSetAllocateInfo allocInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = pVulkan->descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &descriptorSetLayout,
    };
    FBR_VK_CHECK(vkAllocateDescriptorSets(pVulkan->device, &allocInfo, pDescriptorSet));

    VkDescriptorBufferInfo bufferInfo = {
            .buffer = pCameraState->pUBO->uniformBuffer,
            .offset = 0,
            .range = sizeof(FbrCameraUBO),
    };
    VkDescriptorImageInfo imageInfo = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView = imageView,
            .sampler = pVulkan->sampler,
    };
    VkWriteDescriptorSet descriptorWrites[] = {
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = *pDescriptorSet,
                    .dstBinding = 0,
                    .dstArrayElement = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .descriptorCount = 1,
                    .pBufferInfo = &bufferInfo,
            },
            {
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = *pDescriptorSet,
                    .dstBinding = 1,
                    .dstArrayElement = 0,
                    .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .descriptorCount = 1,
                    .pImageInfo = &imageInfo,
            },
    };
    vkUpdateDescriptorSets(pVulkan->device, 2, descriptorWrites, 0, NULL);
}

// full description of a pipeline, but only shaders get hard-bound to it, different descriptor sets can be attached
static void initPipelineLayout(const FbrVulkan *pVulkan, FbrPipeline *pPipeline) {
    VkPushConstantRange pushConstant = {
            .offset = 0,
            .size = sizeof(mat4),
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &pPipeline->descriptorSetLayout,
            .pPushConstantRanges = &pushConstant,
            .pushConstantRangeCount  = 1
    };

    FBR_VK_CHECK(vkCreatePipelineLayout(pVulkan->device, &pipelineLayoutInfo, NULL, &pPipeline->pipelineLayout));
}

// create actual pipeline to do rendering
static void initPipeline(const FbrVulkan *pVulkan,
                         VkRenderPass renderPass,
                         const char* pVertShaderPath,
                         const char* pFragShaderPath,
                         FbrPipeline *pPipeline) {
    uint32_t vertLength;
    char *vertShaderCode = readBinaryFile(pVertShaderPath, &vertLength);
    uint32_t fragLength;
    char *fragShaderCode = readBinaryFile(pFragShaderPath, &fragLength);

    const VkShaderModule vertShaderModule = createShaderModule(pVulkan, vertShaderCode, vertLength);
    const VkShaderModule fragShaderModule = createShaderModule(pVulkan, fragShaderCode, fragLength);

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
    const VkPipelineViewportStateCreateInfo viewportState = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount = 1,
    };
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
    const VkPipelineColorBlendAttachmentState colorBlendAttachment = {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable = VK_FALSE,
    };
    const VkPipelineColorBlendStateCreateInfo colorBlending = {
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
            .pNext = VK_NULL_HANDLE,
            .storageBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2_EXT,
            .uniformBuffers = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2_EXT,
            .vertexInputs = VK_PIPELINE_ROBUSTNESS_BUFFER_BEHAVIOR_ROBUST_BUFFER_ACCESS_2_EXT,
            .images = VK_PIPELINE_ROBUSTNESS_IMAGE_BEHAVIOR_ROBUST_IMAGE_ACCESS_2_EXT,
    };
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
            .layout = pPipeline->pipelineLayout,
            .renderPass = renderPass,
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
                       VkRenderPass renderPass,
                       const char* pVertShaderPath,
                       const char* pFragShaderPath,
                       FbrPipeline **ppAllocPipeline) {
    *ppAllocPipeline = calloc(1, sizeof(FbrPipeline));
    FbrPipeline *pPipeline = *ppAllocPipeline;

    initDescriptorSetLayout(pVulkan, pPipeline);

    initPipelineLayout(pVulkan, pPipeline);

    initPipeline(pVulkan, renderPass, pVertShaderPath, pFragShaderPath, pPipeline);
}

void fbrCleanupPipeline(const FbrVulkan *pVulkan, FbrPipeline *pPipeline) {
    vkDestroyDescriptorSetLayout(pVulkan->device, pPipeline->descriptorSetLayout, NULL);
    vkDestroyPipeline(pVulkan->device, pPipeline->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(pVulkan->device, pPipeline->pipelineLayout, NULL);
    free(pPipeline);
}