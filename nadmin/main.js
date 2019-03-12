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

const TcpCsConfig1 = `{
    "Id" : ${N_ADMIN_MESSAGE_ID_SET_TEST_CONFIG},
    "connPerSec" : 500,
    "maxActSessions" : 20000,
    "maxErrSessions" : 20000,
    "maxSessions" : 10000000,
    "csGroupArr" : [
        {
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

const TcpCsConfig2 = `{
    "Id" : ${N_ADMIN_MESSAGE_ID_SET_TEST_CONFIG},
    "connPerSec" : 500,
    "maxActSessions" : 20000,
    "maxErrSessions" : 20000,
    "maxSessions" : 10000000,
    "csGroupArr" : [
        {
            "clientAddrArr" : [ "12.20.51.2"
                , "12.20.51.3"
                , "12.20.51.4"
                , "12.20.51.5"
                , "12.20.51.6"
                , "12.20.51.7"
                , "12.20.51.8"
                , "12.20.51.9"
                , "12.20.51.10"
                , "12.20.51.11"
                , "12.20.51.12"
                , "12.20.51.13"
                , "12.20.51.14"
                , "12.20.51.15"
                , "12.20.51.16"
                , "12.20.51.17"
                , "12.20.51.18"
                , "12.20.51.19"
                , "12.20.51.20"
                , "12.20.51.21"
                , "12.20.51.22"
                , "12.20.51.23"
                , "12.20.51.24"
                , "12.20.51.25"
                , "12.20.51.26"
                , "12.20.51.27"
                , "12.20.51.28"
                , "12.20.51.29"
                , "12.20.51.30"
                , "12.20.51.31"
            ],
        
            "serverAddr" : "12.20.61.2",
            "csDataLen" : 1000,
            "scDataLen" : 1000,
            "cCloseMethod" : 1,
            "sCloseMethod" : 1,
            "csCloseType" : 3,
            "csWeight" : 1 
        }
    ]
}`;

const TcpCsConfig3 = `{
    "Id" : ${N_ADMIN_MESSAGE_ID_SET_TEST_CONFIG},
    "connPerSec" : 500,
    "maxActSessions" : 20000,
    "maxErrSessions" : 20000,
    "maxSessions" : 10000000,
    "csGroupArr" : [
        {
            "clientAddrArr" : [ "12.20.52.2"
                , "12.20.52.3"
                , "12.20.52.4"
                , "12.20.52.5"
                , "12.20.52.6"
                , "12.20.52.7"
                , "12.20.52.8"
                , "12.20.52.9"
                , "12.20.52.10"
                , "12.20.52.11"
                , "12.20.52.12"
                , "12.20.52.13"
                , "12.20.52.14"
                , "12.20.52.15"
                , "12.20.52.16"
                , "12.20.52.17"
                , "12.20.52.18"
                , "12.20.52.19"
                , "12.20.52.20"
                , "12.20.52.21"
                , "12.20.52.22"
                , "12.20.52.23"
                , "12.20.52.24"
                , "12.20.52.25"
                , "12.20.52.26"
                , "12.20.52.27"
                , "12.20.52.28"
                , "12.20.52.29"
                , "12.20.52.30"
                , "12.20.52.31"
            ],
        
            "serverAddr" : "12.20.62.2",
            "csDataLen" : 1000,
            "scDataLen" : 1000,
            "cCloseMethod" : 1,
            "sCloseMethod" : 1,
            "csCloseType" : 3,
            "csWeight" : 1 
        }
    ]
}`;

const TcpCsConfig4 = `{
    "Id" : ${N_ADMIN_MESSAGE_ID_SET_TEST_CONFIG},
    "connPerSec" : 500,
    "maxActSessions" : 20000,
    "maxErrSessions" : 20000,
    "maxSessions" : 10000000,
    "csGroupArr" : [
        {
            "clientAddrArr" : [ "12.20.53.2"
                , "12.20.53.3"
                , "12.20.53.4"
                , "12.20.53.5"
                , "12.20.53.6"
                , "12.20.53.7"
                , "12.20.53.8"
                , "12.20.53.9"
                , "12.20.53.10"
                , "12.20.53.11"
                , "12.20.53.12"
                , "12.20.53.13"
                , "12.20.53.14"
                , "12.20.53.15"
                , "12.20.53.16"
                , "12.20.53.17"
                , "12.20.53.18"
                , "12.20.53.19"
                , "12.20.53.20"
                , "12.20.53.21"
                , "12.20.53.22"
                , "12.20.53.23"
                , "12.20.53.24"
                , "12.20.53.25"
                , "12.20.53.26"
                , "12.20.53.27"
                , "12.20.53.28"
                , "12.20.53.29"
                , "12.20.53.30"
                , "12.20.50.31"
            ],
        
            "serverAddr" : "12.20.63.2",
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
   
    var sendMsg = '';
    switch (rcvMsg.trim()) {
        case 'test1':
            sendMsg = TcpCsConfig1;
            break;
        case 'test2':
            sendMsg = TcpCsConfig2;
            break;
        case 'test3':
            sendMsg = TcpCsConfig3;
            break;
        case 'test4':
            sendMsg = TcpCsConfig4;
            break;
    }

    if (sendMsg) {
        var s1 = "0000000" + sendMsg.length;
        var s2 = s1.substr(s1.length-7);
        sock.write ( s2 + '\n' + sendMsg);
    }
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