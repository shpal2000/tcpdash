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

TcpServerApp_t theApp;

//hard coded
char* serverIp = "12.20.60.2";
int serverPort = 8081;
struct sockaddr_in serverAddr;

TcpServerAppOptions_t appOptions = {  .maxEvents = 1000
                                    , .maxActiveSessions = 1000
                                    , .maxErrorSessions = 1000
                                    , .csDataLen = 1500
                                    , .serverCount = 1};
//hard coded



void InitSession(TcpServerSession_t* newSess) {

    TcpServerConnection_t* newConn = &newSess->tcConn; 
    
    SSInit(newConn);

    newConn->socketFd = 0;
    newConn->tcSess = newSess;
    newConn->bytesReceived = 0;
}

TcpServerSession_t* GetFreeSession() {

    TcpServerSession_t* newSess 
        = GetSesionFromPool (theApp.freeSessionPool);

    if (newSess != NULL) {
        SetSessionToPool (theApp.activeSessionPool, newSess);
        InitSession(newSess);
    }

    return newSess;
}

void SetFreeSession(TcpServerSession_t* newSess) {
    
    RemoveFromSessionPool (theApp.activeSessionPool, newSess);
    SetSessionToPool (theApp.freeSessionPool, newSess);
}


void InitApp() {

    theApp.freeSessionPool = AllocEmptySessionPool();
    theApp.activeSessionPool = AllocEmptySessionPool();

    for (int i = 0; i < appOptions.maxActiveSessions; i++) {
        TcpServerSession_t* aSession = AllocSession (TcpServerSession_t);
        InitSession (aSession);

        //additional one-time initialization
        aSession->sessionType = SESSION_TYPE_CONNECTION;

        SetSessionToPool (theApp.freeSessionPool, aSession);
    }

    theApp.errorSessionCount = 0;

    theApp.errorSessionArr = CreateArray (TcpServerSession_t
                                , appOptions.maxErrorSessions); 


    theApp.serverSessionArr = CreateArray (TcpServerSession_t
                                , appOptions.serverCount);
     
    theApp.appGroupConnStats = CreateArray (TcpServerConnStats_t
                                , appOptions.serverCount);
    
    for (int i = 0; i < appOptions.serverCount; i++) {
        TcpServerSession_t* aSession = &theApp.serverSessionArr[i];
        InitSession (aSession);

        //additional one-time initialization
        aSession->sessionType = SESSION_TYPE_LISTENER;
    }

    theApp.EventArr = CreateEventArray(appOptions.maxEvents);

    memset(&theApp.appConnStats, 0, sizeof (TcpServerConnStats_t));

    theApp.eventQ = CreateEventQ();

    theApp.readBufLen = 2000;
    theApp.readBuffer = malloc(theApp.readBufLen);
}

void CleanupApp() {
    DeleteEventQ(theApp.eventQ);

//    DumpCStats(&theApp.appConnStats);

    // DumpErrSessions();
}

int main(int argc, char** argv)
{
    InitApp();

    TcpServerConnStats_t* appConnStats = &theApp.appConnStats;
    TcpServerAppStats_t* appStats = &theApp.appStats;

    //hard coded
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIp, &(serverAddr.sin_addr));
    //hard coded

    TcpServerSession_t* srvSess = &theApp.serverSessionArr[0];
    TcpServerConnStats_t* groupConnStats = &theApp.appGroupConnStats[0];
    srvSess->tcConn.socketFd = TcpListenStart((struct sockaddr*) &serverAddr
                                , 1000
                                , appConnStats
                                , groupConnStats
                                , &srvSess->tcConn);

    RegisterForReadEvent(theApp.eventQ
                            , srvSess->tcConn.socketFd
                            , &srvSess->tcConn);
    while (true) {

        int eCount = GetIOEvents(theApp.eventQ, theApp.EventArr
                                , appOptions.maxEvents);

        if (eCount > 0){
            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                TcpServerConnection_t* newConn  
                            = (TcpServerConnection_t*)
                                GetIOEventData(theApp.EventArr[eIndex]);
                
                if (newConn->tcSess->sessionType == SESSION_TYPE_LISTENER){

                    if (IsReadEventSet(theApp.EventArr[eIndex])) {

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

                                appStats->dbgNoFreeSession++;

                                close(newSocketFd);      
                            }else {

                                newSess->tcConn.socketFd = newSocketFd;

                                RegisterForReadEvent(theApp.eventQ
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
                                    , theApp.readBuffer
                                    , theApp.readBufLen
                                    , appConnStats
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