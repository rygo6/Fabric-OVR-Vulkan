#include "fbr_app.h"
#include "fbr_log.h"
#include "fbr_camera.h"
#include "fbr_mesh.h"
#include "fbr_texture.h"
#include "fbr_vulkan.h"
#include "fbr_ipc.h"
#include "fbr_ipc_targets.h"
#include "fbr_node.h"
#include "fbr_process.h"
#include "fbr_node_parent.h"
#include "fbr_descriptors.h"
#include "fbr_pipelines.h"
#include "fbr_transform.h"
#include "fbr_swap.h"

#define FBR_DEFAULT_SCREEN_WIDTH 800
#define FBR_DEFAULT_SCREEN_HEIGHT 600

static void initWindow(FbrApp *pApp) {
    FBR_LOG_DEBUG("initializing window!");

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    if (pApp->isChild){
        pApp->pWindow = glfwCreateWindow(FBR_DEFAULT_SCREEN_WIDTH, FBR_DEFAULT_SCREEN_HEIGHT, "Fabric Child", NULL, NULL);
        glfwSetWindowPos(pApp->pWindow, 100 + FBR_DEFAULT_SCREEN_WIDTH, 100);
    }
    else {
        pApp->pWindow = glfwCreateWindow(FBR_DEFAULT_SCREEN_WIDTH, FBR_DEFAULT_SCREEN_HEIGHT, "Fabric", NULL, NULL);
        glfwSetWindowPos(pApp->pWindow, 100, 100);
    }

    if (pApp->pWindow == NULL) {
        FBR_LOG_ERROR("unable to initialize GLFW Window!");
    }
}

static void initEntities(FbrApp *pApp, long long externalTextureTest) {
    FBR_LOG_DEBUG("initializing vulkan!");

    FbrVulkan *pVulkan = pApp->pVulkan;

    fbrCreateDescriptors(pVulkan, &pApp->pDescriptors);
    fbrCreatePipelines(pVulkan, pApp->pDescriptors, &pApp->pPipelines);

    if (!pApp->isChild) {

        fbrCreateCamera(pVulkan,
                        &pApp->pCamera);
        fbrCreateSetGlobal(pApp->pVulkan,
                           pApp->pDescriptors->setLayoutGlobal,
                           pApp->pCamera,
                           &pApp->pDescriptors->setGlobal);

        // First Quad
        fbrCreateMesh(pVulkan,
                      &pApp->pTestQuadMesh);
        fbrCreateTransform(pVulkan,
                           &pApp->pTestQuadTransform);
        fbrCreateTextureFromFile(pVulkan,
                                 false,
                                 "textures/test.jpg",
                                 &pApp->pTestQuadTexture);
        fbrCreateSetMaterial(pApp->pVulkan,
                           pApp->pDescriptors->setLayoutMaterial,
                           pApp->pTestQuadTexture,
                           &pApp->testQuadMaterialSet);
        fbrCreateSetObject(pApp->pVulkan,
                             pApp->pDescriptors->setLayoutObject,
                             pApp->pTestQuadTransform,
                             &pApp->testQuadObjectSet);

        // Comp Node Quad
        fbrCreateNode(pApp, "TestNode", &pApp->pTestNode);
        glm_vec3_add(pApp->pTestNode->pTransform->pos,
                     (vec3) {1, 0, 0},
                     pApp->pTestNode->pTransform->pos);
//        fbrCreateSetMaterial(pApp->pVulkan,
//                             pApp->pDescriptors->setLayoutMaterial,
//                             pApp->pTestNode->pFramebuffers[0]->pColorTexture,
//                             &pApp->pCompMaterialSets[0]);
//        fbrCreateSetMaterial(pApp->pVulkan,
//                             pApp->pDescriptors->setLayoutMaterial,
//                             pApp->pTestNode->pFramebuffers[1]->pColorTexture,
//                             &pApp->pCompMaterialSets[1]);
//        fbrCreateSetObject(pApp->pVulkan,
//                           pApp->pDescriptors->setLayoutObject,
//                           pApp->pTestNode->pTransform,
//                           &pApp->compObjectSet);
        for (int i = 0; i < 2; ++i) {
            fbrCreateSetNode(pApp->pVulkan,
                             pApp->pDescriptors->setLayoutNode,
                             pApp->pTestNode->pTransform,
                             pApp->pTestNode->pFramebuffers[i]->pDepthTexture,
                             pApp->pTestNode->pFramebuffers[i]->pColorTexture,
                             &pApp->pCompMaterialSets[i]);
        }


        // todo below can go into create node
        HANDLE camDupHandle;
        DuplicateHandle(GetCurrentProcess(),
                        pApp->pCamera->pUBO->externalMemory,
                        pApp->pTestNode->pProcess->pi.hProcess,
                        &camDupHandle,
                        0,
                        false,
                        DUPLICATE_SAME_ACCESS);
        FBR_LOG_DEBUG("export", camDupHandle);

        HANDLE tex0DupHandle;
        DuplicateHandle(GetCurrentProcess(),
                        pApp->pTestNode->pFramebuffers[0]->pColorTexture->externalMemory,
                        pApp->pTestNode->pProcess->pi.hProcess,
                        &tex0DupHandle,
                        0,
                        false,
                        DUPLICATE_SAME_ACCESS);
        FBR_LOG_DEBUG("export", tex0DupHandle);
        HANDLE tex1DupHandle;
        DuplicateHandle(GetCurrentProcess(),
                        pApp->pTestNode->pFramebuffers[1]->pColorTexture->externalMemory,
                        pApp->pTestNode->pProcess->pi.hProcess,
                        &tex1DupHandle,
                        0,
                        false,
                        DUPLICATE_SAME_ACCESS);
        FBR_LOG_DEBUG("export", tex1DupHandle);

        HANDLE vert0DupHandle;
        DuplicateHandle(GetCurrentProcess(),
                        pApp->pTestNode->pVertexUBOs[0]->externalMemory,
                        pApp->pTestNode->pProcess->pi.hProcess,
                        &vert0DupHandle,
                        0,
                        false,
                        DUPLICATE_SAME_ACCESS);
        FBR_LOG_DEBUG("export", vert0DupHandle);
        HANDLE vert1DupHandle;
        DuplicateHandle(GetCurrentProcess(),
                        pApp->pTestNode->pVertexUBOs[1]->externalMemory,
                        pApp->pTestNode->pProcess->pi.hProcess,
                        &vert1DupHandle,
                        0,
                        false,
                        DUPLICATE_SAME_ACCESS);
        FBR_LOG_DEBUG("export", vert1DupHandle);

        HANDLE parentSemDupHandle;
        DuplicateHandle(GetCurrentProcess(),
                        pApp->pVulkan->pMainSemaphore->externalHandle,
                        pApp->pTestNode->pProcess->pi.hProcess,
                        &parentSemDupHandle,
                        0,
                        false,
                        DUPLICATE_SAME_ACCESS);
        FBR_LOG_DEBUG("export", parentSemDupHandle);

        HANDLE childSemDupHandle;
        DuplicateHandle(GetCurrentProcess(),
                        pApp->pTestNode->pChildSemaphore->externalHandle,
                        pApp->pTestNode->pProcess->pi.hProcess,
                        &childSemDupHandle,
                        0,
                        false,
                        DUPLICATE_SAME_ACCESS);
        FBR_LOG_DEBUG("export", childSemDupHandle);

        FbrIPCParamImportNodeParent importNodeParentParam =  {
                .cameraExternalHandle = camDupHandle,
                .framebufferWidth = pApp->pTestNode->pFramebuffers[0]->pColorTexture->extent.width,
                .framebufferHeight = pApp->pTestNode->pFramebuffers[0]->pColorTexture->extent.height,
                .framebuffer0ExternalHandle = tex0DupHandle,
                .framebuffer1ExternalHandle = tex1DupHandle,
                .vertexUBO0ExternalHandle = vert0DupHandle,
                .vertexUBO1ExternalHandle = vert1DupHandle,
                .parentSemaphoreExternalHandle = parentSemDupHandle,
                .childSemaphoreExternalHandle = childSemDupHandle,
        };
        fbrIPCEnque(pApp->pTestNode->pProducerIPC, FBR_IPC_TARGET_IMPORT_NODE_PARENT, &importNodeParentParam);
    } else {

        fbrCreateNodeParent(pVulkan, &pApp->pNodeParent);
        glm_vec3_add(pApp->pNodeParent->pTransform->pos,
                     (vec3) {1, 0, 0},
                     pApp->pNodeParent->pTransform->pos);

        // for debugging ipc now, wait for camera
        while(fbrIPCPollDeque(pApp, pApp->pNodeParent->pReceiverIPC) != 0) {
            FBR_LOG_DEBUG("Wait Message", pApp->pNodeParent->pReceiverIPC->pIPCBuffer->tail, pApp->pNodeParent->pReceiverIPC->pIPCBuffer->head);
        }
        fbrCreateSetGlobal(pApp->pVulkan,
                           pApp->pDescriptors->setLayoutGlobal,
                           pApp->pNodeParent->pCamera,
                           &pApp->pDescriptors->setGlobal);

        fbrCreateMesh(pApp->pVulkan, &pApp->pTestQuadMesh);
        fbrCreateTransform(pVulkan,
                           &pApp->pTestQuadTransform);
        glm_vec3_add(pApp->pTestQuadTransform->pos,
                     (vec3) {1, 0, 0},
                     pApp->pTestQuadTransform->pos);
        fbrCreateTextureFromFile(pVulkan,
                                 false,
                                 "textures/uvgrid.jpg",
                                 &pApp->pTestQuadTexture);
        fbrCreateSetMaterial(pApp->pVulkan,
                             pApp->pDescriptors->setLayoutMaterial,
                             pApp->pTestQuadTexture,
                             &pApp->testQuadMaterialSet);
        fbrCreateSetObject(pApp->pVulkan,
                           pApp->pDescriptors->setLayoutObject,
                           pApp->pTestQuadTransform,
                           &pApp->testQuadObjectSet);
    }
}

void fbrCreateApp(FbrApp **ppAllocApp, bool isChild, long long externalTextureTest) {
    *ppAllocApp = calloc(1, sizeof(FbrApp));
    FbrApp *pApp = *ppAllocApp;
    pApp->pTime = calloc(1, sizeof(FbrTime));
    pApp->isChild = isChild;

    initWindow(pApp);

    VkExtent2D extent = { FBR_DEFAULT_SCREEN_WIDTH, FBR_DEFAULT_SCREEN_HEIGHT };

    fbrCreateVulkan(pApp,
                    &pApp->pVulkan,
                    FBR_DEFAULT_SCREEN_WIDTH,
                    FBR_DEFAULT_SCREEN_HEIGHT,
                    true);
    if (!pApp->isChild) {
        fbrCreateSwap(pApp->pVulkan,
                      extent,
                      &pApp->pSwap);
        fbrInitInput(pApp);
    }

    initEntities(pApp, externalTextureTest);
}

void fbrCleanup(FbrApp *pApp) {
    FBR_LOG_DEBUG("cleaning up!");

    FbrVulkan *pVulkan = pApp->pVulkan;

    fbrDestroySwap(pVulkan, pApp->pSwap);

    // todo this needs to be better
    if (pApp->isChild) {
        fbrDestroyNodeParent(pVulkan, pApp->pNodeParent);
    }
    else{
        fbrDestroyNode(pVulkan, pApp->pTestNode);
        fbrDestroyCamera(pVulkan, pApp->pCamera);
        fbrDestroyPipelines(pVulkan, pApp->pPipelines);
        vkFreeDescriptorSets(pVulkan->device, pApp->pVulkan->descriptorPool, 1, &pApp->pCompMaterialSets[0]);
        vkFreeDescriptorSets(pVulkan->device, pApp->pVulkan->descriptorPool, 1, &pApp->pCompMaterialSets[1]);
        vkFreeDescriptorSets(pVulkan->device, pApp->pVulkan->descriptorPool, 1, &pApp->compObjectSet);
    }

    fbrDestroyTexture(pVulkan, pApp->pTestQuadTexture);
    fbrCleanupMesh(pVulkan, pApp->pTestQuadMesh);
    vkFreeDescriptorSets(pVulkan->device, pApp->pVulkan->descriptorPool, 1, &pApp->testQuadMaterialSet);
    vkFreeDescriptorSets(pVulkan->device, pApp->pVulkan->descriptorPool, 1, &pApp->testQuadObjectSet);

    fbrDestroyDescriptors(pVulkan, pApp->pDescriptors);

    fbrCleanupVulkan(pVulkan);

    glfwDestroyWindow(pApp->pWindow);

    free(pApp);

    glfwTerminate();
}