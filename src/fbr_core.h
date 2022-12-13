#ifndef FABRIC_CORE_H
#define FABRIC_CORE_H

#include "fbr_camera.h"
#include "fbr_app.h"

#include "cglm/cglm.h"

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>
#include <stdbool.h>

void fbrInitWindow(FbrApp *pState);

void fbrInitVulkan(FbrApp *pState);

void fbrMainLoop(FbrApp *pState);

void fbrCleanup(FbrApp *pApp);

#endif //FABRIC_CORE_H
