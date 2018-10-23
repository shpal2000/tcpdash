#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>

#include "tcp_server.h"

static TcpServer_t* AppO;
static TcpServerInterface_t* AppI;

static void InitSession(TcpServerSession_t* newSess
                    , int initApp
                    , int sessionType) {

    TcpServerConnection_t* newConn = &newSess->tcConn; 
    
    if (initApp){
        newSess->tcConn.tcSess = newSess;
        newSess->sessionType = sessionType;
    }

    CSInit(newConn);

    newConn->socketFd = 0;
    newConn->bytesReceived = 0;
}

static TcpServerSession_t* GetFreeSession() {

    TcpServerSession_t* newSess 
        = GetSesionFromPool (AppO->freeSessionPool);

    if (newSess != NULL) {
        SetSessionToPool (AppO->activeSessionPool, newSess);
        InitSession(newSess, 0, 0);
    }

    return newSess;
}

static void SetFreeSession(TcpServerSession_t* newSess) {
    
    RemoveFromSessionPool (AppO->activeSessionPool, newSess);
    SetSessionToPool (AppO->freeSessionPool, newSess);
}

static void StoreErrSession(TcpServerSession_t* aSession) {
    if (AppO->errorSessionCount < AppI->maxErrorSessions) {

        TcpServerSession_t* errSession 
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

        TcpServerSession_t* aSession 
            = AllocSession (TcpServerSession_t);

        InitSession (aSession, 1, SESSION_TYPE_CONNECTION);

        SetSessionToPool (AppO->freeSessionPool, aSession);
    }

    AppO->errorSessionCount = 0;

    AppO->errorSessionArr = CreateArray (TcpServerSession_t
                                , AppI->maxErrorSessions); 
     
    AppO->EventArr = CreateArray(PollEvent_t
                            , AppI->maxEvents);

    AppO->eventQ = CreateEventQ();

    AppO->serverSessionArr = CreateArray (TcpServerSession_t
                                , AppI->csGroupCount);

    for (int i = 0; i < AppI->csGroupCount; i++) {

        TcpServerSession_t* aSession = &AppO->serverSessionArr[i];

        InitSession (aSession, 1, SESSION_TYPE_LISTENER);
    }

    AppO->readBuffer = TdMalloc(AppI->csDataLen);
}

static void CleanupApp() {
    DeleteEventQ(AppO->eventQ);
}

static void OnListenStartError(TcpServerConnection_t* newConn) {

    IncConnStats2(&AppI->appConnStats
            , newConn->tcSess->groupConnStats 
            , tcpListenStartFail);

    StoreErrSession (newConn->tcSess); 
}

static void OnRegisterForListenerReadEventError(TcpServerConnection_t* newConn){

    IncConnStats2(&AppI->appConnStats
                , newConn->tcSess->groupConnStats 
                , tcpConnRegisterForListenerReadEventFail);    

    close(newConn->socketFd);
    SetCS1(newConn, STATE_TCP_SOCK_FD_CLOSE);
    
    StoreErrSession (newConn->tcSess);
}

void TcpServerRun(TcpServerInterface_t* appIface){

    AppO = CreateStruct0 (TcpServer_t);
    AppI = appIface;
    InitApp();

    for (int i = 0; i < AppI->csGroupCount; i++) {

        TcpServerSession_t* srvSess = &AppO->serverSessionArr[i];
        TcpServerConnGroup_t* csGroup = &AppI->csGroupArr[i];
        
        TcpServerConnection_t* newConn = &srvSess->tcConn;
        newConn->localAddress = &(csGroup->serverAddr);
        newConn->tcSess->groupConnStats = &csGroup->cStats;

        newConn->socketFd 
            = TcpListenStart((struct sockaddr*)newConn->localAddress
                                , 1000
                                , &AppI->appConnStats
                                , srvSess->groupConnStats
                                , newConn);
        if ( GetConnLastErr (newConn) ) {
            OnListenStartError(newConn);
        } else { 
            if ( RegisterForReadEvent(AppO->eventQ
                                    , newConn->socketFd
                                    , newConn) ) {
                OnRegisterForListenerReadEventError(newConn);
            }
        }
    }

    while (true) {

        int eCount = GetIOEvents(AppO->eventQ
                                    , AppO->EventArr
                                    , AppI->maxEvents);

        if (eCount > 0){
            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                TcpServerConnection_t* newConn  
                            = (TcpServerConnection_t*)
                                GetIOEventData(AppO->EventArr[eIndex]);
                
                if (newConn->tcSess->sessionType == SESSION_TYPE_LISTENER){
                    
                    if (IsReadEventSet(AppO->EventArr[eIndex])) {
                        
                        TcpServerSession_t* newSess = GetFreeSession ();
                        if (newSess == NULL){
                            AppI->appStats.dbgNoFreeSession++;
                        }else {
                            TcpServerConnection_t* lSockConn = newConn;
                            newConn = &newSess->tcConn;

                            socklen_t addrLen = sizeof (SockAddr_t);
                            newConn->socketFd = accept(lSockConn->socketFd
                                                    , GetSockAddr(newConn->remoteAddress)
                                                    , &addrLen);
                            newConn->localAddress = lSockConn->localAddress;

                            if ( newConn->socketFd > 0 ){

                                int flags = fcntl(newConn->socketFd, F_GETFL, 0);
                                //do error handling ??? 
                                fcntl(newConn->socketFd, F_SETFL, flags | O_NONBLOCK);
                                
                                //do error handling ??? 
                                setsockopt(newConn->socketFd, SOL_SOCKET
                                    , SO_REUSEADDR, &(int){ 1 }, sizeof(int));

                                RegisterForReadEvent(AppO->eventQ
                                                    , newConn->socketFd
                                                    , newConn);

                            } else { //do error handling ???
                                
                            }
                        }
                    }else { //do error handling ???

                    }
                }else {
                    int bytesReceived 
                        = TcpRead (newConn->socketFd
                                    , AppO->readBuffer
                                    , AppI->csDataLen
                                    , &AppI->appConnStats
                                    , newConn->tcSess->groupConnStats
                                    , newConn);
                    
                    if (GetConnLastErr (newConn)) {
                        close(newConn->socketFd);
                        SetCS1(newConn, STATE_TCP_SOCK_FD_CLOSE);
                        // error handling ???
                    } else {
                        if (bytesReceived == 0) {
                           close(newConn->socketFd);
                           SetCS1(newConn, STATE_TCP_SOCK_FD_CLOSE);
                           SetFreeSession (newConn->tcSess); 
                        } else {
                            newConn->bytesReceived += bytesReceived;   
                        }
                    }
                }
            }
        }
    }

    CleanupApp();
}

TcpServerInterface_t* CreateTcpServerInterface (int csGroupCount) {

    TcpServerInterface_t* iFace 
        = (TcpServerInterface_t*) mmap(NULL
            , sizeof (TcpServerInterface_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    iFace->csGroupCount = csGroupCount;
    iFace->csGroupArr 
        = (TcpServerConnGroup_t*) mmap(NULL
            , sizeof (TcpServerConnGroup_t) * iFace->csGroupCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    return iFace;
}

void DeleteTcpServerInterface (TcpServerInterface_t* iFace)
{
    //todo
}