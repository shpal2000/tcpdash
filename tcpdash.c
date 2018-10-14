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
#define TCP_CLIENT_APP_MAX_EVENTS APP_MAX_EVENTS

int main(int argc, char** argv)
{
    char* srcIps[] = {"12.20.50.2"
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
    char* dstIp = "12.20.60.2";
    int dstPort = 8081;

    int csGroupClientAddrCountArr[1] = {19};
    TcpClientAppInterface_t* tcpClientAppI 
        = CreateTcpClienAppInterface(1, csGroupClientAddrCountArr); 

    tcpClientAppI->isRunning = 1; 
    tcpClientAppI->maxEvents = TCP_CLIENT_APP_MAX_EVENTS;
    tcpClientAppI->maxActiveSessions = 100000;
    tcpClientAppI->maxErrorSessions = 40;
    tcpClientAppI->maxSessions = 1000000;
    tcpClientAppI->connectionPerSec = 33000;
    tcpClientAppI->csDataLen = 1500;

    for (int gIndex = 0; gIndex < tcpClientAppI->csGroupCount; gIndex++) {
        TcpClientAppConnGroup_t* csGroup 
            = &tcpClientAppI->csGroupArr[gIndex]; 
        for (int cIndex = 0
                ; cIndex < csGroup->clientAddrCount
                ; cIndex++) { 
            struct sockaddr_in* localAddr 
                = &(csGroup->clientAddrArr[cIndex].inAddr);
            memset(localAddr, 0, sizeof(SockAddr_t));
            localAddr->sin_family = AF_INET;
            inet_pton(AF_INET
                        , srcIps[cIndex]
                        , &(localAddr->sin_addr));
            PortBindQ_t* portQ = &csGroup->clientPortBindArr[cIndex];
            for (int srcPort = 5000; srcPort <= 65000; srcPort++) {
                AddToPortBindQ(portQ, htons(srcPort));
            }
        }
        struct sockaddr_in* remoteAddr 
            = &(csGroup->serverAddr.inAddr);
        memset(remoteAddr, 0, sizeof(SockAddr_t));
        remoteAddr->sin_family = AF_INET;
        inet_pton(AF_INET
                    , dstIp
                    , &(remoteAddr->sin_addr));
        remoteAddr->sin_port = htons(dstPort);
    }

    // TcpClienAppRun(tcpClientAppI);

    int forkPid = fork();

    if (forkPid < 0) {
        exit(-1);
    }

    if (forkPid == 0) {
        TcpClienAppRun(tcpClientAppI);
    }else{
        while (tcpClientAppI->isRunning) {
            sleep(2);
            DumpTcpClientAppStats(&tcpClientAppI->appConnStats); 
        }

        int status;
        wait(&status);
    }

    return 0;
}
