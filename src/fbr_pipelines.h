#ifndef FABRIC_STANDARD_PIPELINES_H
#define FABRIC_STANDARD_PIPELINES_H

#include "fbr_app.h"
#include "fbr_vulkan.h"

typedef VkPipelineLayout FbrPipeLayoutStandard;
typedef VkPipeline FbrPipeStandard;

typedef VkPipelineLayout FbrPipeLayoutNodeTess;
typedef VkPipeline FbrPipeNodeTess;

typedef VkPipelineLayout FbrPipeLayoutNodeMesh;
typedef VkPipeline FbrPipeNodeMesh;

typedef VkPipelineLayout FbrComputePipeLayoutComposite;
typedef VkPipeline FbrComputePipeComposite;

typedef struct FbrPipelines {
    VkPipelineLayout graphicsPipeLayoutStandard;
    FbrPipeStandard graphicsPipeStandard;

    VkPipelineLayout graphicsPipeLayoutNodeTess;
    FbrPipeNodeTess graphicsPipeNodeTess;

    VkPipelineLayout graphicsPipeLayoutNodeMesh;
    FbrPipeNodeMesh graphicsPipeNodeMesh;

    VkPipelineLayout computePipeLayoutComposite;
    FbrComputePipeComposite computePipeComposite;
} FbrPipelines;

FBR_RESULT fbrCreatePipelines(const FbrVulkan *pVulkan,
                              const FbrDescriptors *pDescriptors,
                              FbrPipelines **ppAllocPipes);

void fbrDestroyPipelines(const FbrVulkan *pVulkan,
                                 FbrPipelines *pPipelines);

#endif //FABRIC_STANDARD_PIPELINES_H
