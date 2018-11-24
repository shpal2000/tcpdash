#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "iovents.h"

static IoVentApp_t* App;

static void InitConnection (IoVentConn_t* iovConn) {

    CSInit(iovConn);

    iovConn->socketFd = 0;
    iovConn->cSSL = NULL;
    iovConn->savedLocalPort = 0;
    iovConn->savedRemotePort = 0;
    iovConn->localAddress = NULL;
    iovConn->remoteAddress = NULL;
    iovConn->localPortPool = NULL  

    iovConn->statusId = ;
    iovConn->statusData = NULL;

    iovConn->statusResponseId = ;
    iovConn->statusResponseData = NULL;

    iovConn->appData = NULL;   
}

static void InitApp () {

    SSL_load_error_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_ssl_algorithms();
    SSL_library_init();

    App->freeConnectionPool = AllocEmptyPool ();
    App->activeConnectionPool = AllocEmptyPool ();

    for (int i = 0; i < App->options.maxActiveConnections; i++) {

        IoVentConn_t* iovConn = CreateStruct0 (IoVentConn_t);

        InitConnection (iovConn);

        AddToPool (App->freeConnectionPool, iovConn);
    }

} 

static void CleanupApp () {

}

void InitIoVents (IoVentMethods_t* methods
                , IoVentOptions_t* options) {

    App = CreateStruct0 (IoVentApp_t);

    App->methods = *methods;
    App->options = *options;

    InitApp ();
}

void CleanupIoVents () {

    CleanupApp ();
}

