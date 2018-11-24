#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "iovents.h"

static IoVentApp_t* App;

static void InitConnection (IoVentConn_t* newConn) {

    CSInit(newConn);

    newConn->socketFd = 0;
    newConn->cSSL = NULL;
    newConn->savedLocalPort = 0;
    newConn->savedRemotePort = 0;
    newConn->localAddress = NULL;
    newConn->remoteAddress = NULL;
    newConn->localPortPool = NULL  

    newConn->statusId = ;
    newConn->statusData = NULL;

    newConn->statusResponseId = ;
    newConn->statusResponseData = NULL;

    newConn->appData = NULL;   
}

static IoVentConn_t* GetFreeConnection() {

    IoVentConn_t* newConn 
        = GetFromPool (App->freeConnectionPool);

    if (newConn != NULL) {
        AddToPool (App->activeConnectionPool, newConn);
        InitConnection (newConn);
    }

    return newConn;
}

static void SetFreeSession(IoVentConn_t* newConn) {
    
    RemoveFromPool (App->activeConnectionPool, newConn);
    AddToPool (App->freeConnectionPool, newConn);
}

static void StoreErrConnection(IoVentConn_t* newConn) {

    if (App->errorConnectionCount 
                < App->options.maxErrorConnections) {

        IoVentConn_t* errConn 
                = &App->errorConnectionArr[App->errorConnectionCount];

        *errConn =  *newConn;

        errSession->tcConn.savedLocalPort 
            = ntohs(GetSockPort(errSession->tcConn.localAddress));

        errSession->tcConn.savedRemotePort 
            = ntohs(GetSockPort(errSession->tcConn.remoteAddress));

        App->errorSessionCount++;
    }
}

static void InitApp () {

    SSL_load_error_strings();
    ERR_load_crypto_strings();
    OpenSSL_add_ssl_algorithms();
    SSL_library_init();

    App->freeConnectionPool = AllocEmptyPool ();
    App->activeConnectionPool = AllocEmptyPool ();

    for (int i = 0; i < App->options.maxActiveConnections; i++) {

        IoVentConn_t* newConn = CreateStruct (IoVentConn_t);

        InitConnection (newConn);

        AddToPool (App->freeConnectionPool, newConn);
    }

    App->errorConnectionCount = 0;
    
    App->errorConnectionArr 
        = CreateArray (IoVentConn_t, App->options.maxErrorConnections); 

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

