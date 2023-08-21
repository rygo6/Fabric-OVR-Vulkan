#include "fbr_mesh.h"
#include "fbr_app.h"
#include "fbr_buffer.h"
#include "fbr_vulkan.h"

const Vertex quadVertices[4] = {
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
        {{0.5f,  -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
        {{0.5f,  0.5f, 0.0f},  {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.0f},  {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

const uint16_t quadTriangles[6] = {
        0, 1, 2,
        2, 3, 0
};

#define PI 3.14159265358979323846f

int generateSphereVertexCount(int nslices, int nstacks)
{
    return (nslices + 1) * (nstacks + 1);
}

void generateSphere(int nslices, int nstacks, float radius, Vertex* pVertex) {
//    Vertex* data = (Vertex*)malloc((nslices+1) * (nstacks+1) * sizeof(Vertex));

    float fnslices = (float)nslices;
    float fnstacks = (float)nstacks;

    float dtheta = 2.0f * PI / fnslices;
    float dphi = PI / fnstacks;

    int idx = 0;
    for (int i = 0;+ i <= nstacks; i++) {
        float fi = (float)i;
        float phi = fi * dphi;
        for (int j = 0; j <= nslices; j++) {
            float ji = (float)j;
            float theta = ji * dtheta;

            float x = radius * sinf(phi) * cosf(theta);
            float y = radius * sinf(phi) * sinf(theta);
            float z = radius * cosf(phi);

            vec3 pos = {x, y, z};
            vec3 normal = {x, y, z};
            vec2 uv = {ji / fnslices, fi / fnstacks};

            Vertex vertexData = {};
            glm_vec3_copy(pos, vertexData.pos);
            glm_vec3_copy(normal, vertexData.normal);
            glm_vec2_copy(uv, vertexData.uv);

            pVertex[idx++] = vertexData;
        }
    }
}

int generateSphereIndexCount(int nslices, int nstacks)
{
    return nslices * nstacks * 2 * 3;
}

void generateSphereIndices(int nslices, int nstacks, uint16_t* pIndices) {
//    uint16_t* indices = (uint16_t*)malloc(nslices * nstacks * 2 * sizeof(uint16_t) * 3);

    int idx = 0;
    for (int i = 0; i < nstacks; i++) {
        for (int j = 0; j < nslices; j++) {
            uint16_t v1 = i * (nslices+1) + j;
            uint16_t v2 = i * (nslices+1) + j + 1;
            uint16_t v3 = (i+1) * (nslices+1) + j;
            uint16_t v4 = (i+1) * (nslices+1) + j + 1;

            pIndices[idx++] = v1;
            pIndices[idx++] = v2;
            pIndices[idx++] = v3;

            pIndices[idx++] = v2;
            pIndices[idx++] = v4;
            pIndices[idx++] = v3;
        }
    }
}

static void createVertexBuffer(const FbrVulkan *pVulkan, const Vertex *pVertices, int vertexCount, FbrMesh *pMeshState) {
    pMeshState->vertexCount = vertexCount;
    VkDeviceSize bufferSize = (sizeof(Vertex) * vertexCount);
    fbrCreatePopulateBufferViaStaging(pVulkan,
                                      pVertices,
                                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                      &pMeshState->vertexBuffer,
                                      &pMeshState->vertexBufferMemory,
                                      bufferSize);
}

static void createIndexBuffer(const FbrVulkan *pVulkan, const uint16_t *pIndices, int indexCount, FbrMesh *pMeshState) {
    pMeshState->indexCount = indexCount;
    VkDeviceSize bufferSize = (sizeof(uint16_t) * indexCount);
    fbrCreatePopulateBufferViaStaging(pVulkan,
                                      pIndices,
                                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                      &pMeshState->indexBuffer,
                                      &pMeshState->indexBufferMemory,
                                      bufferSize);
}

void fbrCreateMesh(const FbrVulkan *pVulkan, FbrMesh **ppAllocMeshState)
{
    *ppAllocMeshState = calloc(1, sizeof(FbrMesh));
    FbrMesh *pMeshState = *ppAllocMeshState;
    createVertexBuffer(pVulkan, quadVertices, 4, pMeshState);
    createIndexBuffer(pVulkan, quadTriangles, 6, pMeshState);
}

void fbrCreateSphereMesh(const FbrVulkan *pVulkan, FbrMesh **ppAllocMeshState) {
    *ppAllocMeshState = calloc(1, sizeof(FbrMesh));
    FbrMesh *pMeshState = *ppAllocMeshState;

    const int nSlices = 32;
    const int nStack = 32;
    int vertexCount = generateSphereVertexCount(nSlices, nStack);
    Vertex pVertices[vertexCount];
    generateSphere(nSlices, nStack, 0.5f, pVertices);
    int indexCount = generateSphereIndexCount(nSlices, nStack);
    uint16_t pIndices[indexCount];
    generateSphereIndices(nSlices, nStack, pIndices);

    createVertexBuffer(pVulkan, pVertices, vertexCount, pMeshState);
    createIndexBuffer(pVulkan, pIndices, indexCount, pMeshState);
}

void fbrCleanupMesh(const FbrVulkan *pVulkan, FbrMesh *pMeshState)
{
    vkDestroyBuffer(pVulkan->device, pMeshState->indexBuffer, NULL);
    vkFreeMemory(pVulkan->device, pMeshState->indexBufferMemory, NULL);
    vkDestroyBuffer(pVulkan->device, pMeshState->vertexBuffer, NULL);
    vkFreeMemory(pVulkan->device, pMeshState->vertexBufferMemory, NULL);
    free(pMeshState);
}