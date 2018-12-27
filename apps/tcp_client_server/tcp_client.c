#include <sys/mman.h>
#include "iovents.h"

#define __APP__MAIN__
#include "tcp_client.h"


static void InitRwBuff () {
}

static void InitConn () {
}

static void InitSession () {
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

void CreateTcpClientInterface(int csGroupCount
                                            , int* clientAddrCounts) {

}

void DeleteTlsSampleClientInterface (TlsSampleClient_t* iFace){
}

void DumpTcpClientStats() {
}



