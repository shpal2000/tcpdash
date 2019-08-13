const express = require('express');
const http = require('http');
const net = require('net');
// const plainTextParser = require('plaintextparser');

const host = '192.168.1.24';
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
    "maxSessions" : 1,
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
            "csDataLen" : 7000,
            "scDataLen" : 7000,
            "csStartTlsLen" : 2000,
            "scStartTlsLen" : 2000
        }
    ]
}`;

const TcpCsConfig2 = `{
    "Id" : ${N_ADMIN_MESSAGE_ID_SET_TEST_CONFIG},
    "connPerSec" : 500,
    "maxActSessions" : 20000,
    "maxErrSessions" : 20000,
    "maxSessions" : 8000000,
    "csGroupArr" : [
        {
            "clientAddrArr" : [ "12.20.50.32"
                , "12.20.50.33"
                , "12.20.50.34"
                , "12.20.50.35"
                , "12.20.50.36"
                , "12.20.50.37"
                , "12.20.50.38"
                , "12.20.50.39"
                , "12.20.50.40"
                , "12.20.50.41"
                , "12.20.50.42"
                , "12.20.50.43"
                , "12.20.50.44"
                , "12.20.50.45"
                , "12.20.50.46"
                , "12.20.50.47"
                , "12.20.50.48"
                , "12.20.50.49"
                , "12.20.50.50"
                , "12.20.50.51"
                , "12.20.50.52"
                , "12.20.50.53"
                , "12.20.50.54"
                , "12.20.50.55"
                , "12.20.50.56"
                , "12.20.50.57"
                , "12.20.50.58"
                , "12.20.50.59"
                , "12.20.50.60"
                , "12.20.50.61"
            ],
        
            "serverAddr" : "12.20.60.3",
            "csDataLen" : 7000,
            "scDataLen" : 7000,
            "csStartTlsLen" : 2000,
            "scStartTlsLen" : 2000
        }
    ]
}`;

const TcpCsConfig3 = `{
    "Id" : ${N_ADMIN_MESSAGE_ID_SET_TEST_CONFIG},
    "connPerSec" : 500,
    "maxActSessions" : 20000,
    "maxErrSessions" : 20000,
    "maxSessions" : 100,
    "csGroupArr" : [
        {
            "clientAddrArr" : [ "12.20.50.62"
                , "12.20.50.63"
                , "12.20.50.64"
                , "12.20.50.65"
                , "12.20.50.66"
                , "12.20.50.67"
                , "12.20.50.68"
                , "12.20.50.69"
                , "12.20.50.70"
                , "12.20.50.71"
                , "12.20.50.72"
                , "12.20.50.73"
                , "12.20.50.74"
                , "12.20.50.75"
                , "12.20.50.76"
                , "12.20.50.77"
                , "12.20.50.78"
                , "12.20.50.79"
                , "12.20.50.80"
                , "12.20.50.81"
                , "12.20.50.82"
                , "12.20.50.83"
                , "12.20.50.84"
                , "12.20.50.85"
                , "12.20.50.86"
                , "12.20.50.87"
                , "12.20.50.88"
                , "12.20.50.89"
                , "12.20.50.90"
                , "12.20.50.91"
            ],
        
            "serverAddr" : "12.20.60.4",
            "csDataLen" : 7000,
            "scDataLen" : 7000,
            "csStartTlsLen" : 2000,
            "scStartTlsLen" : 2000
        }
    ]
}`;

const TcpCsConfig4 = `{
    "Id" : ${N_ADMIN_MESSAGE_ID_SET_TEST_CONFIG},
    "connPerSec" : 500,
    "maxActSessions" : 20000,
    "maxErrSessions" : 20000,
    "maxSessions" : 100,
    "csGroupArr" : [
        {
            "clientAddrArr" : [ "12.20.50.92"
                , "12.20.50.93"
                , "12.20.50.94"
                , "12.20.50.95"
                , "12.20.50.96"
                , "12.20.50.97"
                , "12.20.50.98"
                , "12.20.50.99"
                , "12.20.50.100"
                , "12.20.50.101"
                , "12.20.50.102"
                , "12.20.50.103"
                , "12.20.50.104"
                , "12.20.50.105"
                , "12.20.50.106"
                , "12.20.50.107"
                , "12.20.50.108"
                , "12.20.50.109"
                , "12.20.50.110"
                , "12.20.50.111"
                , "12.20.50.112"
                , "12.20.50.113"
                , "12.20.50.114"
                , "12.20.50.115"
                , "12.20.50.116"
                , "12.20.50.117"
                , "12.20.50.118"
                , "12.20.50.119"
                , "12.20.50.120"
                , "12.20.50.121"
            ],
        
            "serverAddr" : "12.20.60.5",
            "csDataLen" : 7000,
            "scDataLen" : 7000,
            "csStartTlsLen" : 2000,
            "scStartTlsLen" : 2000
        }
    ]
}`;

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

const config7 = `{
    "Id" : ${N_ADMIN_MESSAGE_ID_SET_TEST_CONFIG},
    "connPerSec" : 500,
    "maxActSessions" : 20000,
    "maxErrSessions" : 20000,
    "maxSessions" : 100,
    "csGroupArr" : [
        {
            "clientAddrArr" : [ "12.20.50.92"
                , "12.20.50.93"
                , "12.20.50.94"
                , "12.20.50.95"
                , "12.20.50.96"
                , "12.20.50.97"
                , "12.20.50.98"
                , "12.20.50.99"
                , "12.20.50.100"
                , "12.20.50.101"
                , "12.20.50.102"
                , "12.20.50.103"
                , "12.20.50.104"
                , "12.20.50.105"
                , "12.20.50.106"
                , "12.20.50.107"
                , "12.20.50.108"
                , "12.20.50.109"
                , "12.20.50.110"
                , "12.20.50.111"
                , "12.20.50.112"
                , "12.20.50.113"
                , "12.20.50.114"
                , "12.20.50.115"
                , "12.20.50.116"
                , "12.20.50.117"
                , "12.20.50.118"
                , "12.20.50.119"
                , "12.20.50.120"
                , "12.20.50.121"
            ],
        
            "serverAddr" : "192.168.1.24",
            "csDataLen" : 10,
            "scDataLen" : 10,
            "csStartTlsLen" : 0,
            "scStartTlsLen" : 0
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
        case 'test5':
            sendMsg = TcpCsConfig5;
            break;
        case 'test6':
            sendMsg = config6;
            break;
        case 'test7':
            sendMsg = config7;
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