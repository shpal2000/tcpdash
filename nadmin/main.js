const express = require('express');
const http = require('http');
const net = require('net');
// const plainTextParser = require('plaintextparser');

const host = '10.116.6.3';
const adminPort = 8777;
const msgIoPort = 9777;

const msgIoServer = net.createServer();
const msgIoConns = new Map();

const MSG_IO_MESSAGEL_LENGTH_BYTES = 8;
const MSG_IO_READ_WRITE_DATA_MAXLEN = 1048576; 

const N_ADMIN_MESSAGE_ID_GET_TEST_CONFIG = 1;
const N_ADMIN_MESSAGE_ID_SET_TEST_CONFIG = 2;

const TcpCsConfig = `{
    "Id" : ${N_ADMIN_MESSAGE_ID_SET_TEST_CONFIG},
    "connPerSec" : 2000,
    "maxActSessions" : 1000,
    "maxErrSessions" : 1000,
    "maxSessions" : 10,
    "csGroupCount" : 1,
    "csGroupArr" : [
        {
            "clientAddrCount" : 30,
            "clientAddrArr" : [ "12.20.50.2"
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
                , "12.20.50.31"
            ],
        
            "serverAddr" : "12.20.60.2",
            "csDataLen" : 1000,
            "scDataLen" : 1000,
            "cCloseMethod" : 1,
            "sCloseMethod" : 1,
            "csCloseType" : 3,
            "csWeight" : 1 
        }
    ]
}`;


msgIoServer.listen(msgIoPort, host, () => {
    console.log('msgIoServer running on port ' + msgIoPort + '.');
});

function msgIoHandler (sock, sockCtx, rcvMsg) {
    console.log (rcvMsg);
    // console.log (rcvMsg);
    // var sendMsg = TcpCsConfig;
    // var s1 = "0000000" + TcpCsConfig.length;
    // var s2 = s1.substr(s1.length-7);
    // sock.write ( s2 + '\n' + sendMsg);
}

msgIoServer.on('connection', (sock) => {
    
    var sockCtx = { msgBuff : ''
                    , msgLen : NaN
                    , msgData : '' };

    msgIoConns.set (sock, sockCtx);

    sock.on('data', (data) => {

        var sockCtx = msgIoConns.get(sock);

        sockCtx.msgBuff = sockCtx.msgBuff.concat (data);

        while (true) {

            if ( isNaN(sockCtx.msgLen) ) {
                    
                if (sockCtx.msgBuff.length > MSG_IO_MESSAGEL_LENGTH_BYTES) {

                    sockCtx.msgLen 
                        = parseInt(sockCtx.msgBuff.substring(0
                                    , MSG_IO_MESSAGEL_LENGTH_BYTES));

                    if (isNaN(sockCtx.msgLen)
                            || sockCtx.msgLen < 0
                            || sockCtx.msgLen > MSG_IO_READ_WRITE_DATA_MAXLEN) {
                        sock.destroy ();
                    } else {

                        sockCtx.msgBuff 
                            = sockCtx.msgBuff.substring(MSG_IO_MESSAGEL_LENGTH_BYTES);
                    }
                } else {
                    break;
                }
            }

            if (isNaN(sockCtx.msgLen)) {
                break;
            } else {

                if (sockCtx.msgBuff.length >= sockCtx.msgLen) {

                    sockCtx.msgData 
                        = sockCtx.msgBuff.substring(0, sockCtx.msgLen);

                    sockCtx.msgBuff 
                    = sockCtx.msgBuff.substring(sockCtx.msgLen);
                    
                    //handle message
                    msgIoHandler (sock, sockCtx, sockCtx.msgData);

                    //prepare for next message
                    sockCtx.msgLen = NaN;
                    sockCtx.msgData = ''
                } else {
                    break;
                }            
            }
        }
    });

    sock.on('end', () => {
        sock.end ();
    });

    sock.on('error', (err) => {
        console.log('ERROR: ' + sock.remoteAddress + ':' + sock.remotePort + ':' + err);
    });

    sock.on('close', () => {
        msgIoConns.delete(sock);
    });
});

const app = express();
// app.use(plainTextParser);

app.get('/', (req, res) => {
    res.end('Hello GETHello GETHello GETHello GETHello GETHello GETHello GETHello GETHello GETHello GETHello GET');
 });

const adminServer = http.createServer(app);
adminServer.listen(adminPort, host, () => {
    console.log('adminServer running on port ' + adminPort + '.');
});