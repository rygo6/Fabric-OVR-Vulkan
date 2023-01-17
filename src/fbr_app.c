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

//    FbrCamera *pTestCamera;
//    fbrImportCamera(pApp->pVulkan, &pTestCamera, pApp->pCamera->ubo.externalMemory);

    if (!pApp->isChild) {

        fbrCreateCamera(pApp->pVulkan, &pApp->pCamera);

        fbrCreateMesh(pApp->pVulkan, &pApp->pMesh);
        fbrCreateTexture(pApp->pVulkan, &pApp->pTexture, "textures/test.jpg", true);
        fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTexture->imageView, pApp->pVulkan->renderPass, &pApp->pPipeline);

        fbrCreateMesh(pApp->pVulkan, &pApp->pMeshExternalTest);
        fbrImportTexture(pApp->pVulkan, &pApp->pTextureExternalTest, pApp->pTexture->externalMemory, pApp->pTexture->width, pApp->pTexture->height);
        glm_vec3_add(pApp->pMeshExternalTest->transform.pos, (vec3) {1,0,0}, pApp->pMeshExternalTest->transform.pos);
        fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTextureExternalTest->imageView, pApp->pVulkan->renderPass, &pApp->pPipelineExternalTest);

        if (fbrCreateProducerIPC(&pApp->pIPC) != 0) {
            return;
        }

        fbrCreateProcess(&pApp->pTestProcess, pApp->pTexture->externalMemory, pApp->pCamera->ubo.externalMemory);


        HANDLE camDupHandle;
        DuplicateHandle(GetCurrentProcess(), pApp->pCamera->ubo.externalMemory, pApp->pTestProcess->pi.hProcess, &camDupHandle, 0, false, DUPLICATE_SAME_ACCESS);
        FbrIPCExternalCameraUBO camParam =  {
                .handle = camDupHandle,
        };
        fbrIPCEnque(pApp->pIPC->pIPCBuffer, FBR_IPC_TARGET_EXTERNAL_CAMERA_UBO, &camParam);
        printf("external camera handle d %lld\n", camParam.handle);
        printf("external camera handle p %p\n", camParam.handle);

        HANDLE texDupHandle;
        DuplicateHandle(GetCurrentProcess(), pApp->pTexture->externalMemory, pApp->pTestProcess->pi.hProcess, &texDupHandle, 0, false, DUPLICATE_SAME_ACCESS);
        FbrIPCExternalTextureParam texParam =  {
                .handle = texDupHandle,
                .width = pApp->pTexture->width,
                .height = pApp->pTexture->height
        };
        fbrIPCEnque(pApp->pIPC->pIPCBuffer, FBR_IPC_TARGET_EXTERNAL_TEXTURE, &texParam);
        printf("external texture handle d %lld\n", texParam.handle);
        printf("external texture handle p %p\n", texParam.handle);

    } else {

        fbrCreateReceiverIPC(&pApp->pIPC);

        // for debugging ipc now
        while(fbrIPCPollDeque(pApp, pApp->pIPC) != 0) {
            FBR_LOG_DEBUG("Wait Message", pApp->pIPC->pIPCBuffer->tail, pApp->pIPC->pIPCBuffer->head);
//            Sleep(1000);
        }

        while(fbrIPCPollDeque(pApp, pApp->pIPC) != 0) {
            FBR_LOG_DEBUG("Wait Message", pApp->pIPC->pIPCBuffer->tail, pApp->pIPC->pIPCBuffer->head);
//            Sleep(1000);
        }
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