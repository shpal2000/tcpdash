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
    char* srcIp = "12.20.50.1";
    char* dstIp = "12.20.60.1";
    int dstPort = 8081;

    int clientAddrCounts[1] = {1};
    TcpClientAppInterface_t* tcpClientAppI 
        = CreateTcpClienAppInterface(1, clientAddrCounts); 

    tcpClientAppI->isRunning = 1; 
    tcpClientAppI->maxEvents = TCP_CLIENT_APP_MAX_EVENTS;
    tcpClientAppI->maxActiveSessions = 100000;
    tcpClientAppI->maxErrorSessions = 40;
    tcpClientAppI->maxSessions = 100000;
    tcpClientAppI->connectionPerSec = 33000;
    tcpClientAppI->csDataLen = 1500;

    struct sockaddr_in* localAddr 
        = &(tcpClientAppI->csGroupArr[0].clientAddrArr[0].inAddr);

    memset(localAddr, 0, sizeof(SockAddr_t));

    localAddr->sin_family = AF_INET;

    inet_pton(AF_INET
                , srcIp
                , &(localAddr->sin_addr)); 

    struct sockaddr_in* remoteAddr 
        = &(tcpClientAppI->csGroupArr[0].serverAddr.inAddr);

    memset(remoteAddr, 0, sizeof(SockAddr_t));

    remoteAddr->sin_family = AF_INET;

    inet_pton(AF_INET
                , dstIp
                , &(remoteAddr->sin_addr));

    remoteAddr->sin_port = htons(dstPort);

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
        }

        int status;
        wait(&status);
    }

    return 0;
}
