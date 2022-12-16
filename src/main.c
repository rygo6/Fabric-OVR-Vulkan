#include "fbr_app.h"
#include "fbr_core.h"
#include "fbr_log.h"

#include <stdlib.h>

int main(int argc, char *argv[]) {
    FBR_LOG_DEBUG("starting!");

    FbrApp *pApp;
    fbrCreateApp(&pApp);

    fbrMainLoop(pApp);

    fbrCleanup(pApp);

    free(pApp);

    return 0;
}