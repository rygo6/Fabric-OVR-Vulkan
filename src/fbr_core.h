#ifndef FABRIC_CORE_H
#define FABRIC_CORE_H

#include "fbr_camera.h"
#include "fbr_app.h"

#include "cglm/cglm.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>

void fbrInitWindow(FbrAppState* pState);
void fbrInitVulkan(FbrAppState* pState);
void fbrMainLoop(FbrAppState* pState);
void fbrCleanup(FbrAppState* pAppState);

#endif //FABRIC_CORE_H
