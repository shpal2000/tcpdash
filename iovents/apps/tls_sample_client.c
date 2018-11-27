
#include "iovents.h"


static void OnEstablish (IoVentConn_t* iovConn) {

}

static void OnWriteNext (IoVentConn_t* iovConn) {
    
}

static void OnReadNext (IoVentConn_t* iovConn) {
    
}

static void OnCleanup (IoVentConn_t* iovConn) {
    
}

static void OnStatus (IoVentConn_t* iovConn) {

    //todo for more advavanced control
    switch (iovConn->statusId) {
        default:
            break;
    }

}

void TlsSampleClientRun (TlsSampleClient_t* appIface) {

    IoVentMethods_t* iovMethods = CreateStruct0 (IoVentMethods_t);
    iovMethods->OnEstablish = &OnEstablish;
    iovMethods->OnWriteNext = &OnWriteNext;
    iovMethods->OnReadNext = &OnReadNext;
    iovMethods->OnCleanup = &OnCleanup;
    iovMethods->OnStatus = &OnStatus;

    IoVentOptions_t* iovOptions = CreateStruct0 (IoVentOptions_t);
    iovOptions->maxActiveConnections = 100;
    iovOptions->maxErrorConnections = 100;
    
    IoVentCtx_t* iovCtx = CreateIoVentCtx (iovMethods, iovOptions);

    while (1) {
        ProcessIoVent (iovCtx);
    }

    DeleteIoVentCtx (iovCtx);
}




