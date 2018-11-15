#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>

#define TCP_SERVER_MAIN
#include "tcp_server.h"

static TsAppRun_t* AppO;
static TsAppInt_t* AppI;

static void InitSession(TsSess_t* newSess
                        , int initApp
                        , int sessionType) {

    TsConn_t* newConn = &newSess->tcConn; 
    
    if (initApp){
        newSess->tcConn.tcSess = newSess;
        newSess->sessionType = sessionType;
    }

    CSInit(newConn);

    newConn->socketFd = 0;
    newConn->bytesReceived = 0;
}

static TsSess_t* GetFreeSession() {

    TsSess_t* newSess 
        = GetSesionFromPool (AppO->freeSessionPool);

    if (newSess != NULL) {
        SetSessionToPool (AppO->activeSessionPool, newSess);
        InitSession(newSess, 0, 0);
    }

    return newSess;
}

static void SetFreeSession(TsSess_t* newSess) {
    
    RemoveFromSessionPool (AppO->activeSessionPool, newSess);
    SetSessionToPool (AppO->freeSessionPool, newSess);
}

static void StoreErrSession(TsSess_t* aSession) {

    if (AppO->errorSessionCount < AppI->maxErrorSessions) {

        TsSess_t* errSession 
                = &AppO->errorSessionArr[AppO->errorSessionCount];

        *errSession =  *aSession;

        errSession->tcConn.savedLocalPort 
            = ntohs(GetSockPort(errSession->tcConn.localAddress));

        errSession->tcConn.savedRemotePort 
            = ntohs(GetSockPort(&errSession->tcConn.remoteAddress));

        AppO->errorSessionCount++;
    }
}

static void InitApp() {

    AppO->freeSessionPool = AllocEmptySessionPool();
    AppO->activeSessionPool = AllocEmptySessionPool();

    for (int i = 0; i < AppI->maxActiveSessions; i++) {

        TsSess_t* aSession 
            = AllocSession (TsSess_t);

        InitSession (aSession, 1, SESSION_TYPE_CONNECTION);

        SetSessionToPool (AppO->freeSessionPool, aSession);
    }

    AppO->errorSessionCount = 0;

    AppO->errorSessionArr = CreateArray (TsSess_t
                                , AppI->maxErrorSessions); 
     
    AppO->EventArr = CreateArray(PollEvent_t
                                , AppI->maxEvents);

    AppO->eventQ = CreateEventQ();

    AppO->serverSessionArr = CreateArray (TsSess_t
                                , AppI->csGroupCount);

    for (int i = 0; i < AppI->csGroupCount; i++) {

        TsSess_t* aSession = &AppO->serverSessionArr[i];

        InitSession (aSession, 1, SESSION_TYPE_LISTENER);
    }

    AppO->sendBuffer = TdMalloc(AppI->scDataLen);
    AppO->readBuffer = TdMalloc(AppI->csDataLen);

    AppO->eventPTO = 0;
}

static void CleanupApp() {
    DeleteEventQ(AppO->eventQ);
}

static void CloseConnection(TsConn_t* newConn) {

    if ( IsSetCES(newConn, STATE_TCP_SOCK_POLL_UPDATE_FAIL) == 0 ) {

        StopPollReadWriteEvent(AppO->eventQ
                                , newConn->socketFd
                                , &AppI->appConnStats
                                , newConn->tcSess->groupConnStats
                                , newConn);
    }

    TcpClose(newConn->socketFd, newConn);

    if ( GetCES(newConn) ) {
        StoreErrSession (newConn->tcSess);
    }

    SetFreeSession (newConn->tcSess);
}

static void AcceptConnection(TsConn_t* newConn
                                , TsConn_t* lSockConn) {

    newConn->localAddress = lSockConn->localAddress;

    newConn->socketFd = TcpAcceptConnection(lSockConn->socketFd
                                            , &newConn->remoteAddress
                                            , &AppI->appConnStats
                                            , newConn->tcSess->groupConnStats
                                            , newConn);

    if ( GetCES(newConn) ) {
        StoreErrSession (newConn->tcSess);
        SetFreeSession (newConn->tcSess); 
    } else {
        PollReadEventOnly(AppO->eventQ
                            , newConn->socketFd
                            , &AppI->appConnStats
                            , newConn->tcSess->groupConnStats
                            , newConn);

        if ( GetCES(newConn) ) {
            CloseConnection(newConn);
        }
    }
}

static void InitServer(TsConn_t* newConn) {

    newConn->socketFd 
        = TcpListenStart(newConn->localAddress
                            , AppI->listenQLen 
                            , &AppI->appConnStats
                            , newConn->tcSess->groupConnStats
                            , newConn);

    if ( GetCES(newConn) ) {
        StoreErrSession (newConn->tcSess); 
    } else {
        PollReadEventOnly(AppO->eventQ
                            , newConn->socketFd
                            , &AppI->appConnStats
                            , newConn->tcSess->groupConnStats
                            , newConn);

        if ( GetCES(newConn) ) {
            CloseConnection(newConn);
        }
    }
}

void TcpServerRun(TsAppInt_t* appIface) {

    AppO = CreateStruct0 (TsAppRun_t);
    AppI = appIface;
    InitApp();

    for (int i = 0; i < AppI->csGroupCount; i++) {

        TsSess_t* srvSess = &AppO->serverSessionArr[i];
        TsGroup_t* csGroup = &AppI->csGroupArr[i];
        
        TsConn_t* newConn = &srvSess->tcConn;
        newConn->localAddress = &(csGroup->serverAddr);
        newConn->tcSess->groupConnStats = &csGroup->cStats;

        InitServer(newConn);
    }

    while (1) {

        int eCount = GetIOEvents(AppO->eventQ
                                    , AppO->EventArr
                                    , AppI->maxEvents
                                    , AppO->eventPTO);

        if (eCount > 0){

            AppO->eventPTO = 0;
            
            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                TsConn_t* newConn  
                    = (TsConn_t*) GetIOEventData(AppO->EventArr[eIndex]);
                
                if (newConn->tcSess->sessionType == SESSION_TYPE_LISTENER){
                    
                    if ( IsReadEventSet(AppO->EventArr[eIndex]) ) {

                        TsSess_t* newSess = GetFreeSession ();
                        if (newSess == NULL){
                            AppI->appStats.dbgNoFreeSession++;
                        }else {
                            TsConn_t* lSockConn = newConn;
                            newConn = &newSess->tcConn;
                            newConn->tcSess->groupConnStats 
                                = lSockConn->tcSess->groupConnStats;

                            AcceptConnection(newConn, lSockConn);
                        }
                    }
                }else {
                    int bytesReceived 
                        = TcpRead (newConn->socketFd
                                    , AppO->readBuffer
                                    , AppI->csDataLen
                                    , &AppI->appConnStats
                                    , newConn->tcSess->groupConnStats
                                    , newConn);
                    
                    if ( GetCES(newConn) ) {
                        CloseConnection(newConn);
                    } else {
                        if (bytesReceived == 0) {
                           CloseConnection(newConn); 
                        } else {
                            newConn->bytesReceived += bytesReceived;   
                        }
                    }
                }
            }
        }else{
            if (AppO->eventPTO < MAX_POLL_TIMEOUT) {
                AppO->eventPTO++;
            }
        }
    }

    CleanupApp();
}

TsAppInt_t* CreateTcpServerInterface (int csGroupCount) {

    TsAppInt_t* iFace 
        = (TsAppInt_t*) mmap(NULL
            , sizeof (TsAppInt_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    iFace->csGroupCount = csGroupCount;
    iFace->csGroupArr 
        = (TsGroup_t*) mmap(NULL
            , sizeof (TsGroup_t) * iFace->csGroupCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    return iFace;
}

void DeleteTcpServerInterface (TsAppInt_t* iFace)
{
    //todo
}