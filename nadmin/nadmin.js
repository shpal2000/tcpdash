const express = require('express');
const http = require('http');
const net = require('net');
const fs = require('fs');

// const plainTextParser = require('plaintextparser');

const host = process.argv[2];
const adminPort = 8777;
const msgIoPort = 9777;

const msgIoServer = net.createServer();
const msgIoConns = new Map();

const MSG_IO_MESSAGEL_LENGTH_BYTES = 8;
const MSG_IO_READ_WRITE_DATA_MAXLEN = 1048576; 

const config6 = `{
    "cApps" : {
        "appList" : [
            {
                "appName" : "TlsClient",
                "connPerSec" : 500,
                "maxActSess" : 10000,
                "maxErrSess" : 10000,
                "maxSess" : 100,
                "csGrpArr" : [
                    {
                        "srvIp"   : "12.20.60.1",
                        "srvPort" : 8443,
                        "cAddrArr"  : [
                            {
                                "cIp" : "12.20.50.1",
                                "cPortB" : 10000,
                                "cPortE" : 19999 
                            },
                            {
                                "cIp" : "12.20.50.2",
                                "cPortB" : 20000,
                                "cPortE" : 29999 
                            }
                        ],
                        "csDataLen" : 10,
                        "scDataLen" : 20,
                        "csStartTlsLen" : 0,
                        "scStartTlsLen" : 0
                    }
                ]
            }
       ]
    },

    "sApps" : {
       "appList" : [
            {
                "appName" : "TlsServer",
                "maxActSess" : 10000,
                "maxErrSess" : 10000,
                "csGrpArr" : [
                    {
                        "srvIp"   : "12.20.60.1",
                        "srvPort" : 8443,
                        "csDataLen" : 10,
                        "scDataLen" : 20,
                        "csStartTlsLen" : 0,
                        "scStartTlsLen" : 0
                    }
                ]
            }
        ]
    }
}`;

msgIoServer.listen(msgIoPort, host, () => {
    console.log('msgIoServer running on port ' + msgIoPort + '.');
});

function msgIoHandler (sock, sockCtx, rcvMsg) {
    if (rcvMsg.startsWith('test')) {
        var sendMsg = '';
        sendMsg = fs.readFileSync(__dirname + '/configs/' + rcvMsg.trim());

        if (sendMsg) {
            var s1 = "0000000" + sendMsg.length;
            var s2 = s1.substr(s1.length-7);
            sock.write ( s2 + '\n' + sendMsg);
        }
    } else {
        console.log (rcvMsg);
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