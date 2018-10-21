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

#include "tcp_server.h"

TcpClientApp_t* AppO;
TcpClientAppInterface_t* AppI;

void InitSession(TcpServerSession_t* newSess
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

TcpServerSession_t* GetFreeSession() {

    TcpServerSession_t* newSess 
        = GetSesionFromPool (AppO->freeSessionPool);

    if (newSess != NULL) {
        SetSessionToPool (AppO->activeSessionPool, newSess);
        InitSession(newSess, 0, 0);
    }

    return newSess;
}

void SetFreeSession(TcpServerSession_t* newSess) {
    
    RemoveFromSessionPool (AppO->activeSessionPool, newSess);
    SetSessionToPool (AppO->freeSessionPool, newSess);
}

void InitApp() {

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
     
    AppO->EventArr = CreateEventArray(AppI->maxEvents);

    AppO->eventQ = CreateEventQ();

    AppO->appGroupConnStats = CreateArray (TcpServerConnStats_t
                                , AppI->csGroupCount);

    AppO->serverSessionArr = CreateArray (TcpServerSession_t
                                , AppI->csGroupCount);

    for (int i = 0; i < AppI->csGroupCount; i++) {

        TcpServerSession_t* aSession = &AppO->serverSessionArr[i];

        InitSession (aSession, 1, SESSION_TYPE_LISTENER);
    }

    AppO->readBuffer = TdMalloc(AppI->csDataLen);
}

void CleanupApp() {
    DeleteEventQ(AppO->eventQ);
}

void TcpServerAppRun(TcpServerAppInterface_t* appIface){

    AppO = CreateStruct0 (TcpServerApp_t);
    AppI = appIface;
    InitApp();


    for (int i = 0; i < AppI->csGroupCount; i++) {

        TcpServerSession_t* srvSess = &AppO->serverSessionArr[i];
        TcpServerAppConnGroup_t* csGroup = &AppI->csGroupArr[i];
        
        TcpServerConnection_t* newConn = &srvSess->tcConn;
        newConn->localAddress = (struct sockaddr*) &(csGroup->serverAddr);
        newConn->tcSess->groupConnStats = &csGroup->cStats;

        newConn->socketFd 
            = TcpListenStart(newConn->localAddress
                                , 1000
                                , &AppI->appConnStats
                                , srvSess->groupConnStats
                                , newConn);

        RegisterForReadEvent(AppO->eventQ
                                , newConn->socketFd
                                , newConn);
    }

    while (true) {

        int eCount = GetIOEvents(AppO->eventQ, AppO->EventArr
                                , AppI->maxEvents);

        if (eCount > 0){
            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                TcpServerConnection_t* newConn  
                            = (TcpServerConnection_t*)
                                GetIOEventData(AppO->EventArr[eIndex]);
                
                if (newConn->tcSess->sessionType == SESSION_TYPE_LISTENER){

                    if (IsReadEventSet(AppO->EventArr[eIndex])) {

                        int newSocketFd = accept(newConn->socketFd
                                                , NULL
                                                , NULL);

                        if ( newSocketFd > 0 ){

                            int flags = fcntl(newSocketFd, F_GETFL, 0);
                            //do error handling ??? 
                            fcntl(newSocketFd, F_SETFL, flags | O_NONBLOCK);
                            
                            //do error handling ??? 
                            setsockopt(newSocketFd, SOL_SOCKET
                                , SO_REUSEADDR, &(int){ 1 }, sizeof(int));

                            TcpServerSession_t* newSess = GetFreeSession ();

                            if (newSess == NULL) {

                                AppI->appStats->dbgNoFreeSession++;

                                close(newSocketFd);      
                            }else {

                                newSess->tcConn.socketFd = newSocketFd;

                                RegisterForReadEvent(AppO->eventQ
                                                    , newSess->tcConn.socketFd
                                                    , &newSess->tcConn);
                            }
                        } else { //do error handling ???
                            
                        }
                    }else { //do error handling ???

                    }
                }else {
                    int bytesReceived 
                        = TcpRead (newConn->socketFd
                                    , AppO->readBuffer
                                    , AppO->readBufLen
                                    , &AppI->appConnStats
                                    , groupConnStats
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

    return 0;
}

TcpServerAppInterface_t* CreateTcpServerAppInterface (int csGroupCount) {

    TcpServerAppInterface_t* iFace 
        = (TcpServerAppInterface_t*) mmap(NULL
            , sizeof (TcpServerAppInterface_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    iFace->csGroupCount = csGroupCount;
    iFace->csGroupArr 
        = (TcpServerAppConnGroup_t*) mmap(NULL
            , sizeof (TcpServerAppConnGroup_t) * iFace->csGroupCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    return iFace;
}