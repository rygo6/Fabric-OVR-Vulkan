#include "fbr_app.h"
#include "fbr_core.h"
#include "fbr_log.h"

#include <stdlib.h>
#include <string.h>
#include <windows.h>
//#include <processthreadsapi.h>

// https://forums.developer.nvidia.com/t/windows-vk-ext-global-priority/196010/2
BOOL SetPrivilege(
        HANDLE hToken,          // access token handle
        LPCTSTR lpszPrivilege,  // name of privilege to enable/disable
        BOOL bEnablePrivilege   // to enable or disable privilege
)
{
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if ( !LookupPrivilegeValue(
            NULL,            // lookup privilege on local system
            lpszPrivilege,   // privilege to lookup
            &luid ) )        // receives LUID of privilege
    {
        //printf("LookupPrivilegeValue error: %u\n", GetLastError() );
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    // Enable the privilege or disable all privileges.

    if ( !AdjustTokenPrivileges(
            hToken,
            FALSE,
            &tp,
            sizeof(TOKEN_PRIVILEGES),
            (PTOKEN_PRIVILEGES) NULL,
            (PDWORD) NULL) )
    {
        //printf("AdjustTokenPrivileges error: %u\n", GetLastError() );
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)

    {
        //printf("The token does not have the specified privilege. \n");
        return FALSE;
    }

    return TRUE;
}
bool setupRTPrivileges(){
    HANDLE process = GetCurrentProcess();
    HANDLE token;
    if(!OpenProcessToken(process, TOKEN_ADJUST_PRIVILEGES, &token)){
        return false;
    }

    BOOL ret = SetPrivilege(token, SE_INC_BASE_PRIORITY_NAME, TRUE);
    CloseHandle(process);
    CloseHandle(token);
    return ret;
}

int testreturn() {
    10;
}

int main(int argc, char *argv[]) {

    int val = testreturn();
    FBR_LOG_DEBUG("starting!", val);


//    setupRTPrivileges();

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