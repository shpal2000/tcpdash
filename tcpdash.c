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

#include "apps/tcp/tcp_apps.h"
#include "utils/resource.h"

#define APP_MAX_EVENTS 1000
#define TCP_CLIENT_APP_MAX_EVENTS APP_MAX_EVENTS

int main(int argc, char** argv)
{
    uint32_t csGroupCount = 1;
    TcpClientAppOptions_t* options 
            = (TcpClientAppOptions_t*) mmap(NULL
                , sizeof (TcpClientAppOptions_t) 
                    + sizeof (TcpClientAppConnGroup_t) * csGroupCount
                , PROT_READ | PROT_WRITE
                , MAP_SHARED | MAP_ANONYMOUS
                , -1
                , 0);
    options->isRunning = 1; 
    options->maxEvents = TCP_CLIENT_APP_MAX_EVENTS;
    options->maxActiveSessions = 100000;
    options->maxErrorSessions = 1000;
    options->maxSessions = 100000;
    options->connectionPerSec = 35000;
    options->csDataLen = 1500;
    options->csGroupCount = csGroupCount;

    int forkPid = fork();

    if (forkPid < 0) {
        exit(-1);
    }

    if (forkPid == 0) {
        TcpClientRun(options);
    }else{
        while (options->isRunning) {
            sleep(2);
        }

        int status;
        wait(&status);
    }

    return 0;
}
