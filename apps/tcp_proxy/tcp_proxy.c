#include <sys/mman.h>

#include "iovents.h"

#define __APP__MAIN__
#include "tcp_proxy.h"

static void OnEstablish (void* appCtx, IoVentConn_t* iovConn) {
}

static void OnWriteNext (void* appCtx, IoVentConn_t* iovConn) {
}

static void OnWriteNextStatus (void* appCtx
                                , IoVentConn_t* iovConn
                                , char* writeBuffer
                                , int writeBuffOffset
                                , int writeDataLen 
                                , int bytesWritten
                                ) {
}

static void OnReadNext (void* appCtx, IoVentConn_t* iovConn) {

}

static void OnReadNextStatus (void* appCtx
                                , IoVentConn_t* iovConn
                                , char* readBuffer
                                , int readBuffOffset
                                , int readDataLen 
                                , int bytesReceived
                                ) {

}

static void OnCleanup (void* appCtx, IoVentConn_t* iovConn) {
}

static void OnStatus (void* appCtx, IoVentConn_t* iovConn) {
}

static TcpProxyCtx_t* CreateAppCtx (TcpProxyI_t* appI) {

    TcpProxyCtx_t* appCtx = CreateStruct0 (TcpProxyCtx_t);

    return appCtx;
}

void TcpProxyRun (TcpProxyI_t* appI) {

    TcpProxyCtx_t* appCtx = CreateAppCtx (appI);

    IoVentMethods_t* iovMethods = CreateStruct0 (IoVentMethods_t);

    iovMethods->OnEstablish = &OnEstablish;
    iovMethods->OnWriteNext = &OnWriteNext;
    iovMethods->OnWriteNextStatus = &OnWriteNextStatus;
    iovMethods->OnReadNext = &OnReadNext;
    iovMethods->OnReadNextStatus = &OnReadNextStatus;
    iovMethods->OnCleanup = &OnCleanup;
    iovMethods->OnStatus = &OnStatus;

    IoVentOptions_t* iovOptions = CreateStruct0 (IoVentOptions_t);

    iovOptions->maxActiveConnections = appI->maxActiveSessions;
    iovOptions->maxErrorConnections = appI->maxErrorSessions;
    
    IoVentCtx_t* iovCtx = CreateIoVentCtx (iovMethods, iovOptions);

    TcpProxyStats_t* appConnStats = &appI->gStats;

    TcpProxyServer_t* server 
        = &appI->serverArr[0];

    TcpProxyStats_t* groupConnStats = &server->cStats;

    InitServer(iovCtx
                , appCtx
                , &server->serverAddr
                , appConnStats
                , groupConnStats);

    while (1) {

        ProcessIoVent (iovCtx);

    }

    DumpErrConnections (iovCtx);

    DeleteIoVentCtx (iovCtx);
}

TcpProxyI_t* CreateTcpProxyInterface(int serverCount) {

    TcpProxyI_t* iFace 
        = (TcpProxyI_t*) mmap(NULL
            , sizeof (TcpProxyI_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    iFace->serverCount = serverCount;
    iFace->serverArr 
        = (TcpProxyServer_t*) mmap(NULL
            , sizeof (TcpProxyServer_t) * iFace->serverCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    return iFace;
}

void DeleteTcpProxyInterface (TcpProxyI_t* iFace){
    //todo
}

void DumpTcpProxyStats(TcpProxyStats_t* appConnStats) {
    
    char statsString[120];

    sprintf (statsString, 
                        "%" PRIu64 "\n" 
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "\n"
        , GetConnStats(appConnStats, tcpConnInit)
        , GetConnStats(appConnStats, tcpConnInitSuccess)
        , GetConnStats(appConnStats, tcpConnInitFail)
        , GetConnStats(appConnStats, tcpConnInitFailImmediateOther)
        , GetConnStats(appConnStats, tcpConnInitFailImmediateEaddrNotAvail)
        , GetConnStats(appConnStats, tcpPollRegUnregFail)
        , GetConnStats(appConnStats, dummyCount)
        );

    puts (statsString);
}



