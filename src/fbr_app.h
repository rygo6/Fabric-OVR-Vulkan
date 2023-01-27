#ifndef FABRIC_APP_H
#define FABRIC_APP_H

#include <cglm/cglm.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

typedef struct FbrCamera FbrCamera;
typedef struct FbrMesh FbrMesh;
typedef struct FbrPipeline FbrPipeline;
typedef struct FbrTexture FbrTexture;
typedef struct FbrVulkan FbrVulkan;
typedef struct FbrFramebuffer FbrFramebuffer;
typedef struct FbrProcess FbrProcess;
typedef struct FbrIPC FbrIPC;

typedef struct FbrTimeState {
    double currentTime;
    double deltaTime;
} FbrTime;

typedef struct FbrApp {
    bool exiting;

    bool isChild;

    GLFWwindow *pWindow;
    FbrVulkan *pVulkan;
    FbrCamera *pCamera;
    FbrTime *pTime;

    FbrFramebuffer *pFramebuffer;

    FbrMesh *pMesh;
    FbrTexture *pTexture;
    FbrPipeline *pPipeline;

    FbrIPC *pIPC;
    FbrProcess *pTestProcess;
    FbrMesh *pMeshExternalTest;
    FbrTexture *pTextureExternalTest;
    FbrPipeline *pPipelineExternalTest;
    FbrPipeline *pTestPipeline;

} FbrApp;

void fbrCreateApp(FbrApp **ppAllocApp, bool isChild, long long externalTextureTest);

void fbrCleanup(FbrApp *pApp);

#endif //FABRIC_APP_H
