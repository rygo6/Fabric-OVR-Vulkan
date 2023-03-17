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
typedef struct FbrNode FbrNode;
typedef struct FbrTimelineSemaphore FbrTimelineSemaphore;
typedef struct FbrNodeParent FbrNodeParent;
typedef struct FbrDescriptors_Std FbrDescriptors_Std;

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
    FbrCamera *pCamera;
    FbrTime *pTime;

    // go in a fbr_scene?
    FbrMesh *pTestQuadMesh;
    FbrTexture *pTestQuadTexture;
    VkDescriptorSet pTestQuadDescriptorSet;

    // go in swap object
    FbrPipeline *pSwapPipeline;

    FbrPipeline *pCompPipeline;
    VkDescriptorSet pCompDescriptorSets[2];

    // go in fbrvulkan?
    FbrDescriptors_Std *pDescriptorsStd;

    // node tests
    FbrNode *pTestNode;
    FbrNodeParent *pNodeParent;
} FbrApp;

void fbrCreateApp(FbrApp **ppAllocApp, bool isChild, long long externalTextureTest);

void fbrCleanup(FbrApp *pApp);

#endif //FABRIC_APP_H
