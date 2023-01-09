#include "fbr_process.h"
#include <stdio.h>
#include <tchar.h>

void fbrCreateProcess(FbrProcess **ppAllocProcess) {
    *ppAllocProcess = calloc(1, sizeof(FbrProcess));
    FbrProcess *pProcess = *ppAllocProcess;

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcess(NULL,   // No module name (use command line)
                       "fabric.exe -child",        // Command line
                       NULL,           // Process handle not inheritable
                       NULL,           // Thread handle not inheritable
                       FALSE,          // Set handle inheritance to FALSE
                       0,              // No creation flags
                       NULL,           // Use parent's environment block
                       NULL,           // Use parent's starting directory
                       &si,            // Pointer to STARTUPINFO structure
                       &pi)           // Pointer to PROCESS_INFORMATION structure
            ) {
        printf("CreateProcess failed (%lu).\n", GetLastError());
    }

    pProcess->pi = pi;
    pProcess->si = si;
}

void fbrDestroyProcess(FbrProcess *pProcess) {
    // Wait until child process exits.
    WaitForSingleObject(pProcess->pi.hProcess, INFINITE);

    // Close process and thread handles.
    CloseHandle(pProcess->pi.hProcess);
    CloseHandle(pProcess->pi.hThread);
}
