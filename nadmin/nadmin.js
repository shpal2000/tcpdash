const express = require('express');
const http = require('http');
const net = require('net');
const fs = require('fs');
const execSync = require('child_process').execSync;

const host = process.argv[2];
const userName = process.argv[3];
const passWord = process.argv[4];

const adminPort = 8777;
const msgIoPort = 9777;

const msgIoServer = net.createServer();
const msgIoConns = new Map();

const MSG_IO_MESSAGEL_LENGTH_BYTES = 8;
const MSG_IO_READ_WRITE_DATA_MAXLEN = 1048576; 
const MSG_ID_APP_START = 1;
const MSG_ID_RESERVED = 100;

function hostCommand (cmd) {
    var cmd1 = 'echo \"' + passWord+ '\"' + ' | sshpass -p ' + '\"' + passWord + '\"' + ' ssh -tt -o StrictHostKeyChecking=no ' + userName + '@'+ host + ' \"' + cmd + '\"';
    execSync (cmd1);
}

msgIoServer.listen(msgIoPort, host, () => {

    hostCommand ('sudo ~/nadmin/tools/doc_brsetup.sh eth1');

    hostCommand ('sudo ~/nadmin/tools/doc_brsetup.sh eth2');

    console.log('msgIoServer running on port ' + msgIoPort + '.');
});

function msgIoHandler (sock, sockCtx, rcvMsgStr) {
    console.log (rcvMsgStr);

    try {
        var rcvMsg = JSON.parse(rcvMsgStr);
        
        switch (rcvMsg.MessageType) {
            case "AppStart":
                var appInfo = rcvMsg.Message; 
                var appCfgStr  = fs.readFileSync(__dirname + '/configs/' + appInfo.cfgId);
                var appCfgObjAll = JSON.parse (appCfgStr);
                var appCfgObj = appCfgObjAll[appInfo.cfgSelect];
                hostCommand ('sudo ~/nadmin/tools/doc_connect.sh ' + appInfo.docName + ' ' + appCfgObj.port);
                for (var i = 0; i < appCfgObj.subnets.length; i++ ) {
                    hostCommand('sudo ~/nadmin/tools/doc_ipaddr.sh ' + appInfo.docName + ' add ' + appCfgObj.subnets[i]);
                }
                var sndMsg = {};
                sndMsg.MessageType = rcvMsg.MessageType;
                sndMsg.MessageId = rcvMsg.MessageId;
                sndMsg.Message = appCfgObjAll; 
                sndMsg.MessgeStatus = 0;
                var sndMsgStr = JSON.stringify (sndMsg);
                var s1 = "0000000" + sndMsgStr.length;
                var s2 = s1.substr(s1.length-7);
                sock.write ( s2 + '\n' + sndMsgStr);
                console.log (s2 + '\n' + sndMsgStr);
                break;
        }  
    } catch (err) {
        console.log (err.message);
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