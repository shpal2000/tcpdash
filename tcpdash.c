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

#include <gmodule.h>
#include <glib-object.h>
#include <json-glib/json-glib.h>
#include <json-glib/json-gobject.h>

#include "apps/tcp/tcp_apps.h"

int main(int argc, char** argv)
{
    
    TcpClientAppOptions_t options 
        = {  .maxEvents = 1000
            , .maxActiveSessions = 25000
            , .maxErrorSessions = 1000
            , .maxSessions = 5
            , .connectionPerSec = 2000
            , .csDataLen = 1500};

    TcpClientRun(&options);

    return 0;
}
