#include "fbr_app.h"
#include "fbr_core.h"
#include "fbr_log.h"

#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    FBR_LOG_DEBUG("starting!");

    bool isChild = false;
    for (int i = 0; i < argc; ++i) {
        FBR_LOG_DEBUG("arg", argv[i]);
        if (strcmp(argv[i], "-child") == 0){
            isChild = true;
            FBR_LOG_DEBUG("Is Child Process", isChild);
        }
    }

    FbrApp *pApp;
    fbrCreateApp(&pApp, isChild);

    fbrMainLoop(pApp);

    fbrCleanup(pApp);

    free(pApp);

    return 0;
}