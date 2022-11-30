#ifndef MOXAIC_CORE_H
#define MOXAIC_CORE_H

#include "mxc_camera.h"
#include "mxc_app.h"

#include "cglm/cglm.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <stdbool.h>

void mxcInitWindow(MxcAppState* pState);
void mxcInitVulkan(MxcAppState* pState);
void mxcMainLoop(MxcAppState* pState);
void mxcCleanup(MxcAppState* pAppState);

#endif //MOXAIC_CORE_H
