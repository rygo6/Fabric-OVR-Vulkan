#include "fbr_app.h"
#include "fbr_log.h"
#include "fbr_camera.h"
#include "fbr_mesh.h"
#include "fbr_texture.h"
#include "fbr_pipeline.h"
#include "fbr_vulkan.h"
#include "fbr_framebuffer.h"
#include "fbr_process.h"
#include "fbr_ipc.h"
#include "fbr_ipc_targets.h"


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

    if (!pApp->isChild) {

        fbrCreateCamera(pApp->pVulkan, &pApp->pCamera);

        fbrCreateMesh(pApp->pVulkan, &pApp->pMesh);
        fbrCreateTextureFromFile(pApp->pVulkan, &pApp->pTexture, "textures/test.jpg", true);
        fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pVulkan->swapRenderPass, &pApp->pPipeline);
        fbrInitDescriptorSet(pApp->pVulkan, pApp->pPipeline, pApp->pCamera, pApp->pTexture->imageView, &pApp->pVulkan->swapDescriptorSet);

        // render texture
//        fbrCreateMesh(pApp->pVulkan, &pApp->pMeshExternalTest);
//        fbrImportTexture(pApp->pVulkan, &pApp->pTextureExternalTest, pApp->pTexture->externalMemory, pApp->pTexture->width, pApp->pTexture->height);
//        glm_vec3_add(pApp->pMeshExternalTest->transform.pos, (vec3) {1,0,0}, pApp->pMeshExternalTest->transform.pos);
//        fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTextureExternalTest->imageView, pApp->pVulkan->swapRenderPass, &pApp->pPipelineExternalTest);


        //render to framebuffer
        fbrCreateFrameBuffer(pApp->pVulkan, &pApp->pFramebuffer);
//        fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTexture->imageView, pApp->pFramebuffer->swapRenderPass, &pApp->pFramebufferPipeline); // is this pipeline needed!?

        fbrCreateMesh(pApp->pVulkan, &pApp->pMeshExternalTest);
        glm_vec3_add(pApp->pMeshExternalTest->transform.pos, (vec3) {1,0,0}, pApp->pMeshExternalTest->transform.pos);
        fbrInitDescriptorSet(pApp->pVulkan, pApp->pPipeline, pApp->pCamera, pApp->pFramebuffer->pTexture->imageView, &pApp->testProcessDescriptorSet);
//        fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pFramebuffer->pTexture->imageView, pApp->pVulkan->swapRenderPass, &pApp->pPipelineExternalTest);

        fbrCreateProcess(&pApp->pTestProcess);


        HANDLE camDupHandle;
        DuplicateHandle(GetCurrentProcess(), pApp->pCamera->ubo.externalMemory, pApp->pTestProcess->pi.hProcess, &camDupHandle, 0, false, DUPLICATE_SAME_ACCESS);
        FbrIPCParamImportCamera camParam =  {
                .handle = camDupHandle,
        };
        fbrIPCEnque(pApp->pTestProcess->pProducerIPC, FBR_IPC_TARGET_IMPORT_CAMERA, &camParam);
        printf("external camera handle d %lld\n", camParam.handle);
        printf("external camera handle p %p\n", camParam.handle);


        HANDLE texDupHandle;
        DuplicateHandle(GetCurrentProcess(), pApp->pFramebuffer->pTexture->externalMemory, pApp->pTestProcess->pi.hProcess, &texDupHandle, 0, false, DUPLICATE_SAME_ACCESS);
        FbrIPCParamImportFrameBuffer texParam =  {
                .handle = texDupHandle,
                .width = pApp->pFramebuffer->pTexture->width,
                .height = pApp->pFramebuffer->pTexture->height
        };
        fbrIPCEnque(pApp->pTestProcess->pProducerIPC, FBR_IPC_TARGET_IMPORT_FRAMEBUFFER, &texParam);
        printf("external pTexture handle d %lld\n", texParam.handle);
        printf("external pTexture handle p %p\n", texParam.handle);

    } else {

        fbrCreateReceiverIPC(&pApp->pParentProcessReceiverIPC);

        // for debugging ipc now, wait for camera
        while(fbrIPCPollDeque(pApp, pApp->pParentProcessReceiverIPC) != 0) {
            FBR_LOG_DEBUG("Wait Message", pApp->pParentProcessReceiverIPC->pIPCBuffer->tail, pApp->pParentProcessReceiverIPC->pIPCBuffer->head);
//            Sleep(1000);
        }

        // for debugging ipc now, wait for framebuffer
        while(fbrIPCPollDeque(pApp, pApp->pParentProcessReceiverIPC) != 0) {
            FBR_LOG_DEBUG("Wait Message", pApp->pParentProcessReceiverIPC->pIPCBuffer->tail, pApp->pParentProcessReceiverIPC->pIPCBuffer->head);
//            Sleep(1000);
        }

        fbrCreateMesh(pApp->pVulkan, &pApp->pMesh);
        glm_vec3_add(pApp->pMesh->transform.pos, (vec3) {1,0,0}, pApp->pMesh->transform.pos);
        fbrCreateTextureFromFile(pApp->pVulkan, &pApp->pTexture, "textures/UV_Grid_Sm.jpg", false);
//        fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pVulkan->swapRenderPass, &pApp->pPipeline);
//        fbrInitDescriptorSet(pApp->pVulkan, pApp->pPipeline, pApp->pCamera, pApp->pTexture->imageView, &pApp->pVulkan->swapDescriptorSet);

        fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pFramebuffer->renderPass, &pApp->pPipeline);
        fbrInitDescriptorSet(pApp->pVulkan, pApp->pPipeline, pApp->pCamera, pApp->pTexture->imageView, &pApp->testProcessDescriptorSet);
    }
}

void fbrCreateApp(FbrApp **ppAllocApp, bool isChild, long long externalTextureTest) {
    *ppAllocApp = calloc(1, sizeof(FbrApp));
    FbrApp *pApp = *ppAllocApp;
    pApp->pTime = calloc(1, sizeof(FbrTime));
    pApp->isChild = isChild;

    initWindow(pApp);
    fbrInitInput(pApp);
    fbrCreateVulkan(pApp, &pApp->pVulkan, FBR_DEFAULT_SCREEN_WIDTH, FBR_DEFAULT_SCREEN_HEIGHT, true);

    initEntities(pApp, externalTextureTest);
}

void fbrCleanup(FbrApp *pApp) {
    FBR_LOG_DEBUG("cleaning up!");
    fbrDestroyIPC(pApp->pParentProcessReceiverIPC);

    fbrCleanupCamera(pApp->pVulkan, pApp->pCamera);
    fbrDestroyTexture(pApp->pVulkan, pApp->pTexture);
    fbrDestroyTexture(pApp->pVulkan, pApp->pTextureExternalTest);
    fbrCleanupMesh(pApp->pVulkan, pApp->pMesh);
    fbrCleanupMesh(pApp->pVulkan, pApp->pMeshExternalTest);
    fbrCleanupPipeline(pApp->pVulkan, pApp->pPipeline);
//    fbrCleanupPipeline(pApp->pVulkan, pApp->pPipelineExternalTest);

    fbrDestroyFrameBuffer(pApp->pVulkan, pApp->pFramebuffer);

    fbrCleanupVulkan(pApp->pVulkan);
    glfwDestroyWindow(pApp->pWindow);
    free(pApp);
    glfwTerminate();
}