#include "fbr_app.h"
#include "fbr_core.h"
#include "fbr_log.h"

#include <stdlib.h>
#include <string.h>
#include <windows.h>

int main(int argc, char *argv[]) {
    FBR_LOG_DEBUG("starting!");

    bool isChild = false;
    long long externalTextureTest;
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "-child") == 0) {
            isChild = true;
        } else if (strcmp(argv[i], "-pTestTexture") == 0) {
            i++;
            externalTextureTest = strtoll(argv[i], NULL, 10);
        }
    }

    if (isChild) {
        Sleep(1000);
        FBR_LOG_DEBUG("Is Child Process", isChild);
    }

    FbrApp *pApp;
    fbrCreateApp(&pApp, isChild, externalTextureTest);

    fbrMainLoop(pApp);

    fbrCleanup(pApp);

    free(pApp);

    return 0;
}