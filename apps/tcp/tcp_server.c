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


#include "sessions/sessions.h"
#include "sessions/tcp_connect.h"
#include "logging/logs.h"
#include "tcp_server.h"

TcpServerApp_t theApp;

//hard coded
char* serverIp = "12.20.60.1";
int serverPort = 8081;
struct sockaddr_in serverAddr;

TcpServerAppOptions_t appOptions = {  .maxEvents = 1000
                                    , .maxActiveSessions = 1000
                                    , .maxErrorSessions = 1000};
//hard coded



void InitSession(TcpServerSession_t* newSess) {

    TcpServerConnection_t* newConn = &newSess->tcConn; 
    
    SSInit(newConn);

    newConn->socketFd = 0;
    newConn->tcSess = newSess;
}

TcpServerSession_t* GetFreeSession() {

    TcpServerSession_t* newSess 
        = GetAnySesionFromPool (theApp.freeSessionPool);

    if (newSess != NULL) {
        AddToSessionPool (theApp.activeSessionPool, newSess);
        InitSession(newSess);
    }

    return newSess;
}

void SetFreeSession(TcpServerSession_t* newSess) {
    
    RemoveFromSessionPool (theApp.activeSessionPool, newSess);
    AddToSessionPool (theApp.freeSessionPool, newSess);
}


void InitApp(TcpServerAppOptions_t* options) {

    theApp.appOptions = *options;

    theApp.freeSessionPool = AllocEmptySessionPool();
    theApp.activeSessionPool = AllocEmptySessionPool();

    for (int i = 0; i < theApp.appOptions.maxActiveSessions; i++) {
        TcpServerSession_t* aSession = AllocSession (TcpServerSession_t);
        InitSession (aSession);
        AddToSessionPool (theApp.freeSessionPool, aSession);
    }

    theApp.errorSessionCount = 0;
    theApp.errorSessionArr = CreateSessionArray (TcpServerSession_t
                                , theApp.appOptions.maxErrorSessions); 

    theApp.EventArr = CreateEventArray(theApp.appOptions.maxEvents);

    memset(&theApp.appConnStats, 0, sizeof (TcpServerConnStats_t));

    theApp.eventQId = CreateEventQ();
}

void CleanupApp() {
    DeleteEventQ(theApp.eventQId);

//    DumpSStats(&theApp.appConnStats);

    // DumpErrSessions();
}

int main(int argc, char** argv)
{
    InitApp(&appOptions);
    TcpServerConnStats_t* appConnStats = &theApp.appConnStats;
    // TcpServerAppStats_t* appStats = &theApp.appStats;

    //hard coded
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverIp, &(serverAddr.sin_addr));
    //hard coded

    TcpServerListener_t serverListener;
    TcpServerConnStats_t* groupConnStats = &theApp.appGroupConnStats[0];
    serverListener.socketFd = TcpListenStart(0
                                , (struct sockaddr*) &serverAddr
                                , 1000
                                , appConnStats
                                , groupConnStats
                                , &serverListener);

    RegisterForReadEvent(theApp.eventQId
                                , serverListener.socketFd
                                , &serverListener);
    while (true) {

        int eCount = GetIOEvents(theApp.eventQId, theApp.EventArr
                                , theApp.appOptions.maxEvents);

        if (eCount > 0){
            for(int eIndex = 0; eIndex < eCount; eIndex++) {

                if (IsReadEventSet(theApp.EventArr[eIndex])) {
                    int newSocketFd = accept(serverListener.socketFd, NULL, NULL);
                    int flags = fcntl(newSocketFd, F_GETFL, 0);
                    fcntl(newSocketFd, F_SETFL, flags | O_NONBLOCK);
                    close (newSocketFd);
                }
            }
        }
    }

    CleanupApp();

    return 0;
}