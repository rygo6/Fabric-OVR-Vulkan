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

    // entities
    fbrCreateCamera(pApp->pVulkan, &pApp->pCamera);
    fbrCreateMesh(pApp->pVulkan, &pApp->pMesh);
    fbrCreateTexture(pApp->pVulkan, &pApp->pTexture, "textures/test.jpg", false);
    fbrCreateTexture(pApp->pVulkan, &pApp->pExternalTextureTest, "textures/test.jpg", true);

    // Pipeline
    fbrCreatePipeline(pApp->pVulkan, pApp->pCamera, pApp->pTexture, &pApp->pPipeline);
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
    fbrCleanupTexture(pApp->pVulkan, pApp->pTexture);
    fbrCleanupTexture(pApp->pVulkan, pApp->pExternalTextureTest);
    fbrCleanupCamera(pApp->pVulkan, pApp->pCamera);
    fbrCleanupMesh(pApp->pVulkan, pApp->pMesh);
    fbrCleanupPipeline(pApp->pVulkan, pApp->pPipeline);
    fbrCleanupVulkan(pApp->pVulkan);
    glfwDestroyWindow(pApp->pWindow);
    free(pApp);
    glfwTerminate();
}