#include <sys/mman.h>
#include "iovents.h"

#define __APP__MAIN__
#include "tcp_client_server.h"


static void InitRwBuff (RwBuff_t* newBuff) {

    newBuff->buffLen = RW_MAX_BUFF_LEN;
    newBuff->buffOffset = 0;
    newBuff->dataLen = 0;
}

static void InitConn (TcpClientConn_t * tcpConn) {

    tcpConn->iovConn = NULL;
    tcpConn->readBuff = NULL;
    tcpConn->writeBuff = NULL;
}

static void InitSession (TcpClientSession_t* newSess
                            TcpClientAppCtx_t* appCtx ) {

    newSess->appCtx = appCtx;
    InitConn (&newSess->tcpConn);
}

static void OnEstablish (struct IoVentConn* iovConn) {
}

static void OnWriteNext (struct IoVentConn* iovConn) {
}

static void OnWriteStatus (struct IoVentConn* iovConn
                                    , int bytesWritten) {
}

static void OnReadNext (struct IoVentConn* iovConn) {
}

static void OnReadStatus (struct IoVentConn* iovConn
                                    , int bytesReceived) {

}

static void OnCleanup (struct IoVentConn* iovConn) {
}

static void OnStatus (struct IoVentConn* iovConn) {
}

static void CreateAppCtx () {

}

void TcpClientRun () {
}

void CreateTcpClientServerInterface(int csGroupCount
                                    , int* clientAddrCounts) {

}

void DeleteTcpClientServerInterface (TlsSampleClient_t* iFace){
}

void DumpTcpClientStats() {
}



