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

#include "tcp_client.h"

TcpClientApp_t* AppObj;
TcpClientAppInterface_t* AppIface;

void InitSession(TcpClientSession_t* newSess) {

    TcpClientConnection_t* newConn = &newSess->tcConn; 
    
    SSInit(newConn);

    newConn->socketFd = 0;
    newConn->isIpv6 = 0;
    newConn->localAddress = NULL;
    newConn->remoteAddress = NULL;
    newConn->bytesSent = 0;
}

void InitSessionInitApp(TcpClientSession_t* newSess) {

    newSess->tcConn.tcSess = newSess;

    InitSession(newSess); 
}

void SetSessionAddress(TcpClientConnection_t* newConn
                        , int isIpv6
                        , struct sockaddr* localAddr
                        , struct sockaddr* remoteAddr) 
{
    newConn->isIpv6 = isIpv6;
    newConn->localAddress = localAddr;
    newConn->remoteAddress = remoteAddr;
}

TcpClientSession_t* GetFreeSession() {

    TcpClientSession_t* newSess 
        = GetAnySesionFromPool (AppObj->freeSessionPool);

    if (newSess != NULL) {
        AddToSessionPool (AppObj->activeSessionPool, newSess);
        InitSession(newSess);
    }

    return newSess;
}

void SetFreeSession(TcpClientSession_t* newSess) {
    
    RemoveFromSessionPool (AppObj->activeSessionPool, newSess);
    AddToSessionPool (AppObj->freeSessionPool, newSess);
}

void StoreErrSession(TcpClientSession_t* aSession) {
    if (AppObj->errorSessionCount < AppIface->maxErrorSessions) {

        TcpClientSession_t* errSession 
                = &AppObj->errorSessionArr[AppObj->errorSessionCount];

        *errSession =  *aSession;

        AppObj->errorSessionCount++;
    }
}

void DumpErrSessions() {
    if (AppObj->errorSessionCount) 
    {
        for (int i = 0; i < AppObj->errorSessionCount; i++) {

            TcpClientSession_t* newSess 
                    = &AppObj->errorSessionArr[i];

            TcpClientConnection_t* newConn 
                    = &newSess->tcConn;

            struct sockaddr_in* localAddress 
                = (struct sockaddr_in*)newConn->localAddress;
            struct sockaddr_in* remoteAddress 
                = (struct sockaddr_in*)newConn->remoteAddress;
            char srcAddr[INET_ADDRSTRLEN];
            char dstAddr[INET_ADDRSTRLEN];
            
            inet_ntop(AF_INET, &(localAddress->sin_addr), srcAddr, INET_ADDRSTRLEN);
            inet_ntop(AF_INET, &(remoteAddress->sin_addr), dstAddr, INET_ADDRSTRLEN);

            printf ("SS1 = %#018" PRIx64 
                    ", Err = %d" 
                    ", SysErr = %d"
                    ", src = %s : %hu"
                    ", dst = %s : %hu\n"
                    , GetSS1(newConn)
                    , GetSSLastErr(newConn)
                    , GetErrno(newConn) 
                    , srcAddr, ntohs(localAddress->sin_port)
                    , dstAddr, ntohs(remoteAddress->sin_port)); 
        }
    }
}

void InitApp() {

    AppObj->freeSessionPool = AllocEmptySessionPool();
    AppObj->activeSessionPool = AllocEmptySessionPool();

    for (int i = 0; i < AppIface->maxActiveSessions; i++) {

        TcpClientSession_t* aSession 
                    = AllocSession (TcpClientSession_t);

        InitSessionInitApp (aSession);

        AddToSessionPool (AppObj->freeSessionPool, aSession);
    }

    AppObj->errorSessionCount = 0;
    
    AppObj->errorSessionArr = CreateArray (TcpClientSession_t
                                , AppIface->maxErrorSessions); 

    AppObj->EventArr = CreateEventArray(AppIface->maxEvents);

    AppObj->eventQId = CreateEventQ();

    AppObj->timerWheel = CreateTimerWheel();

    //malloc ???
    AppObj->sendBuffer =  malloc(AppIface->csDataLen);
}

void DumpTcpClientAppStats(TcpClientAppConnStats_t* appConnStats) {
    
    char statsString[120];

    sprintf (statsString, 
                        "%" PRIu64 "\n" 
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "%" PRIu64 "\n"
                        "\n"
        , GetSStats(appConnStats, tcpConnInit)
        , GetSStats(appConnStats, tcpConnInitSuccess)
        , GetSStats(appConnStats, tcpConnInitFail)
        , GetSStats(appConnStats, tcpConnInitFailEaddrNotAvail)
        , GetSStats(appConnStats, tcpConnRegisterForWriteEventFail)
        );

    puts (statsString);
}

void CleanupApp() {

    DeleteEventQ(AppObj->eventQId);

    DeleteTimerWheel(AppObj->timerWheel);

    DumpTcpClientAppStats(&AppIface->appConnStats);

    DumpErrSessions();
}

void ReleaseBindPort(TcpClientConnection_t* newConn){
    AddToPortBindQ (newConn->clientPortBindQ
                    , ((struct sockaddr_in*)newConn->localAddress)->sin_port);
}

void TcpClienAppRun(TcpClientAppInterface_t* appIface)
{
    AppObj = CreateStruct0 (TcpClientApp_t);
    AppIface = appIface;

    InitApp();
    TcpClientAppConnStats_t* appConnStats = &AppIface->appConnStats;
    TcpClientAppStats_t* appStats = &AppIface->appStats;

    //hard coded
    int clientAddIndex = 0;
    //hard coded 

    time_t epochSinceSeconds = time(NULL);
    printf("%ld - -\n", epochSinceSeconds);

    double lastConnectionInitTime = TimeElapsedTimerWheel(AppObj->timerWheel);

    while (true) {

        if ( (GetSStats(appConnStats, tcpConnInitFail) 
                    == AppIface->maxErrorSessions) 
                || (GetSStats(appConnStats, tcpConnInit) 
                    == AppIface->maxSessions 
                && IsSessionPoolEmpty (AppObj->activeSessionPool)) ){
            break;
        }

        if (GetSStats(appConnStats, tcpConnInit) 
                < AppIface->maxSessions) {
 
            int newConnectionInits 
                = ((TimeElapsedTimerWheel(AppObj->timerWheel)
                        - lastConnectionInitTime)
                    * AppIface->connectionPerSec);

            while (newConnectionInits > 0 
                    && GetSStats(appConnStats, tcpConnInit) 
                        < AppIface->maxSessions) {
                newConnectionInits--;

                lastConnectionInitTime 
                        = TimeElapsedTimerWheel(AppObj->timerWheel);

                TcpClientSession_t* newSess = GetFreeSession ();

                if (newSess == NULL) {
                    appStats->dbgNoFreeSession++;      
                }else {

                    TcpClientConnection_t* newConn = &newSess->tcConn;
                    
                    SockAddr_t* clientAddrArr 
                        = AppIface->csGroupArr[0].clientAddrArr;
                                        
                    struct sockaddr_in* nextClientAddr 
                        = &(clientAddrArr[clientAddIndex].inAddr);

                    PortBindQ_t*clientPortBindArr 
                        = AppIface->csGroupArr[0].clientPortBindArr;

                    newConn->clientPortBindQ = &clientPortBindArr[clientAddIndex];

                    int nextSrcPort = GetFromPortBindQ(newConn->clientPortBindQ);
                    // ??? error handling 

                    clientAddIndex++;
                    if (clientAddIndex == AppIface->csGroupArr[0].clientAddrCount) {
                        clientAddIndex = 0;
                    }

                    struct sockaddr_in* nextServerAddr 
                        = &(AppIface->csGroupArr[0].serverAddr.inAddr); 

                    SetSessionAddress(newConn, 0
                                , (struct sockaddr*) nextClientAddr
                                , (struct sockaddr*) nextServerAddr);

                    TcpClientAppConnStats_t* groupConnStats 
                                        = &AppIface->csGroupArr[0].cStats; 

                    newSess->groupConnStats = groupConnStats;

                    SetAppState (newConn, APP_STATE_CONNECTION_IN_PROGRESS);


                    nextClientAddr->sin_port = nextSrcPort;

                    IncSStats2(appConnStats
                                , groupConnStats
                                , tcpConnInit);

                    newConn->socketFd 
                        = TcpNewConnection(newConn->isIpv6, 
                                        newConn->localAddress, 
                                        newConn->remoteAddress,
                                        appConnStats,
                                        groupConnStats,
                                        newConn);


                    if ( GetSSLastErr (newConn) ) {
                        if (GetSSLastErr (newConn) 
                                == TD_SOCKET_CONNECT_FAILED_IMMEDIATE
                            && GetErrno(newConn) == EADDRNOTAVAIL) {
                                IncSStats2(appConnStats
                                        , groupConnStats
                                        , tcpConnInitFailEaddrNotAvail);
                        }//else{
                            IncSStats2(appConnStats
                                        , groupConnStats
                                        , tcpConnInitFail);
                            StoreErrSession (newSess);
                            SetFreeSession (newSess);
                            ReleaseBindPort (newConn);
                        //}
                    } else {
                        if ( RegisterForWriteEvent(AppObj->eventQId
                                        , newConn->socketFd
                                        , newConn)){

                            SaveErrno(newConn);                
                            IncSStats2(appConnStats
                                    , groupConnStats
                                    , tcpConnRegisterForWriteEventFail);
                            StoreErrSession(newSess);
                            SetFreeSession (newSess);
                            ReleaseBindPort (newConn);
                        }
                    }
                }
            }
        }

        int eCount = GetIOEvents(AppObj->eventQId, AppObj->EventArr
                                    , AppIface->maxEvents);
        if (eCount > 0){
            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                TcpClientConnection_t* newConn 
                            = (TcpClientConnection_t*)
                                GetIOEventData(AppObj->EventArr[eIndex]);

                TcpClientSession_t* newSess = newConn->tcSess;
                
                TcpClientAppConnStats_t* groupConnStats = newSess->groupConnStats;

                if (IsWriteEventSet(AppObj->EventArr[eIndex])) {
                    if ( GetAppState(newConn) 
                            == APP_STATE_CONNECTION_IN_PROGRESS ) {

                        if ( IsNewTcpConnectionComplete(newConn->socketFd) ){
                            
                            IncSStats2(appConnStats
                                        , groupConnStats
                                        , tcpConnInitSuccess);

                            SetAppState (newConn
                                        , APP_STATE_CONNECTION_ESTABLISHED);

                            SetSS1(newConn, STATE_TCP_CONN_ESTABLISHED);
                        }else{
                            IncSStats2(appConnStats
                                        , groupConnStats
                                        , tcpConnInitFail);

                            close(newConn->socketFd);
                            ReleaseBindPort (newConn);

                            SetSS1(newConn, STATE_TCP_SOCK_FD_CLOSE);

                            SetFreeSession (newSess);
                            //to capture the error ?
                        }
                    } else if ( GetAppState(newConn) == 
                                     APP_STATE_CONNECTION_ESTABLISHED 
                                && newConn->bytesSent < 
                                    AppIface->csDataLen ) {

                        int bytesToSend 
                            = AppIface->csDataLen - newConn->bytesSent;

                        const char* sendBuffer
                            = &AppObj->sendBuffer[newConn->bytesSent];

                        int bytesSent 
                            = TcpWrite (newConn->socketFd
                                        , sendBuffer
                                        , bytesToSend
                                        , appConnStats
                                        , groupConnStats
                                        , newConn);

                        if (GetSSLastErr (newConn)) {
                            close(newConn->socketFd);
                            ReleaseBindPort (newConn);

                            SetSS1(newConn, STATE_TCP_SOCK_FD_CLOSE);

                            StoreErrSession(newSess);

                            SetFreeSession (newSess);
                        }else {
                            newConn->bytesSent += bytesSent;

                            if (newConn->bytesSent == AppIface->csDataLen) {
                                close(newConn->socketFd);
                                ReleaseBindPort (newConn);

                                SetSS1(newConn, STATE_TCP_SOCK_FD_CLOSE);

                                SetFreeSession (newSess);
                            }
                        }
                    }
                }
            }
        }
    }

    epochSinceSeconds = time(NULL);
    printf("-- %ld\n", epochSinceSeconds);

    CleanupApp();
    
    AppIface->isRunning = 0;
}

TcpClientAppInterface_t* CreateTcpClienAppInterface(int csGroupCount
                                            , int* clientAddrCounts)
{
    TcpClientAppInterface_t* iFace 
        = (TcpClientAppInterface_t*) mmap(NULL
            , sizeof (TcpClientAppInterface_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    iFace->csGroupCount = csGroupCount;
    iFace->csGroupArr 
        = (TcpClientAppConnGroup_t*) mmap(NULL
            , sizeof (TcpClientAppConnGroup_t) * iFace->csGroupCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    for (int gIndex = 0; gIndex < iFace->csGroupCount; gIndex++) {
        TcpClientAppConnGroup_t* csGroup = &iFace->csGroupArr[gIndex];
        csGroup->clientAddrCount = clientAddrCounts[gIndex];
        csGroup->clientAddrArr
            = (SockAddr_t*) mmap(NULL
                , sizeof (SockAddr_t) * csGroup->clientAddrCount
                , PROT_READ | PROT_WRITE
                , MAP_SHARED | MAP_ANONYMOUS
                , -1
                , 0);
        csGroup->clientPortBindArr 
            = (PortBindQ_t*) mmap(NULL
                , sizeof (PortBindQ_t) * csGroup->clientAddrCount
                , PROT_READ | PROT_WRITE
                , MAP_SHARED | MAP_ANONYMOUS
                , -1
                , 0);
        for (int cIndex = 0
                ; cIndex < csGroup->clientAddrCount
                ; cIndex++) {
            InitPortBindQ(&csGroup->clientPortBindArr[cIndex]);
        }
    }

    return iFace;
}

void DeleteTcpClienAppInterface (TcpClientAppInterface_t* iFace)
{
    //todo
}