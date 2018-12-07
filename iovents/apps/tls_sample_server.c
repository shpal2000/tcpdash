
#include <sys/mman.h>

#include "iovents.h"
#include "tls_sample_server.h"

static SSL_CTX* GSslContext = NULL;

static void OnEstablish (void* appCtx, IoVentConn_t* iovConn) {
    // iovConn->connData = SSL_new(GSslContext);
    iovConn->bytesSent = 0;
    // SslServerInit (iovConn, (SSL*)iovConn->connData);     
}

static void OnWriteNext (void* appCtx, IoVentConn_t* iovConn) {

    // TlsSampleServerCtx_t* appData 
    //         = (TlsSampleServerCtx_t*) appCtx;

    // if (iovConn->bytesSent < appData->csDataLen ) {

    //    WriteNextData (iovConn
    //                 , appData->sendBuffer
    //                 , 0
    //                 , appData->csDataLen - iovConn->bytesSent); 
    // }

    if ( IsSetCS1 (iovConn, STATE_TCP_REMOTE_CLOSED) ) {
        //change this to iovent API
        SetCS1(iovConn, STATE_NO_MORE_WRITE_DATA 
                        | STATE_SSL_TO_SEND_SHUTDOWN
                        | STATE_TCP_TO_SEND_FIN);
    } 
}

static void OnWriteNextStatus (void* appCtx
                                , IoVentConn_t* iovConn
                                , char* writeBuffer
                                , int writeBuffOffset
                                , int writeDataLen 
                                , int bytesWritten
                                ) {

    // TlsSampleServerCtx_t* appData 
    //         = (TlsSampleServerCtx_t*) appCtx;
    // iovConn->bytesSent += bytesWritten;

    // if (iovConn->bytesSent == appData->csDataLen) {
    //     //change this to iovent API
    //     SetCS1(iovConn, STATE_NO_MORE_WRITE_DATA 
    //                     | STATE_SSL_TO_SEND_SHUTDOWN
    //                     | STATE_TCP_TO_SEND_FIN);
    // }          
}

static void OnReadNext (void* appCtx, IoVentConn_t* iovConn) {

    TlsSampleServerCtx_t* appData 
            = (TlsSampleServerCtx_t*) appCtx;

    ReadNextData (iovConn
                    , appData->receiveBuffer
                    , 0
                    , appData->scDataLen);
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
    // SSL_free((SSL*)iovConn->connData);
}

static void OnStatus (void* appCtx, IoVentConn_t* iovConn) {

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
    iovMethods->OnWriteNextStatus = &OnWriteNextStatus;
    iovMethods->OnReadNext = &OnReadNext;
    iovMethods->OnReadNextStatus = &OnReadNextStatus;
    iovMethods->OnCleanup = &OnCleanup;
    iovMethods->OnStatus = &OnStatus;

    IoVentOptions_t* iovOptions = CreateStruct0 (IoVentOptions_t);
    iovOptions->maxActiveConnections = appI->maxActiveSessions;
    iovOptions->maxErrorConnections = appI->maxErrorSessions;
    iovOptions->maxEvents = appI->maxEvents; 
    
    IoVentCtx_t* iovCtx = CreateIoVentCtx (iovMethods, iovOptions);
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



