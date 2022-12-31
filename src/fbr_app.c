#include "fbr_app.h"
#include "fbr_log.h"
#include "fbr_camera.h"
#include "fbr_mesh.h"
#include "fbr_texture.h"
#include "fbr_pipeline.h"
#include "fbr_vulkan.h"

#define FBR_DEFAULT_SCREEN_WIDTH 800
#define FBR_DEFAULT_SCREEN_HEIGHT 600

static void initWindow(FbrApp *pApp) {
    FBR_LOG_DEBUG("initializing window!");

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    pApp->pWindow = glfwCreateWindow(FBR_DEFAULT_SCREEN_WIDTH, FBR_DEFAULT_SCREEN_HEIGHT, "Fabric", NULL, NULL);
    if (pApp->pWindow == NULL) {
        FBR_LOG_ERROR("unable to initialize GLFW Window!");
    }
}

static void initEntities(FbrApp *pApp) {
    FBR_LOG_DEBUG("initializing vulkan!");

    fbrCreateCamera(pApp->pVulkan, &pApp->pCamera);

    fbrCreateMesh(pApp->pVulkan, &pApp->pMesh);
    fbrCreateTexture(pApp->pVulkan, &pApp->pTexture, "textures/test.jpg", true);
    fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTexture, &pApp->pPipeline);

    fbrCreateMesh(pApp->pVulkan, &pApp->pMeshExternalTest);
//    fbrCreateTexture(pApp->pVulkan, &pApp->pTextureExternalTest, "textures/test.jpg", true);
    fbrImportTexture(pApp->pVulkan, &pApp->pTextureExternalTest, pApp->pTexture->sharedMemory);
    fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTextureExternalTest, &pApp->pPipelineExternalTest);
}

void fbrCreateApp(FbrApp **ppAllocApp) {
    *ppAllocApp = calloc(1, sizeof(FbrApp));
    FbrApp *pApp = *ppAllocApp;
    pApp->pTime = calloc(1, sizeof(FbrTime));

    initWindow(pApp);
    fbrInitInput(pApp);
    fbrCreateVulkan(pApp, &pApp->pVulkan, FBR_DEFAULT_SCREEN_WIDTH, FBR_DEFAULT_SCREEN_HEIGHT, true);

    initEntities(pApp);
}

void fbrCleanup(FbrApp *pApp) {
    FBR_LOG_DEBUG("cleaning up!");
    fbrCleanupCamera(pApp->pVulkan, pApp->pCamera);
    fbrCleanupTexture(pApp->pVulkan, pApp->pTexture);
    fbrCleanupTexture(pApp->pVulkan, pApp->pTextureExternalTest);
    fbrCleanupMesh(pApp->pVulkan, pApp->pMesh);
    fbrCleanupMesh(pApp->pVulkan, pApp->pMeshExternalTest);
    fbrCleanupPipeline(pApp->pVulkan, pApp->pPipeline);
    fbrCleanupPipeline(pApp->pVulkan, pApp->pPipelineExternalTest);
    fbrCleanupVulkan(pApp->pVulkan);
    glfwDestroyWindow(pApp->pWindow);
    free(pApp);
    glfwTerminate();
}