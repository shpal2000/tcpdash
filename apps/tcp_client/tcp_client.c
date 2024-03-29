#include "iovents.h"
#include "msg_io.h"
#include "nadmin.h"

#include "tcp_client.h"

static void InitConn (TcpClientConn_t * tcpConn) {

    tcpConn->iovConn = NULL;
    tcpConn->bytesRead = 0;
    tcpConn->bytesWritten = 0;
    tcpConn->writeBuffOffset = 0;
}

static TcpClientSession_t* GetSession (TcpClientCtx_t* appCtx) {

    TcpClientSession_t* newSess =         
        GetFromPool (appCtx->freeSessionPool);
    
    if (newSess) {

        AddToPool (&appCtx->activeSessionPool, newSess);

        newSess->appCtx = appCtx;

        InitConn (&newSess->tcpConn);
    }

    return newSess;
}

static void FreeSession (TcpClientSession_t* newSess) {

    RemoveFromPool (&newSess->appCtx->activeSessionPool, newSess);

    AddToPool (newSess->appCtx->freeSessionPool, newSess);
}

static void OnEstablish (struct IoVentConn* iovConn) { 

    TcpClientSession_t* newSess 
        = (TcpClientSession_t*) iovConn->cInfo.sessionData;
 
    if ( IsConnErr (iovConn) ) {
        FreeSession (newSess);
    } else {
        setsockopt(iovConn->socketFd, SOL_SOCKET, SO_KEEPALIVE, &(int){ 1 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPCNT, &(int){ 3 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPIDLE, &(int){ 5 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_KEEPINTVL, &(int){ 1 }, sizeof(int));
        setsockopt(iovConn->socketFd, SOL_TCP, TCP_USER_TIMEOUT, &(int){ 15000 }, sizeof(int));
    }
}

static void OnReadNext (struct IoVentConn* iovConn) {

    TcpClientCtx_t* appCtx 
        = (TcpClientCtx_t*) iovConn->cInfo.appCtx;

    ReadNextData (iovConn
                , appCtx->commonReadBuff
                , 0
                , COMMON_READBUFF_MAXLEN
                , 1);
}

static void OnReadStatus (struct IoVentConn* iovConn
                                    , int bytesRead) {

    TcpClientSession_t* newSess 
        = (TcpClientSession_t*) iovConn->cInfo.sessionData;

    
    if (bytesRead > 0) {

        newSess->tcpConn.bytesRead += bytesRead;
    
    } else {

        int closeErr = bytesRead;

        if (closeErr != ON_CLOSE_ERROR_NONE) {
            AbortConnection (iovConn);    
        }
    }
}

static void OnWriteNext (struct IoVentConn* iovConn) {

    TcpClientCtx_t* appCtx 
        = (TcpClientCtx_t*) iovConn->cInfo.appCtx;

    TcpClientSession_t* newSess 
        = (TcpClientSession_t*) iovConn->cInfo.sessionData;

    TcpClientGroup_t* groupCtx 
        = (TcpClientGroup_t*) iovConn->cInfo.groupCtx;
  

    if (newSess->tcpConn.bytesWritten < groupCtx->csDataLen) {

        int nextChunkLen = 0;

        if ( (groupCtx->csDataLen - newSess->tcpConn.bytesWritten) 
                                > COMMON_WRITEBUFF_MAXLEN ) {

            nextChunkLen = COMMON_WRITEBUFF_MAXLEN;

        } else {

            nextChunkLen 
                = (int) (groupCtx->csDataLen - newSess->tcpConn.bytesWritten);
        }

        WriteNextData (iovConn
                        , appCtx->commonWriteBuff
                        , 0
                        , nextChunkLen
                        , 0);
    }
}

static void OnWriteStatus (struct IoVentConn* iovConn
                                    , int bytesWritten) {

    TcpClientSession_t* newSess 
        = (TcpClientSession_t*) iovConn->cInfo.sessionData;

    TcpClientGroup_t* groupCtx 
        = (TcpClientGroup_t*) iovConn->cInfo.groupCtx;


    if (bytesWritten > 0) {

        newSess->tcpConn.bytesWritten += bytesWritten;

        if (newSess->tcpConn.bytesWritten == groupCtx->csDataLen) {
            WriteClose (iovConn);
        }
    } else {
        AbortConnection (iovConn);
    }
}

static void OnCleanup (struct IoVentConn* iovConn) {

    TcpClientSession_t* newSess 
        = (TcpClientSession_t*) iovConn->cInfo.sessionData;

    FreeSession (newSess);
}

static void OnStatus (struct IoVentConn* iovConn) {
}

static int OnContinue (void* appData) {
    
    TcpClientCtx_t* appCtx = (TcpClientCtx_t*) appData;

    TcpClientI_t* appI = appCtx->appI;

    //exceeded error connections
    uint64_t tcpConnInitFailCount 
        = GetConnStats(&appI->gStats, tcpConnInitFail);

    if (tcpConnInitFailCount >= appI->maxErrSessions) {
        return EmAppExit; 
    }

    //achived desired total connections
    uint64_t tcpConnInitCount 
        = GetConnStats(&appI->gStats, tcpConnInit);

    if (tcpConnInitCount == appI->maxSessions) {
        return EmAppContinueZeroActive; 
    }

    //continue to run
    return EmAppContinue;
}

static void MsgIoOnOpen (MsgIoChannelId_t mioChannelId) {

    // TcpClientCtx_t* appCtx = (TcpClientCtx_t*) MsgIoGetCtx (mioChannelId);

    // appCtx->nAdminChannelState = N_ADMIN_CHANNEL_STATE_GET_CONFIG;

    // MsgIoSend (mioChannelId
    //             , N_ADMIN_CMD_GET_TEST_CONFIG
    //             , strlen(N_ADMIN_CMD_GET_TEST_CONFIG));

    TcpClientCtx_t* appCtx = (TcpClientCtx_t*) MsgIoGetCtx (mioChannelId);


    char* srcIpGroup1[] = { "12.20.50.2"
                , "12.20.50.3"
                , "12.20.50.4"
                , "12.20.50.5"
                , "12.20.50.6"
                , "12.20.50.7"
                , "12.20.50.8"
                , "12.20.50.9"
                , "12.20.50.10"
                , "12.20.50.11"
                , "12.20.50.12"
                , "12.20.50.13"
                , "12.20.50.14"
                , "12.20.50.15"
                , "12.20.50.16"
                , "12.20.50.17"
                , "12.20.50.18"
                , "12.20.50.19"
                , "12.20.50.20"
                , "12.20.50.21"
                , "12.20.50.22"
                , "12.20.50.23"
                , "12.20.50.24"
                , "12.20.50.25"
                , "12.20.50.26"
                , "12.20.50.27"
                , "12.20.50.28"
                , "12.20.50.29"
                , "12.20.50.30"
                , "12.20.50.31"};

    char** srcIpGroups[1];
    srcIpGroups[0] = srcIpGroup1;

    char* dstIpGroups[1] = { "12.20.60.2"};
    int dstPort = 443;

    int csGroupClientAddrCountArr[1] = {30};

    int csGroupCount = 1;

    TcpClientI_t* appI = CreateStruct0 (TcpClientI_t);

    appI->csGroupCount = csGroupCount;

    appI->csGroupArr = CreateArray (TcpClientGroup_t, appI->csGroupCount);

    appI->nextCsGroupIndex = 0;

    for (int gIndex = 0; gIndex < appI->csGroupCount; gIndex++) {
        
        TcpClientGroup_t* csGroup = &appI->csGroupArr[gIndex];
        
        csGroup->clientAddrCount = csGroupClientAddrCountArr[gIndex];
        
        csGroup->nextClientAddrIndex = 0;

        csGroup->clientAddrArr 
            = CreateArray (SockAddr_t, csGroup->clientAddrCount);
        
        csGroup->LocalPortPoolArr 
            = CreateArray (LocalPortPool_t, csGroup->clientAddrCount);
        
        for (int cIndex = 0
                ; cIndex < csGroup->clientAddrCount
                ; cIndex++) {
        
            struct sockaddr_in* localAddr 
                = &(csGroup->clientAddrArr[cIndex].inAddr);

            memset(localAddr, 0, sizeof(SockAddr_t));
            localAddr->sin_family = AF_INET;
            inet_pton(AF_INET
                        , srcIpGroups[gIndex][cIndex]
                        , &(localAddr->sin_addr));

            LocalPortPool_t* portQ = &csGroup->LocalPortPoolArr[cIndex];
            InitPortBindQ(portQ);
            for (int srcPort = 5000; srcPort <= 65000; srcPort++) {
                SetPortToPool(portQ, htons(srcPort));
            }
        }

        struct sockaddr_in* remoteAddr 
            = &(csGroup->serverAddr.inAddr);
        memset(remoteAddr, 0, sizeof(SockAddr_t));
        remoteAddr->sin_family = AF_INET;
        inet_pton(AF_INET
                    , dstIpGroups[gIndex]
                    , &(remoteAddr->sin_addr));
        remoteAddr->sin_port = htons(dstPort);

        csGroup->csDataLen = 100;
        csGroup->scDataLen = 100;
        csGroup->cCloseMethod = EmTcpFIN; 
        csGroup->csCloseType = EmDataFinish;
        csGroup->csWeight = 1;  
    }

    appI->connPerSec = 1200;
    appI->maxActSessions = 100000;
    appI->maxErrSessions = 100000;
    appI->maxSessions = 100000;

    appCtx->appI = appI;

    appCtx->nAdminChannelState = N_ADMIN_CHANNEL_STATE_RECV_CONFIG;  
}

static void MsgIoOnError (MsgIoChannelId_t mioChannelId) {

    TcpClientCtx_t* appCtx = (TcpClientCtx_t*) MsgIoGetCtx (mioChannelId);

    appCtx->nAdminChannelErr = N_ADMIN_CHANNEL_ERROR_CONN;
}

static void MsgIoOnMsgRecv (MsgIoChannelId_t mioChannelId) {

    // TcpClientCtx_t* appCtx = (TcpClientCtx_t*) MsgIoGetCtx (mioChannelId);

    // char* msgData;
    // int msgLen;
    // MsgIoRecv (mioChannelId, &msgData, &msgLen);
    // msgData[msgLen] = '\0';

    // JsonNode *root;
    // GError *error = NULL;

    // root = json_from_string (msgData, &error);

    // if (error) {
        
    // } else {
    //    json_node_get_object (root); 
    // }
    
    // char* srcIpGroup1[] = { "12.20.50.2"
    //             , "12.20.50.3"
    //             , "12.20.50.4"
    //             , "12.20.50.5"
    //             , "12.20.50.6"
    //             , "12.20.50.7"
    //             , "12.20.50.8"
    //             , "12.20.50.9"
    //             , "12.20.50.10"
    //             , "12.20.50.11"
    //             , "12.20.50.12"
    //             , "12.20.50.13"
    //             , "12.20.50.14"
    //             , "12.20.50.15"
    //             , "12.20.50.16"
    //             , "12.20.50.17"
    //             , "12.20.50.18"
    //             , "12.20.50.19"
    //             , "12.20.50.20"
    //             , "12.20.50.21"
    //             , "12.20.50.22"
    //             , "12.20.50.23"
    //             , "12.20.50.24"
    //             , "12.20.50.25"
    //             , "12.20.50.26"
    //             , "12.20.50.27"
    //             , "12.20.50.28"
    //             , "12.20.50.29"
    //             , "12.20.50.30"
    //             , "12.20.50.31"};

    // char** srcIpGroups[1];
    // srcIpGroups[0] = srcIpGroup1;

    // char* dstIpGroups[1] = { "12.20.60.2"};
    // int dstPort = 443;

    // int csGroupClientAddrCountArr[1] = {30};

    // int csGroupCount = 1;

    // TcpClientI_t* appI 
    //     = (TcpClientI_t*) mmap(NULL
    //         , sizeof (TcpClientI_t)
    //         , PROT_READ | PROT_WRITE
    //         , MAP_SHARED | MAP_ANONYMOUS
    //         , -1
    //         , 0);

    // appI->csGroupCount = csGroupCount;
    // appI->csGroupArr 
    //     = (TcpClientGroup_t*) mmap(NULL
    //         , sizeof (TcpClientGroup_t) * appI->csGroupCount
    //         , PROT_READ | PROT_WRITE
    //         , MAP_SHARED | MAP_ANONYMOUS
    //         , -1
    //         , 0);
    
    // appI->nextCsGroupIndex = 0;
    // for (int gIndex = 0; gIndex < appI->csGroupCount; gIndex++) {
    //     TcpClientGroup_t* csGroup = &appI->csGroupArr[gIndex];
    //     csGroup->clientAddrCount = csGroupClientAddrCountArr[gIndex];
    //     csGroup->nextClientAddrIndex = 0;
    //     csGroup->clientAddrArr
    //         = (SockAddr_t*) mmap(NULL
    //             , sizeof (SockAddr_t) * csGroup->clientAddrCount
    //             , PROT_READ | PROT_WRITE
    //             , MAP_SHARED | MAP_ANONYMOUS
    //             , -1
    //             , 0);
    //     csGroup->LocalPortPoolArr 
    //         = (LocalPortPool_t*) mmap(NULL
    //             , sizeof (LocalPortPool_t) * csGroup->clientAddrCount
    //             , PROT_READ | PROT_WRITE
    //             , MAP_SHARED | MAP_ANONYMOUS
    //             , -1
    //             , 0);
    //     for (int cIndex = 0
    //             ; cIndex < csGroup->clientAddrCount
    //             ; cIndex++) {
        
    //         struct sockaddr_in* localAddr 
    //             = &(csGroup->clientAddrArr[cIndex].inAddr);
    //         memset(localAddr, 0, sizeof(SockAddr_t));
    //         localAddr->sin_family = AF_INET;
    //         inet_pton(AF_INET
    //                     , srcIpGroups[gIndex][cIndex]
    //                     , &(localAddr->sin_addr));

    //         LocalPortPool_t* portQ = &csGroup->LocalPortPoolArr[cIndex];
    //         InitPortBindQ(portQ);
    //         for (int srcPort = 5000; srcPort <= 65000; srcPort++) {
    //             SetPortToPool(portQ, htons(srcPort));
    //         }
    //     }

    //     struct sockaddr_in* remoteAddr 
    //         = &(csGroup->serverAddr.inAddr);
    //     memset(remoteAddr, 0, sizeof(SockAddr_t));
    //     remoteAddr->sin_family = AF_INET;
    //     inet_pton(AF_INET
    //                 , dstIpGroups[gIndex]
    //                 , &(remoteAddr->sin_addr));
    //     remoteAddr->sin_port = htons(dstPort);

    //     csGroup->csDataLen = 3000;
    //     csGroup->scDataLen = 3000;
    //     csGroup->cCloseMethod = EmTcpFIN; 
    //     csGroup->sCloseMethod = EmTcpFIN;
    //     csGroup->csCloseType = EmDataFinish;
    //     csGroup->csWeight = 1;  
    // }

    // appI->maxEvents = 0;
    // appI->connPerSec = 2000;
    // appI->maxActSessions = 100000;
    // appI->maxErrSessions = 10000;
    // appI->maxSessions = 100000;

    // appCtx->appI = appI;

    // appCtx->nAdminChannelState = N_ADMIN_CHANNEL_STATE_RECV_CONFIG;
}

static void MsgIoOnMsgSent (MsgIoChannelId_t mioChannelId) {

}

static TcpClientCtx_t* InitApp (char* nAdminTestId
                                , char* nAdminAddr
                                , int nAdminPort) {

    int status = -1;

    TcpClientCtx_t* appCtx = CreateStruct0 (TcpClientCtx_t);
    
    if (appCtx) {

        appCtx->nAdminChannelErr = 0;

        strcpy (appCtx->nAdminTestId, nAdminTestId);

        SetSockAddress (&appCtx->nAdminAddr, nAdminAddr, nAdminPort);

        SetSockAddress0 (&appCtx->nLocalAddr, 0); 

        MsgIoMethods_t mioMethods;
        mioMethods.OnOpen = &MsgIoOnOpen;
        mioMethods.OnError = &MsgIoOnError;
        mioMethods.OnMsgRecv = &MsgIoOnMsgRecv;
        mioMethods.OnMsgSent = &MsgIoOnMsgSent;

        appCtx->nAdminChannelId =  MsgIoNew (&appCtx->nLocalAddr
                                                , &appCtx->nAdminAddr
                                                , &mioMethods
                                                , appCtx);
        
        if (appCtx->nAdminChannelId) {

            appCtx->nAdminChannelState = N_ADMIN_CHANNEL_STATE_INIT;

            while ( 1 ) {

                MsgIoProcess (appCtx->nAdminChannelId);

                if (MsgIoTimeElapsed (appCtx->nAdminChannelId) 
                                    > N_ADMIN_GET_CONFIG_MAX_TIME) {
                    
                    appCtx->nAdminChannelErr 
                        = N_ADMIN_CHANNEL_ERROR_GET_CONFIG;
                }

                if (appCtx->nAdminChannelErr) {
                    break;
                }

                if (appCtx->nAdminChannelState == N_ADMIN_CHANNEL_STATE_RECV_CONFIG) {

                    IoVentMethods_t iovMethods;
                    iovMethods.OnEstablish = &OnEstablish;
                    iovMethods.OnWriteNext = &OnWriteNext;
                    iovMethods.OnWriteStatus = &OnWriteStatus;
                    iovMethods.OnReadNext = &OnReadNext;
                    iovMethods.OnReadStatus = &OnReadStatus;
                    iovMethods.OnCleanup = &OnCleanup;
                    iovMethods.OnStatus = &OnStatus;
                    iovMethods.OnContinue = &OnContinue;

                    IoVentOptions_t iovOptions;
                    iovOptions.maxActiveConnections = appCtx->appI->maxActSessions;
                    iovOptions.maxErrorConnections = appCtx->appI->maxErrSessions;
                    iovOptions.maxEvents = 0;
                    iovOptions.eventPTO = DEFAULT_MAX_POLL_TIMEOUT;

                    appCtx->iovCtx 
                        = CreateIoVentCtx (&iovMethods, &iovOptions, appCtx);

                    if (appCtx->iovCtx) {

                        CreatePool (&appCtx->freeSessionPool
                                    , appCtx->appI->maxActSessions
                                    , TcpClientSession_t);

                        InitPool (&appCtx->activeSessionPool);
                    }

                    break;
                }
            }
        }

        if (appCtx->nAdminChannelId
                && appCtx->iovCtx
                && appCtx->freeSessionPool) {
            status = 0;
        }
    }

    if (status) {

        if (appCtx) {

            if (appCtx->nAdminChannelId) {
                MsgIoDelete (appCtx->nAdminChannelId);
            }

            if (appCtx->iovCtx) {
                DeleteIoVentCtx (appCtx->iovCtx);
            }

            if (appCtx->freeSessionPool) {
                //??? clean up pool
            }

            DeleteStruct (TcpClientCtx_t, appCtx);

            appCtx = NULL;
        }
    }

    return appCtx;
}

int main(int argc, char** argv) {

    TcpClientCtx_t* appCtx 
        = InitApp ( argv[1], argv[2], atoi(argv[3]) );

    if (appCtx == NULL) {
        exit (-1); //???
    }

    TcpClientI_t* appI = appCtx->appI;

    IoVentCtx_t* iovCtx = appCtx->iovCtx;

    double lastConnInitTime 
        = TimeElapsedIoVentCtx (iovCtx);

    double lastMsgIoTime
        = MsgIoTimeElapsed (appCtx->nAdminChannelId);

    char statsString[256];

    while (1) {

        MsgIoProcess (appCtx->nAdminChannelId);

        if ( (MsgIoTimeElapsed (appCtx->nAdminChannelId) - lastMsgIoTime) >= 2 ) {

            lastMsgIoTime = MsgIoTimeElapsed (appCtx->nAdminChannelId);

            sprintf (statsString, 
                        "TC : Init = %" PRIu64 "; " 
                        "Succ = %" PRIu64 "; "
                        "Fail = %" PRIu64
                        "\n"
                        , GetConnStats(&appI->gStats, tcpConnInit)
                        , GetConnStats(&appI->gStats, tcpConnInitSuccess)
                        , GetConnStats(&appI->gStats, tcpConnInitFail)
                        );

            MsgIoSend (appCtx->nAdminChannelId
                        , statsString
                        , strlen(statsString));

        }
        
        if ( ProcessIoVent (iovCtx) == 0 ) {
            break;
        }

        uint64_t tcpConnInitCount 
            = GetConnStats(&appI->gStats, tcpConnInit);

        int newConnectionInits 
            = (TimeElapsedIoVentCtx (iovCtx) - lastConnInitTime) 
                * appI->connPerSec;

        while (newConnectionInits > 0
                    && tcpConnInitCount < appI->maxSessions) {

            //new connection init
            TcpClientGroup_t* csGroup
                = &appI->csGroupArr[appI->nextCsGroupIndex];

            SockAddr_t* localAddress 
                = &(csGroup->clientAddrArr[csGroup->nextClientAddrIndex]);

            SockAddr_t* remoteAddress 
                = &(csGroup->serverAddr);

            LocalPortPool_t* localPortPool 
                = &csGroup->LocalPortPoolArr[csGroup->nextClientAddrIndex];

            TcpClientSession_t* newSess = GetSession (iovCtx->appCtx);

            if (newSess == NULL) {

                IncConnStats2 ( &appI->gStats
                                , &csGroup->cStats
                                , appSessStructNotAvail );
                break;
            }

            appI->nextCsGroupIndex += 1;
            if (appI->nextCsGroupIndex == appI->csGroupCount){
                appI->nextCsGroupIndex = 0;
            }
            
            csGroup->nextClientAddrIndex += 1;
            if (csGroup->nextClientAddrIndex == csGroup->clientAddrCount) {
                csGroup->nextClientAddrIndex = 0;
            }

            int newConnInitErr = 
                NewConnection (iovCtx
                                , csGroup
                                , newSess
                                , localAddress
                                , localPortPool
                                , remoteAddress
                                , 0
                                , &csGroup->cStats
                                , &appI->gStats);

            if (newConnInitErr) {
                FreeSession (newSess);
            }  

            newConnectionInits -= 1;

            lastConnInitTime = TimeElapsedIoVentCtx (iovCtx);

            tcpConnInitCount 
                = GetConnStats(&appI->gStats, tcpConnInit);
        }
    }

    DumpErrConnections (iovCtx);

    MsgIoDelete (appCtx->nAdminChannelId);

    DeleteIoVentCtx (iovCtx);

    return 0;
}

