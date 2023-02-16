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

    FbrMesh *pMesh;
    FbrTexture *pTexture;
    FbrPipeline *pPipeline;

    // parent tests
    FbrNode *pTestNode;
    FbrMesh *pTestNodeDisplayMesh;
    VkDescriptorSet testNodeDisplayDescriptorSet;

    // child tests
    FbrIPC *pParentProcessReceiverIPC; //Todo put in parent process type
    FbrFramebuffer *pParentProcessFramebuffer;
    VkDescriptorSet parentFramebufferDescriptorSet;
    VkSemaphore parentTimelineSemaphore;
    uint64_t parentTimelineValue;
} FbrApp;

void fbrCreateApp(FbrApp **ppAllocApp, bool isChild, long long externalTextureTest);

void fbrCleanup(FbrApp *pApp);

#endif //FABRIC_APP_H
