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

msgIoServer.listen(msgIoPort, host, () => {
    console.log('msgIoServer running on port ' + msgIoPort + '.');
});

msgIoServer.on('connection', (sock) => {

    console.log('CONNECTED: ' + sock.remoteAddress + ':' + sock.remotePort);
    
    var sockCtx = { msgBuff : ''
                    , msgs : []
                    , state : 'MSG_LENGHT_READ' };

    msgIoConns.set (sock, sockCtx);

    sock.on('data', (data) => {

        console.log('DATA ' + sock.remoteAddress + ':' + sock.remotePort + ':' + data);

        var sockCtx = msgIoConns.get(sock);

        sockCtx.msgBuff = sockCtx.msgBuff.concat (data);

        if (sockCtx.state === 'MSG_LENGHT_READ') {
            console.log('MSG_LENGHT_READ');
            if (sockCtx.msgBuff.length >= MSG_IO_MESSAGEL_LENGTH_BYTES) {
                console.log('MSG_IO_MESSAGEL_LENGTH_BYTES');
                var msgLenStr 
                    = sockCtx.msgBuff.substring(0, MSG_IO_MESSAGEL_LENGTH_BYTES);

                var msgLen = parseInt(msgLenStr);
                console.log('msgLen: ' + msgLen);
                if (isNaN(msgLen)
                        || msgLen < 0
                        || msgLen > MSG_IO_READ_WRITE_DATA_MAXLEN) {
                    console.log('sock.end');
                    sock.destroy ();
                } else {
                    sockCtx.state = MSG_IO_ON_MESSAGE_STATE_READ_DATA; 
                }
            }
        } else {

        }
    });

    sock.on('end', () => {
        console.log('END: ' + sock.remoteAddress + ':' + sock.remotePort);
        sock.end ();
    });

    sock.on('error', (err) => {
        console.log('ERROR: ' + sock.remoteAddress + ':' + sock.remotePort + ':' + err);
    });

    sock.on('close', () => {
        console.log('CLOSED: ' + sock.remoteAddress + ':' + sock.remotePort);
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