
#include <sys/mman.h>

#include "iovents.h"
#include "tls_sample_client.h"

static SSL_CTX* GSslContext = NULL;

static void OnEstablish (struct IoVentConn* iovConn) {
    // iovConn->connData = SSL_new(GSslContext);
    iovConn->bytesSent = 0;
    // SslClientInit (iovConn, (SSL*)iovConn->connData);     
}

static void OnWriteNext (struct IoVentConn* iovConn) {

    TlsSampleClientCtx_t* appCtx 
        = (TlsSampleClientCtx_t*) iovConn->appCtx;

    if (iovConn->bytesSent < appCtx->csDataLen ) {

       WriteNextData (iovConn
                    , appCtx->sendBuffer
                    , 0
                    , appCtx->csDataLen - iovConn->bytesSent); 
    }
}

static void OnWriteStatus (struct IoVentConn* iovConn
                            , int bytesWritten
                            ) {

    TlsSampleClientCtx_t* appCtx 
            = (TlsSampleClientCtx_t*) iovConn->appCtx;

    iovConn->bytesSent += bytesWritten;

    if (iovConn->bytesSent == appCtx->csDataLen) {
        //change this to iovent API
        SetCS1(iovConn, STATE_NO_MORE_WRITE_DATA 
                        | STATE_SSL_TO_SEND_SHUTDOWN
                        | STATE_TCP_TO_SEND_FIN);
    }          
}

static void OnReadNext (struct IoVentConn* iovConn) {

    TlsSampleClientCtx_t* appCtx 
            = (TlsSampleClientCtx_t*) iovConn->appCtx;

    ReadNextData (iovConn
                    , appCtx->receiveBuffer
                    , 0
                    , appCtx->scDataLen);
}

static void OnReadStatus (struct IoVentConn* iovConn
                        , int bytesReceived
                        ) {

}

static void OnCleanup (struct IoVentConn* iovConn) {
    // SSL_free((SSL*)iovConn->connData);
}

static void OnStatus (struct IoVentConn* iovConn) {

    //todo for more advavanced control
    switch (iovConn->statusId) {
        default:
            break;
    }

}

static TlsSampleClientCtx_t* CreateAppCtx (TlsSampleClient_t* appI) {

    TlsSampleClientCtx_t* appCtx = CreateStruct0 (TlsSampleClientCtx_t);

    appCtx->sendBuffer = TdMalloc (appI->csDataLen);
    appCtx->csDataLen = appI->csDataLen;

    appCtx->receiveBuffer = TdMalloc (appI->scDataLen);
    appCtx->scDataLen = appI->scDataLen;

    return appCtx;
}

void TlsSampleClientRun (TlsSampleClient_t* appI) {

    appI->isRunning = 1;
    TlsSampleClientCtx_t* appCtx = CreateAppCtx (appI);

    SSL_load_error_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_ssl_algorithms();
    SSL_library_init();

    GSslContext = SSL_CTX_new(SSLv23_client_method());

    SSL_CTX_set_verify(GSslContext
                            , SSL_VERIFY_NONE, 0);

    SSL_CTX_set_options(GSslContext
                            , SSL_OP_NO_SSLv2 
                            | SSL_OP_NO_SSLv3 
                            | SSL_OP_NO_COMPRESSION);
                            
    SSL_CTX_set_mode(GSslContext
                            , SSL_MODE_ENABLE_PARTIAL_WRITE);


    IoVentMethods_t* iovMethods = CreateStruct0 (IoVentMethods_t);
    iovMethods->OnEstablish = &OnEstablish;
    iovMethods->OnWriteNext = &OnWriteNext;
    iovMethods->OnWriteStatus = &OnWriteStatus;
    iovMethods->OnReadNext = &OnReadNext;
    iovMethods->OnReadStatus = &OnReadStatus;
    iovMethods->OnCleanup = &OnCleanup;
    iovMethods->OnStatus = &OnStatus;

    IoVentOptions_t* iovOptions = CreateStruct0 (IoVentOptions_t);
    iovOptions->maxActiveConnections = appI->maxActiveSessions;
    iovOptions->maxErrorConnections = appI->maxErrorSessions;
    iovOptions->maxEvents = appI->maxEvents; 
    
    IoVentCtx_t* iovCtx = CreateIoVentCtx (iovMethods, iovOptions);

    TimerWheel_t* timerWheel = CreateTimerWheel();
    double lastConnectionInitTime = TimeElapsedTimerWheel(timerWheel);
    int activeConnections = 0;
    TlsSampleClientStats_t* appConnStats = &appI->appConnStats;
    // TlsSampleClientAppStats_t* appStats = &appI->appStats;

    while (1) {

        activeConnections = ProcessIoVent (iovCtx);

        if ( (GetConnStats(appConnStats, tcpConnInitFail) 
                    >= appI->maxErrorSessions) 
                || (GetConnStats(appConnStats, tcpConnInit) 
                    == appI->maxSessions 
                    && activeConnections == 0) ) {
            break;
        }

        if (GetConnStats(appConnStats, tcpConnInit) < appI->maxSessions) {

            int newConnectionInits 
                = ((TimeElapsedTimerWheel(timerWheel)
                        - lastConnectionInitTime)
                    * appI->connectionPerSec);
                
            while ( (newConnectionInits > 0) 
                    && (GetConnStats(appConnStats, tcpConnInit) 
                        < appI->maxSessions) ) {

                newConnectionInits--;

                lastConnectionInitTime = TimeElapsedTimerWheel(timerWheel);

                TlsSampleClientGroup_t* csGroup 
                    = &appI->csGroupArr[appI->nextCsGroupIndex];

                SockAddr_t* localAddress 
                    = &(csGroup->clientAddrArr[csGroup->nextClientAddrIndex]);

                SockAddr_t* remoteAddress 
                    = &(csGroup->serverAddr);

                TlsSampleClientStats_t* groupConnStats 
                    = &csGroup->cStats;

                LocalPortPool_t* localPortPool 
                    = &csGroup->LocalPortPoolArr[csGroup->nextClientAddrIndex];

                appI->nextCsGroupIndex += 1;
                if (appI->nextCsGroupIndex == appI->csGroupCount){
                    appI->nextCsGroupIndex = 0;
                }
                
                csGroup->nextClientAddrIndex += 1;
                if (csGroup->nextClientAddrIndex == csGroup->clientAddrCount) {
                    csGroup->nextClientAddrIndex = 0;
                }

                NewConnection (iovCtx
                                , csGroup
                                , appCtx
                                , localAddress
                                , localPortPool
                                , remoteAddress
                                , appConnStats
                                , groupConnStats);
            }
        }
    }

    DumpErrConnections (iovCtx);

    DeleteIoVentCtx (iovCtx);
    DeleteTimerWheel(timerWheel);


    appI->isRunning = 0;
}

TlsSampleClient_t* CreateTlsSampleClientInterface(int csGroupCount
                                            , int* clientAddrCounts) {

    TlsSampleClient_t* iFace 
        = (TlsSampleClient_t*) mmap(NULL
            , sizeof (TlsSampleClient_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    iFace->csGroupCount = csGroupCount;
    iFace->csGroupArr 
        = (TlsSampleClientGroup_t*) mmap(NULL
            , sizeof (TlsSampleClientGroup_t) * iFace->csGroupCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);
    iFace->nextCsGroupIndex = 0;
    for (int gIndex = 0; gIndex < iFace->csGroupCount; gIndex++) {
        TlsSampleClientGroup_t* csGroup = &iFace->csGroupArr[gIndex];
        csGroup->clientAddrCount = clientAddrCounts[gIndex];
        csGroup->nextClientAddrIndex = 0;
        csGroup->clientAddrArr
            = (SockAddr_t*) mmap(NULL
                , sizeof (SockAddr_t) * csGroup->clientAddrCount
                , PROT_READ | PROT_WRITE
                , MAP_SHARED | MAP_ANONYMOUS
                , -1
                , 0);
        csGroup->LocalPortPoolArr 
            = (LocalPortPool_t*) mmap(NULL
                , sizeof (LocalPortPool_t) * csGroup->clientAddrCount
                , PROT_READ | PROT_WRITE
                , MAP_SHARED | MAP_ANONYMOUS
                , -1
                , 0);
        for (int cIndex = 0
                ; cIndex < csGroup->clientAddrCount
                ; cIndex++) {
            InitPortBindQ(&csGroup->LocalPortPoolArr[cIndex]);
        }
    }

    return iFace;
}

void DeleteTlsSampleClientInterface (TlsSampleClient_t* iFace){
    //todo
}

void DumpTlsSampleClientStats(TlsSampleClientStats_t* appConnStats) {
    
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



