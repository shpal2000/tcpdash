
#include <sys/mman.h>

#include "iovents.h"
#include "tls_sample_server.h"

static SSL_CTX* GSslContext = NULL;

static void OnEstablish (struct IoVentConn* iovConn) {
    // iovConn->connData = SSL_new(GSslContext);
    iovConn->bytesSent = 0;
    // SslServerInit (iovConn, (SSL*)iovConn->connData);     
}

static void OnWriteNext (struct IoVentConn* iovConn) {

    // TlsSampleServerCtx_t* appCtx 
    //         = (TlsSampleServerCtx_t*) iovConn->cInfo.appCtx;

    // if (iovConn->bytesSent < appCtx->csDataLen ) {

    //    WriteNextData (iovConn
    //                 , appCtx->sendBuffer
    //                 , 0
    //                 , appCtx->csDataLen - iovConn->bytesSent
    //                 , 1); 
    // }

    if ( IsSetCS1 (iovConn, STATE_TCP_REMOTE_CLOSED) ) {
        //change this to iovent API
        SetCS1(iovConn, STATE_NO_MORE_WRITE_DATA 
                        | STATE_SSL_TO_SEND_SHUTDOWN
                        | STATE_TCP_TO_SEND_FIN);
    } 
}

static void OnWriteStatus (struct IoVentConn* iovConn 
                        , int bytesWritten
                        ) {

    // TlsSampleServerCtx_t* appCtx 
    //         = (TlsSampleServerCtx_t*) iovConn->cInfo.appCtx;
    // iovConn->bytesSent += bytesWritten;

    // if (iovConn->bytesSent == appCtx->csDataLen) {
    //     //change this to iovent API
    //     SetCS1(iovConn, STATE_NO_MORE_WRITE_DATA 
    //                     | STATE_SSL_TO_SEND_SHUTDOWN
    //                     | STATE_TCP_TO_SEND_FIN);
    // }          
}

static void OnReadNext (struct IoVentConn* iovConn) {

    TlsSampleServerCtx_t* appCtx 
            = (TlsSampleServerCtx_t*) iovConn->cInfo.appCtx;

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

static TlsSampleServerCtx_t* CreateAppCtx (TlsSampleServer_t* appI) {

    TlsSampleServerCtx_t* appCtx = CreateStruct0 (TlsSampleServerCtx_t);

    appCtx->sendBuffer = TdMalloc (appI->csDataLen);
    appCtx->csDataLen = appI->csDataLen;

    appCtx->receiveBuffer = TdMalloc (appI->scDataLen);
    appCtx->scDataLen = appI->scDataLen;

    return appCtx;
}

void TlsSampleServerRun (TlsSampleServer_t* appI) {

    appI->isRunning = 1;
    TlsSampleServerCtx_t* appCtx = CreateAppCtx (appI);

    SSL_load_error_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_ssl_algorithms();
    SSL_library_init();

    GSslContext = SSL_CTX_new(SSLv23_server_method());

    SSL_CTX_set_verify(GSslContext
                            , SSL_VERIFY_NONE, 0);

    SSL_CTX_set_options(GSslContext
                            , SSL_OP_NO_SSLv2 
                            | SSL_OP_NO_SSLv3 
                            | SSL_OP_NO_COMPRESSION);
                            
    SSL_CTX_set_mode(GSslContext
                            , SSL_MODE_ENABLE_PARTIAL_WRITE);


    SSL_CTX_use_certificate_file(GSslContext
            , "/root/autssl/certdepo/ca1/usrcerts/rsa2048_1_sha256.cert"
            , SSL_FILETYPE_PEM);

    SSL_CTX_use_PrivateKey_file(GSslContext
            , "/root/autssl/certdepo/ca1/usrcerts/rsa2048_1.key"
            , SSL_FILETYPE_PEM);

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
    
    IoVentCtx_t* iovCtx 
        = CreateIoVentCtx (iovMethods, iovOptions, appCtx);

    TlsSampleServerStats_t* appConnStats = &appI->appConnStats;
    // TlsSampleServerAppStats_t* appStats = &appI->appStats;


    for (int i = 0; i < appI->csGroupCount; i++) {

        TlsSampleServerGroup_t* csGroup 
            = &appI->csGroupArr[i];

        TlsSampleServerStats_t* groupConnStats 
            = &csGroup->cStats;

        SockAddr_t* localAddress 
             = &(csGroup->serverAddr);

        InitServer(iovCtx
                    , csGroup
                    , appCtx
                    , localAddress
                    , appConnStats
                    , groupConnStats);
    }


    while (1) {

        ProcessIoVent (iovCtx);

    }

    DumpErrConnections (iovCtx);

    DeleteIoVentCtx (iovCtx);


    appI->isRunning = 0;
}

TlsSampleServer_t* CreateTlsSampleServerInterface(int csGroupCount) {

    TlsSampleServer_t* iFace 
        = (TlsSampleServer_t*) mmap(NULL
            , sizeof (TlsSampleServer_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    iFace->csGroupCount = csGroupCount;
    iFace->csGroupArr 
        = (TlsSampleServerGroup_t*) mmap(NULL
            , sizeof (TlsSampleServerGroup_t) * iFace->csGroupCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    return iFace;
}

void DeleteTlsSampleServerInterface (TlsSampleServer_t* iFace){
    //todo
}

void DumpTlsSampleServerStats(TlsSampleServerStats_t* appConnStats) {
    
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
        , GetConnStats(appConnStats, tcpAcceptSuccess)
        , GetConnStats(appConnStats, tcpConnInitFail)
        , GetConnStats(appConnStats, tcpConnInitFailImmediateOther)
        , GetConnStats(appConnStats, tcpConnInitFailImmediateEaddrNotAvail)
        , GetConnStats(appConnStats, tcpPollRegUnregFail)
        , GetConnStats(appConnStats, dummyCount)
        );

    puts (statsString);
}



