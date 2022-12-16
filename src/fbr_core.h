#ifndef FABRIC_CORE_H
#define FABRIC_CORE_H

#include "fbr_app.h"

void fbrCreateApp(FbrApp **ppAllocApp);

void fbrMainLoop(FbrApp *pApp);

void fbrCleanup(FbrApp *pApp);

#endif //FABRIC_CORE_H
