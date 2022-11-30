#include "mxc_core.h"
#include "mxc_input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    printf("%s - starting up moxaic!\n", __FUNCTION__);

    MxcAppState *pState;
    pState = malloc(sizeof(MxcAppState));
    memset(pState, 0, sizeof(MxcAppState));
    pState->screenWidth = 800;
    pState->screenHeight = 600;
    pState->enableValidationLayers = true;
    pState->pTimeState = malloc(sizeof(MxcTimeState));
    memset(pState->pTimeState, 0, sizeof(MxcTimeState));

    mxcInitWindow(pState);
    mxcInitInput(pState);
    mxcInitVulkan(pState);
    mxcMainLoop(pState);
    mxcCleanup(pState);

    free(pState);

    return 0;
}