#include <stdio.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <gmodule.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

#include "tcpdash.h"

#include "apps/tcp/tcp_client.h"
#include "apps/tcp/tcp_server.h"

#define APP_MAX_EVENTS 1000
#define APP_MAX_LISTENQ_LENGTH 10000

#define TCP_CLIENT_MAX_EVENTS APP_MAX_EVENTS

#define TCP_SERVER_MAX_EVENTS APP_MAX_EVENTS
#define TCP_SERVER_MAX_LISTENQ_LENGTH APP_MAX_LISTENQ_LENGTH


void TcpClientMain(){
        char* srcIpGroup1[] = {"12.20.50.2"
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
                    , "12.20.50.20"};

        char* srcIpGroup2 [] = {"12.20.50.12"
                    , "12.20.50.13"
                    , "12.20.50.14"
                    , "12.20.50.15"
                    , "12.20.50.16"
                    , "12.20.50.17"
                    , "12.20.50.18"
                    , "12.20.50.19"
                    , "12.20.50.20"};

        char** srcIpGroups[2];
        srcIpGroups[0] = srcIpGroup1;
        srcIpGroups[1] = srcIpGroup2;
        

    // char* dstIpGroups[2] = { "12.20.60.2", "12.20.60.3" };
    char* dstIpGroups[1] = { "12.20.60.2"};
    int dstPort = 8081;

    int csGroupClientAddrCountArr[1] = {19};
    TcpClientInterface_t* TcpClientI 
        = CreateTcpClientInterface(1, csGroupClientAddrCountArr); 

    TcpClientI->isRunning = 1; 
    TcpClientI->maxEvents = TCP_CLIENT_MAX_EVENTS;
    TcpClientI->maxActiveSessions = 100000;
    TcpClientI->maxErrorSessions = 40;
    TcpClientI->maxSessions = 1000000;
    TcpClientI->connectionPerSec = 33000;
    TcpClientI->csDataLen = 5000;
    TcpClientI->scDataLen = 5000;

    for (int gIndex = 0; gIndex < TcpClientI->csGroupCount; gIndex++) {

        TcpClientConnGroup_t* csGroup 
            = &TcpClientI->csGroupArr[gIndex];

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
    }

    // TcpClientRun(TcpClientI);

    int forkPid = fork();

    if (forkPid < 0) {
        exit(-1);
    }

    if (forkPid == 0) {
        TcpClientRun(TcpClientI);
    }else{
        while (TcpClientI->isRunning) {
            sleep(2);
            DumpTcpClientStats(&TcpClientI->appConnStats); 
        }

        int status;
        wait(&status);
    }
}

void TcpServerMain() {
    char* dstIpGroups[2] = { "12.20.60.2", "12.20.60.3" };
    int dstPort = 8081;

    TcpServerInterface_t* TcpServerI 
        = CreateTcpServerInterface(1); 

    TcpServerI->isRunning = 1; 
    TcpServerI->maxEvents = TCP_SERVER_MAX_EVENTS;
    TcpServerI->listenQLen = TCP_SERVER_MAX_LISTENQ_LENGTH;
    TcpServerI->maxActiveSessions = 100000;
    TcpServerI->maxErrorSessions = 40;
    TcpServerI->csDataLen = 5000;
    TcpServerI->scDataLen = 5000;

    for (int gIndex = 0; gIndex < TcpServerI->csGroupCount; gIndex++) {

        TcpServerConnGroup_t* csGroup 
            = &TcpServerI->csGroupArr[gIndex];

        struct sockaddr_in* remoteAddr 
            = &(csGroup->serverAddr.inAddr);

        memset(remoteAddr, 0, sizeof(SockAddr_t));

        remoteAddr->sin_family = AF_INET;
        inet_pton(AF_INET
                    , dstIpGroups[gIndex]
                    , &(remoteAddr->sin_addr));

        remoteAddr->sin_port = htons(dstPort);
    }

    TcpServerRun(TcpServerI);
}

int main(int argc, char** argv)
{
    if (strcmp (argv[1], "TcpClient") == 0){
        TcpClientMain();
    }else if (strcmp (argv[1], "TcpServer") == 0) {
        TcpServerMain();
    }

    return 0;
}
