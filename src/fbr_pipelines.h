#ifndef FABRIC_STANDARD_PIPELINES_H
#define FABRIC_STANDARD_PIPELINES_H

#include "fbr_app.h"

// Push.VertStage.Matrix _ Uniform.VertStage.UBO _ Uniform.FragStage.Sampler
typedef VkPipelineLayout FbrPipeLayout_pvMat4_uvUBO_ufSampler;
typedef VkPipeline FbrPipe_pvMat4_uvUBO_ufSampler;
// PushConstant.VertStage.Matrix _ Uniform.VertStage.UBO _ Uniform.FragStage.Sampler _ In.VertStage.FbrVertex
typedef FbrPipe_pvMat4_uvUBO_ufSampler FbrPipe_pvMat4_uvUBO_ufSampler_ivVertex;


typedef struct FbrPipelines {
    FbrPipeLayout_pvMat4_uvUBO_ufSampler pipeLayout_pvMat4_uvUBO_ufSampler;

    FbrPipe_pvMat4_uvUBO_ufSampler_ivVertex pipe_pvMat4_uvCamera_ufTexture_ivVertex;

} FbrPipelines;

VkResult fbrCreatePipe_pvMat4_uvUBO_ufSampler_ivVertex(const FbrVulkan *pVulkan,
                                                       FbrPipeLayout_pvMat4_uvUBO_ufSampler pipeLayout_pvMat4_uvUBO_ufSampler,
                                                       const char *pVertShaderPath,
                                                       const char *pFragShaderPath,
                                                       FbrPipe_pvMat4_uvUBO_ufSampler_ivVertex *pPipeline);

VkResult fbrCreatePipelines(const FbrVulkan *pVulkan,
                                const FbrDescriptors *pDescriptors,
                                FbrPipelines **ppAllocPipes);

VkResult fbrDestroyPipelines(const FbrVulkan *pVulkan,
                                 FbrPipelines *pPipelines);

#endif //FABRIC_STANDARD_PIPELINES_H