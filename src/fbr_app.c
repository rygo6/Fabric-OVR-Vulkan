#include "fbr_app.h"
#include "fbr_log.h"
#include "fbr_camera.h"
#include "fbr_mesh.h"
#include "fbr_texture.h"
#include "fbr_pipeline.h"
#include "fbr_vulkan.h"
#include "fbr_ipc.h"
#include "fbr_ipc_targets.h"
#include "fbr_node.h"
#include "fbr_process.h"
#include "fbr_node_parent.h"


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

    if (!pApp->isChild) {

        fbrCreateCamera(pApp->pVulkan, &pApp->pCamera);

        fbrCreateMesh(pApp->pVulkan, &pApp->pTestQuadMesh);
        fbrCreateTextureFromFile(pApp->pVulkan, &pApp->pTestTexture, "textures/test.jpg", true);
        fbrCreatePipeline(pApp->pVulkan,
                          pApp->pVulkan->renderPass,
                          "./shaders/vert.spv",
                          "./shaders/frag.spv",
                          &pApp->pSwapPipeline);
        fbrInitDescriptorSet(pApp->pVulkan,
                             pApp->pCamera,
                             pApp->pSwapPipeline->descriptorSetLayout,
                             pApp->pTestTexture->imageView,
                             &pApp->pVulkan->swapDescriptorSet);

        // render texture
//        fbrCreateMesh(pApp->pVulkan, &pApp->pCompMesh);
//        fbrImportTexture(pApp->pVulkan, &pApp->pTextureExternalTest, pApp->pTestTexture->externalMemory, pApp->pTestTexture->width, pApp->pTestTexture->height);
//        glm_vec3_add(pApp->pCompMesh->transform.pos, (vec3) {1,0,0}, pApp->pCompMesh->transform.pos);
//        fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTextureExternalTest->imageView, pApp->pVulkan->renderPass, &pApp->pPipelineExternalTest);

        fbrCreateNode(pApp, "TestNode", &pApp->pTestNode);
        glm_vec3_add(pApp->pTestNode->transform.pos, (vec3) {1, 0, 0}, pApp->pTestNode->transform.pos);

        fbrCreatePipeline(pApp->pVulkan,
                          pApp->pVulkan->renderPass,
                          "./shaders/vert_comp.spv",
//                          "./shaders/vert.spv",
                          "./shaders/frag.spv",
                          &pApp->pCompPipeline);
        fbrInitDescriptorSet(pApp->pVulkan,
                             pApp->pCamera,
                             pApp->pCompPipeline->descriptorSetLayout,
                             pApp->pTestNode->pFramebuffers[0]->pTexture->imageView,
                             &pApp->compDescriptorSet);


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
                        pApp->pTestNode->pFramebuffers[0]->pTexture->externalMemory,
                        pApp->pTestNode->pProcess->pi.hProcess,
                        &tex0DupHandle,
                        0,
                        false,
                        DUPLICATE_SAME_ACCESS);
        FBR_LOG_DEBUG("export", tex0DupHandle);
        HANDLE tex1DupHandle;
        DuplicateHandle(GetCurrentProcess(),
                        pApp->pTestNode->pFramebuffers[1]->pTexture->externalMemory,
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
                .framebufferWidth = pApp->pTestNode->pFramebuffers[0]->pTexture->extent.width,
                .framebufferHeight = pApp->pTestNode->pFramebuffers[0]->pTexture->extent.height,
                .framebuffer0ExternalHandle = tex0DupHandle,
                .framebuffer1ExternalHandle = tex1DupHandle,
                .vertexUBO0ExternalHandle = vert0DupHandle,
                .vertexUBO1ExternalHandle = vert1DupHandle,
                .parentSemaphoreExternalHandle = parentSemDupHandle,
                .childSemaphoreExternalHandle = childSemDupHandle,
        };
        fbrIPCEnque(pApp->pTestNode->pProducerIPC, FBR_IPC_TARGET_IMPORT_NODE_PARENT, &importNodeParentParam);
    } else {

        fbrCreateNodeParent(&pApp->pNodeParent);

        // for debugging ipc now, wait for camera
        while(fbrIPCPollDeque(pApp, pApp->pNodeParent->pReceiverIPC) != 0) {
            FBR_LOG_DEBUG("Wait Message", pApp->pNodeParent->pReceiverIPC->pIPCBuffer->tail, pApp->pNodeParent->pReceiverIPC->pIPCBuffer->head);
//            Sleep(1000);
        }

        fbrCreateMesh(pApp->pVulkan, &pApp->pTestQuadMesh);
        glm_vec3_add(pApp->pTestQuadMesh->transform.pos, (vec3) {1, 0, 0}, pApp->pTestQuadMesh->transform.pos);
        fbrCreateTextureFromFile(pApp->pVulkan, &pApp->pTestTexture, "textures/UV_Grid_Sm.jpg", false);

        fbrCreatePipeline(pApp->pVulkan,
                          pApp->pNodeParent->pFramebuffer->renderPass,
                          "./shaders/vert.spv",
                          "./shaders/frag.spv",
//                          "./shaders/frag_crasher.spv",
                          &pApp->pSwapPipeline);
        fbrInitDescriptorSet(pApp->pVulkan,
                             pApp->pNodeParent->pCamera,
                             pApp->pSwapPipeline->descriptorSetLayout,
                             pApp->pTestTexture->imageView,
                             &pApp->pNodeParent->parentFramebufferDescriptorSet);
    }
}

void fbrCreateApp(FbrApp **ppAllocApp, bool isChild, long long externalTextureTest) {
    *ppAllocApp = calloc(1, sizeof(FbrApp));
    FbrApp *pApp = *ppAllocApp;
    pApp->pTime = calloc(1, sizeof(FbrTime));
    pApp->isChild = isChild;

    initWindow(pApp);
    if (!pApp->isChild)
        fbrInitInput(pApp);
    fbrCreateVulkan(pApp,
                    &pApp->pVulkan,
                    FBR_DEFAULT_SCREEN_WIDTH,
                    FBR_DEFAULT_SCREEN_HEIGHT,
                    true);

    initEntities(pApp, externalTextureTest);
}

void fbrCleanup(FbrApp *pApp) {
    FBR_LOG_DEBUG("cleaning up!");

    // todo this needs to be better
    if (pApp->isChild) {
        fbrDestroyNodeParent(pApp->pVulkan, pApp->pNodeParent);
    }
    else{
        fbrDestroyNode(pApp->pVulkan, pApp->pTestNode);
        fbrDestroyCamera(pApp->pVulkan, pApp->pCamera);
        fbrCleanupPipeline(pApp->pVulkan, pApp->pSwapPipeline);
        vkFreeDescriptorSets(pApp->pVulkan->device, pApp->pVulkan->descriptorPool, 1, &pApp->compDescriptorSet);
    }

    fbrDestroyTexture(pApp->pVulkan, pApp->pTestTexture);
    fbrCleanupMesh(pApp->pVulkan, pApp->pTestQuadMesh);

    fbrCleanupVulkan(pApp->pVulkan);

    glfwDestroyWindow(pApp->pWindow);

    free(pApp);

    glfwTerminate();
}