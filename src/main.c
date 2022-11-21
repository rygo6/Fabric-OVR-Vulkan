#include "moxaic_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    printf( "%s - starting up moxaic!\n", __FUNCTION__ );

    MxcAppState *pState;
    pState = malloc(sizeof(*pState));
    memset(pState, 0, sizeof( *pState ) );
    pState->screenWidth = 800;
    pState->screenHeight = 600;
    pState->enableValidationLayers = true;

    mxcInitWindow(pState);
    mxcInitVulkan(pState);
    mxcMainLoop(pState);
    mxcCleanup(pState);

    free(pState);

    return 0;
}