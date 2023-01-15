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

    fbrCreateCamera(pApp->pVulkan, &pApp->pCamera);

    if (!pApp->isChild) {

        fbrCreateMesh(pApp->pVulkan, &pApp->pMesh);
        fbrCreateTexture(pApp->pVulkan, &pApp->pTexture, "textures/test.jpg", true);
        fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTexture->imageView, pApp->pVulkan->renderPass, &pApp->pPipeline);

        printf("external texture handle d %lld\n", pApp->pTexture->sharedMemory);
        printf("external texture handle p %p\n", pApp->pTexture->sharedMemory);

        fbrCreateMesh(pApp->pVulkan, &pApp->pMeshExternalTest);
        fbrImportTexture(pApp->pVulkan, &pApp->pTextureExternalTest, pApp->pTexture->sharedMemory);
        glm_vec3_add(pApp->pMeshExternalTest->transform.pos, (vec3) {1,0,0}, pApp->pMeshExternalTest->transform.pos);
        fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTextureExternalTest->imageView, pApp->pVulkan->renderPass, &pApp->pPipelineExternalTest);

        if (fbrCreateProducerIPC(&pApp->pIPC) != 0) {
            return;
        }

        fbrCreateProcess(&pApp->pTestProcess, pApp->pTexture->sharedMemory, pApp->pCamera->ubo.sharedMemory);

        HANDLE dupHandle;
        DuplicateHandle(GetCurrentProcess(), pApp->pTexture->sharedMemory, pApp->pTestProcess->pi.hProcess, &dupHandle, 0, false, DUPLICATE_SAME_ACCESS);
        FBR_LOG_DEBUG("Handles", pApp->pTexture->sharedMemory, dupHandle);

        fbrIPCEnquePtr(pApp->pIPC, dupHandle);

    } else {

        fbrCreateReceiverIPC(&pApp->pIPC);

        while(!fbrIPCDequeAvailable(pApp->pIPC)) {
            printf("Wait Message: %d\n", (int)pApp->pIPC->pBuffer);
        }

        HANDLE sharedMemory;
        fbrIPCDequePtr(pApp->pIPC, &sharedMemory);

        printf("external texture handle d %lld\n", sharedMemory);
        printf("external texture handle p %p\n", sharedMemory);

        fbrCreateMesh(pApp->pVulkan, &pApp->pMesh);
        fbrImportTexture(pApp->pVulkan, &pApp->pTexture, sharedMemory);
        fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTexture->imageView, pApp->pVulkan->renderPass, &pApp->pPipeline);


        fbrCreateMesh(pApp->pVulkan, &pApp->pMeshExternalTest);
        fbrImportTexture(pApp->pVulkan, &pApp->pTextureExternalTest, sharedMemory);
        glm_vec3_add(pApp->pMeshExternalTest->transform.pos, (vec3) {1,0,0}, pApp->pMeshExternalTest->transform.pos);
        fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTextureExternalTest->imageView, pApp->pVulkan->renderPass, &pApp->pPipelineExternalTest);
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
    fbrDestroyIPC(pApp->pIPC);

    fbrCleanupCamera(pApp->pVulkan, pApp->pCamera);
    fbrCleanupTexture(pApp->pVulkan, pApp->pTexture);
    fbrCleanupTexture(pApp->pVulkan, pApp->pTextureExternalTest);
    fbrCleanupMesh(pApp->pVulkan, pApp->pMesh);
    fbrCleanupMesh(pApp->pVulkan, pApp->pMeshExternalTest);
    fbrCleanupPipeline(pApp->pVulkan, pApp->pPipeline);
    fbrCleanupPipeline(pApp->pVulkan, pApp->pPipelineExternalTest);

    fbrDestroyFramebuffer(pApp->pVulkan, pApp->pFramebuffer);

    fbrCleanupVulkan(pApp->pVulkan);
    glfwDestroyWindow(pApp->pWindow);
    free(pApp);
    glfwTerminate();
}