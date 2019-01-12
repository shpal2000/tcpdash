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

#include "iovents/apps/tls_sample_client.h"
#include "iovents/apps/tls_sample_server.h"

#include "apps/common.h"

#define APP_MAX_EVENTS 1000
#define APP_MAX_LISTENQ_LENGTH 10000

#define TCP_CLIENT_MAX_EVENTS APP_MAX_EVENTS

#define TCP_SERVER_MAX_EVENTS APP_MAX_EVENTS
#define TCP_SERVER_MAX_LISTENQ_LENGTH APP_MAX_LISTENQ_LENGTH

typedef void (*pAppRunFunc_t) (AppI_t*);
typedef void (*pDumpStatsFunc_t) (AppI_t*);

void RunMain (pAppRunFunc_t pAppRun 
                , pDumpStatsFunc_t pDumpStats
                , AppI_t* appI
                , int isDebug) {

    if (isDebug) {

        (*pAppRun) (appI);

    } else {

        int forkPid = fork();

        if (forkPid < 0) {
            exit(-1);
        }

        appI->isRunning = 1;

        if (forkPid == 0) {

            (*pAppRun) (appI);

        }else{

            while (appI->isRunning) {

                sleep(2);

                (*pDumpStats) (appI);
            }

            int status;

            wait(&status);
        }
    }       
}

void TlsSampleClientMain () {
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
    int dstPort = 444;

    int csGroupClientAddrCountArr[1] = {19};
    TlsSampleClient_t* TcpClientI 
        = CreateTlsSampleClientInterface(1, csGroupClientAddrCountArr); 

    TcpClientI->isRunning = 1; 
    TcpClientI->maxEvents = TCP_CLIENT_MAX_EVENTS;
    TcpClientI->maxActiveSessions = 10000;
    TcpClientI->maxErrorSessions = 100;
    TcpClientI->maxSessions = 1000000;
    TcpClientI->connectionPerSec = 20000;
    TcpClientI->csDataLen = 70000;
    TcpClientI->scDataLen = 70000;

    for (int gIndex = 0; gIndex < TcpClientI->csGroupCount; gIndex++) {

        TlsSampleClientGroup_t* csGroup 
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

    // TlsSampleClientRun(TcpClientI);
    // return;

    int forkPid = fork();

    if (forkPid < 0) {
        exit(-1);
    }

    if (forkPid == 0) {
        TlsSampleClientRun(TcpClientI);
    }else{
        while (TcpClientI->isRunning) {
            sleep(2);
            DumpTlsSampleClientStats(&TcpClientI->appConnStats); 
        }

        int status;
        wait(&status);
    }    
}

void TlsSampleServerMain() {
    char* dstIpGroups[2] = { "12.20.60.2", "12.20.60.3" };
    int dstPort = 444;

    TlsSampleServer_t* TcpServerI 
        = CreateTlsSampleServerInterface(1); 

    TcpServerI->isRunning = 1; 
    TcpServerI->maxEvents = TCP_CLIENT_MAX_EVENTS;
    TcpServerI->maxActiveSessions = 10000;
    TcpServerI->maxErrorSessions = 100;
    TcpServerI->maxSessions = 500000;
    TcpServerI->connectionPerSec = 900;
    TcpServerI->csDataLen = 70000;
    TcpServerI->scDataLen = 70000;

    for (int gIndex = 0; gIndex < TcpServerI->csGroupCount; gIndex++) {

        TlsSampleServerGroup_t* csGroup 
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

    // TlsSampleServerRun(TcpServerI);
    // return;

    int forkPid = fork();

    if (forkPid < 0) {
        exit(-1);
    }

    if (forkPid == 0) {
        TlsSampleServerRun(TcpServerI);
    }else{
        while (TcpServerI->isRunning) {
            sleep(2);
            DumpTlsSampleServerStats(&TcpServerI->appConnStats); 
        }

        int status;
        wait(&status);
    }    

}

void TcpProxyMain() {

    int csGroupCount = 1;

    TcpProxyAppI_t* appI 
        = (TcpProxyAppI_t*) mmap(NULL
            , sizeof (TcpProxyAppI_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    appI->csGroupCount = csGroupCount;
    appI->csGroupArr 
        = (TcpProxyAppGroup_t*) mmap(NULL
            , sizeof (TcpProxyAppGroup_t) * appI->csGroupCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    appI->maxActiveSessions = 100000;
    appI->maxErrorSessions = 100;

    TcpProxyAppGroup_t* server 
        = &appI->csGroupArr[0];

    server->keepSourcePort = 0;

    // proxy address
    struct sockaddr_in* serverAddrP 
        = &(server->serverAddrP.inAddr);
    memset(serverAddrP, 0, sizeof(SockAddr_t));
    serverAddrP->sin_family = AF_INET;
    inet_pton(AF_INET
                , "0.0.0.0"
                , &(serverAddrP->sin_addr));
    serverAddrP->sin_port = htons(883);

    RunMain (&TcpProxyRun, &DumpTcpProxyStats, (AppI_t*) appI, 0);
}

void TcpCSMain (int isServer) {

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

    TcpCsAppI_t* appI 
        = (TcpCsAppI_t*) mmap(NULL
            , sizeof (TcpCsAppI_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    appI->csGroupCount = csGroupCount;
    appI->csGroupArr 
        = (TcpCsAppGroup_t*) mmap(NULL
            , sizeof (TcpCsAppGroup_t) * appI->csGroupCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);
    
    appI->nextCsGroupIndex = 0;
    for (int gIndex = 0; gIndex < appI->csGroupCount; gIndex++) {
        TcpCsAppGroup_t* csGroup = &appI->csGroupArr[gIndex];
        csGroup->clientAddrCount = csGroupClientAddrCountArr[gIndex];
        csGroup->nextClientAddrIndex = 0;
        csGroup->clientAddrArr
            = (SockAddr_t*) mmap(NULL
                , sizeof (SockAddr_t) * csGroup->clientAddrCount
                , PROT_READ | PROT_WRITE
                , MAP_SHARED | MAP_ANONYMOUS
                , -1
                , 0);
        csGroup->LocalPortPoolArr 
            = (LocalPortPool_t*) mmap(NULL
                , sizeof (LocalPortPool_t) * csGroup->clientAddrCount
                , PROT_READ | PROT_WRITE
                , MAP_SHARED | MAP_ANONYMOUS
                , -1
                , 0);
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
        csGroup->sCloseMethod = EmTcpFIN;
        csGroup->csCloseType = EmDataFinish;
        csGroup->csWeight = 1;  
    }

    appI->maxEvents = 0;
    appI->connPerSec = 2000;
    appI->maxActSessions = 100000;
    appI->maxErrSessions = 10000;
    appI->maxSessions = 110000;

    if (isServer) {
        RunMain (&TcpServerRun, &DumpTcpServerStats, (AppI_t*) appI, 0);
    } else {
        RunMain (&TcpClientRun, &DumpTcpClientStats, (AppI_t*) appI, 0);
    }
}

int main(int argc, char** argv)
{
    if (strcmp (argv[1], "TlsSampleClient") == 0) {
        TlsSampleClientMain();
    } else if (strcmp (argv[1], "TlsSampleServer") == 0) {
        TlsSampleServerMain();
    } else if (strcmp (argv[1], "TcpProxy") == 0) {
        TcpProxyMain();
    } else if (strcmp (argv[1], "TcpClient") == 0) {
        TcpCSMain(0);
    } else if (strcmp (argv[1], "TcpServer") == 0) {
        TcpCSMain(1);
    }

    return 0;
}
