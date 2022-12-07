#include "fbr_pipeline.h"
#include "fbr_mesh.h"

#include <memory.h>

#define FBR_ATTRIBUTE_DESCRIPTION_COUNT 2

static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {
            .binding = 0,
            .stride = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
    };

    return bindingDescription;
}

static void getAttributeDescriptions(VkVertexInputAttributeDescription attributeDescriptions[FBR_ATTRIBUTE_DESCRIPTION_COUNT]) {
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, color);
}

static char* readBinaryFile(const char* filename, uint32_t *length) {
    FILE* file = fopen(filename, "rb");

    if (file == NULL) {
        printf("%s - file can't be opened! %s\n", __FUNCTION__, filename);
    }

    fseek(file, 0, SEEK_END);
    *length = ftell(file);
    rewind(file);
    char* contents = malloc(1 + *length * sizeof (char));
    size_t readCount = fread( contents, *length, 1, file);
    if (readCount == 0) {
        printf("%s - failed to read file! %s\n", __FUNCTION__, filename);
    }
    fclose(file);

    return contents;
}

static VkShaderModule createShaderModule(const FbrAppState *pAppState, const char* code, const uint32_t codeLength) {
    VkShaderModuleCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = codeLength,
            .pCode = (const uint32_t *) code,
    };

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(pAppState->device, &createInfo, NULL, &shaderModule) != VK_SUCCESS) {
        printf("%s - failed to create shader module! %s\n", __FUNCTION__, code);
    }

    return shaderModule;
}

static void createDescriptorSetLayout(const FbrAppState *pAppState, FbrPipeline* pPipeline) {
    VkDescriptorSetLayoutBinding uboLayoutBinding = {
            .binding = 0,
            .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = 1,
            .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
    };

    VkDescriptorSetLayoutCreateInfo layoutInfo = {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .bindingCount = 1,
            .pBindings = &uboLayoutBinding,
    };

    if (vkCreateDescriptorSetLayout(pAppState->device, &layoutInfo, NULL, &pPipeline->descriptorSetLayout) != VK_SUCCESS) {
        printf("%s - failed to create descriptor set layout!\n", __FUNCTION__);
    }
}

void fbrCreatePipeline(const FbrAppState *pAppState, FbrPipeline** ppAllocPipeline) {
    *ppAllocPipeline = malloc(sizeof(FbrPipeline));
    FbrPipeline* pPipeline = *ppAllocPipeline;
    memset(pPipeline, 0, sizeof(FbrPipeline));

    createDescriptorSetLayout(pAppState, pPipeline);

    uint32_t vertLength;
    const char* vertShaderCode = readBinaryFile("./shaders/vert.spv", &vertLength);
    uint32_t fragLength;
    const char* fragShaderCode = readBinaryFile("./shaders/frag.spv", &fragLength);

    VkShaderModule vertShaderModule = createShaderModule(pAppState, vertShaderCode, vertLength);
    VkShaderModule fragShaderModule = createShaderModule(pAppState, fragShaderCode, fragLength);

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

    VkVertexInputBindingDescription bindingDescription = getBindingDescription();
    VkVertexInputAttributeDescription attributeDescriptions[FBR_ATTRIBUTE_DESCRIPTION_COUNT];
    getAttributeDescriptions(attributeDescriptions);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount = 1,
            .vertexAttributeDescriptionCount = FBR_ATTRIBUTE_DESCRIPTION_COUNT,
            .pVertexBindingDescriptions = &bindingDescription,
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

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount = 1,
            .pSetLayouts = &pPipeline->descriptorSetLayout,
    };

    if (vkCreatePipelineLayout(pAppState->device, &pipelineLayoutInfo, NULL, &pPipeline->pipelineLayout) != VK_SUCCESS) {
        printf("%s - failed to create pipeline layout!\n", __FUNCTION__);
    }

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
            .renderPass = pAppState->renderPass,
            .subpass = 0,
            .basePipelineHandle = VK_NULL_HANDLE,
    };

    if (vkCreateGraphicsPipelines(pAppState->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, &pPipeline->graphicsPipeline) != VK_SUCCESS) {
        printf("%s - failed to create graphics pipeline!\n", __FUNCTION__);
    }

    vkDestroyShaderModule(pAppState->device, fragShaderModule, NULL);
    vkDestroyShaderModule(pAppState->device, vertShaderModule, NULL);
}

void fbrFreePipeline(const FbrAppState *pAppState, FbrPipeline* pPipeline) {
    vkDestroyDescriptorSetLayout(pAppState->device, pAppState->pPipeline->descriptorSetLayout, NULL);
    vkDestroyPipeline(pAppState->device, pAppState->pPipeline->graphicsPipeline, NULL);
    vkDestroyPipelineLayout(pAppState->device, pAppState->pPipeline->pipelineLayout, NULL);
    free(pPipeline);
}