#ifndef FABRIC_APP_H
#define FABRIC_APP_H

#include <cglm/cglm.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>
#include <stdbool.h>

typedef struct FbrCamera FbrCamera;
typedef struct FbrMesh FbrMesh;
typedef struct FbrTexture FbrTexture;
typedef struct FbrVulkan FbrVulkan;
typedef struct FbrFramebuffer FbrFramebuffer;
typedef struct FbrProcess FbrProcess;
typedef struct FbrIPC FbrIPC;
typedef struct FbrNode FbrNode;
typedef struct FbrTimelineSemaphore FbrTimelineSemaphore;
typedef struct FbrNodeParent FbrNodeParent;
typedef struct FbrDescriptors FbrDescriptors;
typedef struct FbrPipelines FbrPipelines;
typedef struct FbrTransform FbrTransform;
typedef struct FbrSwap FbrSwap;

typedef enum FbrIPCTargetType FbrIPCTargetType;

typedef struct FbrTimeState {
    double currentTime;
    double deltaTime;
} FbrTime;

typedef struct FbrApp {
    bool exiting;

    bool isChild;

    GLFWwindow *pWindow;
    FbrVulkan *pVulkan;
    FbrSwap *pSwap;
    FbrCamera *pCamera;
    FbrTime *pTime;

    int cameraCount;
    FbrCamera *pCameras;


    // go in a fbr_scene?
    FbrMesh *pTestQuadMesh;
    FbrTexture *pTestQuadTexture;
    FbrTransform *pTestQuadTransform;
    VkDescriptorSet testQuadMaterialSet;
    VkDescriptorSet testQuadObjectSet;

    VkDescriptorSet pCompMaterialSets[2];

    // go in fbrvulkan?
    FbrDescriptors *pDescriptors;
    FbrPipelines *pPipelines;

    // node tests
    FbrNode *pTestNode;
    FbrNodeParent *pNodeParent;
} FbrApp;

void fbrCreateApp(FbrApp **ppAllocApp, bool isChild, long long externalTextureTest);

void fbrCleanup(FbrApp *pApp);

#endif //FABRIC_APP_H
