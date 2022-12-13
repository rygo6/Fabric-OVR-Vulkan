#include "fbr_core.h"
#include "fbr_input.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    printf("%s - starting up !\n", __FUNCTION__);

    FbrApp *pState;
    pState = calloc(1, sizeof(FbrApp));
    pState->screenWidth = 800;
    pState->screenHeight = 600;
    pState->enableValidationLayers = true;
    pState->pTime = calloc(1, sizeof(FbrTime));

    fbrInitWindow(pState);
    fbrInitInput(pState);
    fbrInitVulkan(pState);
    fbrMainLoop(pState);
    fbrCleanup(pState);

    free(pState);

    return 0;
}