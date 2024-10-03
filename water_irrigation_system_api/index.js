const express = require('express');
const { createServer } = require('node:http');
const { join } = require('node:path');
const { Server } = require('socket.io');
const { log } = require('./utils/logger');
const { pushStatisticsToDB, updateCurrentStateOnClientSide, prepareDataFromEspForClient } = require('./src/core');
const app = express();
const server = createServer(app);
const io = new Server(server);

const PORT = 8080;

let masterID='';

app.use(express.json());
app.use((req, res, next) => {
    log("INFO", `Received ${req.method} request for ${req.url}`);
    next();
});
app.get('/', (req, res) => {
    res.sendFile(join(__dirname, 'src/index.html'));
});

io.on('connection', (socket) => {
    
    socket.on('pump', (arg, callback) => {
        log("INFO","pump " + arg);

        socket.to(masterID).emit('pump', arg);
    })

    socket.on('master', (arg, callback) => {
        log("INFO", "master connected");
        masterID = socket.id;
    })
    socket.on('client', (arg, callback) => {
        log("INFO", "client connected");
        socket.join('clients');
    })

    socket.on('kefteme', (arg, callback) => {
        log("INFO", "Getted emit with Json from esp");
        if(arg){
            try {
                let data = JSON.parse(arg.val);
                log("INFO", "Json parsed successfully!")
                const setOfExtractedValuesFromJson = prepareDataFromEspForClient(data);
                socket.to('clients').emit('updateCurrentStateOnClientSide', setOfExtractedValuesFromJson);
            }catch (e){
                log("ERROR", e);
            }
        }
    })

    socket.on('sendCurrentState', (arg,callback) => {
        log("INFO", "Getted corrunt state!");
        socket.emit('sendCurrentState', arg);
    })
    socket.on('disconnect', () => {
        console.log('user disconnected');
    });
});

app.post('/api/sendCurrentState', (req, res) => {
    const body = req.body;
    const newEntry = {
        timestamp: new Date().toISOString(),
        data: body
    };
    //pushStatisticsToDB(body, newEntry, res);
    updateCurrentStateOnClientSide()
});

app.post('/api/sendCurrentStateToStatistics', (req, res) => {
    const body = req.body;
    const newEntry = {
        timestamp: new Date().toISOString(),
        data: body
    };
    pushStatisticsToDB(body, newEntry, res);
    updateCurrentStateOnClientSide()
});

server.listen(PORT, () => {
    log("INFO", `server running at ${PORT}`);
});