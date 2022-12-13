#include "fbr_core.h"
#include "fbr_input.h"

#include <stdlib.h>

int main(int argc, char *argv[]) {
    FbrApp *pApp;
    pApp = calloc(1, sizeof(FbrApp));
    pApp->screenWidth = 800;
    pApp->screenHeight = 600;
    pApp->enableValidationLayers = true;
    pApp->pTime = calloc(1, sizeof(FbrTime));

    fbrInitWindow(pApp);
    fbrInitInput(pApp);
    fbrInitVulkan(pApp);
    fbrMainLoop(pApp);
    fbrCleanup(pApp);

    free(pApp);

    return 0;
}