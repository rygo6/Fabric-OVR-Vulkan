#include "fbr_ipc.h"
#include "fbr_log.h"

#define BUF_SIZE 256
char szName[] = "FbrIPC";
char szMsg[] = "Message from first process.";

int fbrCreateProducerIPC(FbrIPC **ppAllocIPC) {
    HANDLE hMapFile;
    LPVOID pBuf;

    hMapFile = CreateFileMapping(
            INVALID_HANDLE_VALUE,    // use paging file
            NULL,                    // default security
            PAGE_READWRITE,          // read/write access
            0,                       // maximum object size (high-order DWORD)
            BUF_SIZE,                // maximum object size (low-order DWORD)
            szName);                 // name of mapping object

    if (hMapFile == NULL) {
        FBR_LOG_DEBUG("Could not create file mapping object (%lu)", GetLastError());
        return 1;
    }
    pBuf = MapViewOfFile(hMapFile,   // handle to map object
                                  FILE_MAP_ALL_ACCESS, // read/write permission
                                  0,
                                  0,
                                  BUF_SIZE);
    memset(pBuf, 0, BUF_SIZE);

    if (pBuf == NULL) {
        printf(TEXT("Could not map view of file (%lu).\n"), GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

    *ppAllocIPC = calloc(1, sizeof(FbrIPC));
    FbrIPC *pIPC = *ppAllocIPC;

//    FBR_LOG_DEBUG("sizes", sizeof(szMsg), sizeof(int));

//    memcpy((void*) pBuf, szMsg, sizeof(szMsg));

//    int test = 12345789;
//    memcpy(pBuf, &test, sizeof(int));

    pIPC->hMapFile = hMapFile;
    pIPC->pBuf = pBuf;

    return 0;
}

int fbrCreateReceiverIPC(FbrIPC **ppAllocIPC) {
    HANDLE hMapFile;
    LPVOID pBuf;

    hMapFile = OpenFileMapping(
            FILE_MAP_ALL_ACCESS,   // read/write access
            FALSE,                 // do not inherit the name
            szName);               // name of mapping object

    if (hMapFile == NULL) {
        FBR_LOG_DEBUG("Could not create file mapping object", GetLastError());
        return 1;
    }

    pBuf = MapViewOfFile(hMapFile, // handle to map object
                                  FILE_MAP_ALL_ACCESS,  // read/write permission
                                  0,
                                  0,
                                  BUF_SIZE);

    if (pBuf == NULL) {
        FBR_LOG_DEBUG("Could not map view of file", GetLastError());
        CloseHandle(hMapFile);
        return 1;
    }

//    while(*(int*)pBuf == 0) {
//        printf("Wait Message: %d\n", pBuf);
//    }
//
//    int test;
//    memcpy(&test, pBuf, sizeof(int));
//    printf("Message: %d\n", test);

    *ppAllocIPC = calloc(1, sizeof(FbrIPC));
    FbrIPC *pIPC = *ppAllocIPC;

    pIPC->hMapFile = hMapFile;
    pIPC->pBuf = pBuf;

    return 0;
}

int fbrDestroyIPC(FbrIPC *pIPC) {
    UnmapViewOfFile(pIPC->pBuf);
    CloseHandle(pIPC->hMapFile);
    free(pIPC);

    return 0;
}