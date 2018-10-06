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

#define APP_MAX_EVENTS 1000
#define TCP_CLIENT_APP_MAX_EVENTS APP_MAX_EVENTS

int main(int argc, char** argv)
{
    TcpClientAppInterface_t* options 
        = (TcpClientAppInterface_t*) mmap(NULL
            , sizeof (TcpClientAppInterface_t)
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    options->isRunning = 1; 
    options->maxEvents = TCP_CLIENT_APP_MAX_EVENTS;
    options->maxActiveSessions = 100000;
    options->maxErrorSessions = 1000;
    options->maxSessions = 100;
    options->connectionPerSec = 100;
    options->csDataLen = 1500;

    options->csGroupCount = 1;
    options->csGroupArr 
        = (TcpClientAppConnGroup_t*) mmap(NULL
            , sizeof (TcpClientAppConnGroup_t) * options->csGroupCount
            , PROT_READ | PROT_WRITE
            , MAP_SHARED | MAP_ANONYMOUS
            , -1
            , 0);

    for (int i = 0; i < options->csGroupCount; i++) {
        TcpClientAppConnGroup_t* csGroup = &options->csGroupArr[i];
        csGroup->clientAddrCount = 1;
        csGroup->clientAddrArr
            = (struct sockaddr*) mmap(NULL
                , sizeof (struct sockaddr)
                , PROT_READ | PROT_WRITE
                , MAP_SHARED | MAP_ANONYMOUS
                , -1
                , 0);
    }


    TcpClienAppRun(options);

    // int forkPid = fork();

    // if (forkPid < 0) {
    //     exit(-1);
    // }

    // if (forkPid == 0) {
    //     TcpClienAppRun(options);
    // }else{
    //     while (options->isRunning) {
    //         sleep(2);
    //     }

    //     int status;
    //     wait(&status);
    // }

    return 0;
}
