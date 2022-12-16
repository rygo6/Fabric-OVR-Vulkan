#ifndef FABRIC_APP_H
#define FABRIC_APP_H

#include "cglm/cglm.h"

#define GLFW_INCLUDE_VULKAN

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
    int screenWidth;
    int screenHeight;
    GLFWwindow *pWindow;

    FbrVulkan *pVulkan;

    FbrCamera *pCamera;
    FbrTime *pTime;
    FbrMesh *pMesh;
    FbrTexture *pTexture;
    FbrPipeline *pPipeline;
} FbrApp;

#endif //FABRIC_APP_H
