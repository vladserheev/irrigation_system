const express = require('express');
const path  = require("path");
const { createServer } = require('node:http');
const { join } = require('node:path');
const { Server } = require('socket.io');
const { log } = require('./utils/logger');
const { pushStatisticsToDB, updateCurrentStateOnClientSide, prepareDataFromEspForClient } = require('./src/core');
const app = express();
const server = createServer(app);
const io = new Server(server);
const cors = require('cors');

const PORT = 8080;

let masterID='';
let isConnectedToMaster = false;
app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use(express.static(path.join(__dirname, 'public')));
app.use(cors());
//app.use(express.static(path.join(__dirname, "js")));
app.use((req, res, next) => {
    log("INFO", `Received ${req.method} request for ${req.url}`);
    next();
});
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html')); // Change 'src' to 'public'
});


io.on('connection', (socket) => {

    socket.on('btnAction', (arg, callback) => {
        log("INFO",arg.btnName + " " + arg.action);
        socket.to(masterID).emit(arg.btnName, arg.action);
    })

    socket.on('master', (arg, callback) => {
        log("INFO", "Master connected with ID: " + socket.id);
        socket.to("clients").emit("isConnectedToMaster", true);
        masterID = socket.id;
        isConnectedToMaster = true;
    })
    socket.on('client', (arg, callback) => {
        log("INFO", "client connected with ID: " + socket.id);
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

    socket.on('checkIfConnectedToMaster', (arg, callback) => {
        log('INFO', "Checking if client connected to master...");
        if (typeof callback === 'function') {
            callback(isConnectedToMaster);  // Responds with the master connection status
        } else {
            log('ERROR', "Callback is not a function!");
        }
    });

    socket.on('sendCurrentState', (arg,callback) => {
        log("INFO", "Getted corrunt state!");
        socket.emit('sendCurrentState', arg);
    })
    socket.on('disconnect', () => {
        log("INFO", `Socket disconnected: ${socket.id}`);
        if(socket.id === masterID){
            masterID = 0;
            isConnectedToMaster = false;
            socket.to("clients").emit("isConnectedToMaster", false);
            log("INFO", 'master disconnected');
        }else{
            log("INFO", 'client disconnected');
        }
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

app.post('/api/manualSettingsForm', (req, res) => {
    const body = req.body;

    log("INFO", "Got manual Settings Form");
    log("INFO", body);
    if(body)
        res.send(200, body);
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