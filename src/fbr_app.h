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

typedef struct FbrTimeState {
    double currentTime;
    double deltaTime;
} FbrTime;

typedef struct FbrApp {
    GLFWwindow *pWindow;
    FbrVulkan *pVulkan;
    FbrCamera *pCamera;
    FbrTime *pTime;
    FbrMesh *pMesh;
    FbrTexture *pTexture;
    FbrTexture *pExternalTextureTest;
    FbrPipeline *pPipeline;
} FbrApp;

void fbrCreateApp(FbrApp **ppAllocApp);

void fbrCleanup(FbrApp *pApp);

#endif //FABRIC_APP_H
