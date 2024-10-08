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

let masterID = '';
let isConnectedToMaster = false;

app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use(express.static(path.join(__dirname, 'public')));
app.use(cors());
app.use((req, res, next) => {
    log("INFO", `Received ${req.method} request for ${req.url}`);
    next();
});
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html')); // Change 'src' to 'public'
});

io.on('connection', (socket) => {
    socket.on('btnAction', (arg, callback) => {
        log("INFO", arg.btnName + " " + arg.action);
        if (masterID) {
            socket.to(masterID).emit("btnAction", { btnName: arg.btnName, btnVal: arg.action });
        } else {
            log("ERROR", "Master not connected, cannot send button action");
            if (typeof callback === 'function') {
                callback({ error: "Master not connected" });
            }
        }
    });

    socket.on('master', (arg, callback) => {
        log("INFO", "Master connected with ID: " + socket.id);
        socket.to("clients").emit("isConnectedToMaster", true);
        masterID = socket.id;
        isConnectedToMaster = true;
    });

    socket.on('client', (arg, callback) => {
        log("INFO", "Client connected with ID: " + socket.id);
        socket.join('clients');
    });

    socket.on('kefteme', (arg, callback) => {
        log("INFO", "Received emit with JSON from ESP");
        if (arg && arg.val) {
            try {
                log("INFO", arg.val);
                socket.to('clients').emit('updateCurrentStateOnClientSide', arg.val);
            } catch (e) {
                log("ERROR", e);
            }
        }
    });

    socket.on('checkIfConnectedToMaster', (arg, callback) => {
        log('INFO', `Client ${socket.id} checking if connected to master...`);
        if (typeof callback === 'function') {
            if (isConnectedToMaster) {
                log('INFO', `Client ${socket.id} connected to master ${masterID}`);
                callback(true);
            } else {
                callback(false);
            }
        } else {
            log('ERROR', "Callback is not a function for checking master connection!");
        }
    });

    socket.on('sendCurrentState', (arg, callback) => {
        log("INFO", "Received current state!");
        socket.emit('sendCurrentState', arg);
    });

    socket.on('disconnect', () => {
        log("INFO", `Socket disconnected: ${socket.id}`);
        if (socket.id === masterID) {
            masterID = 0;
            isConnectedToMaster = false;
            socket.to("clients").emit("isConnectedToMaster", false);
            log("INFO", 'Master disconnected');
        } else {
            log("INFO", 'Client disconnected');
        }
    });
});

app.post('/api/sendCurrentState', (req, res) => {
    const body = req.body;
    const newEntry = {
        timestamp: new Date().toISOString(),
        data: body
    };
    updateCurrentStateOnClientSide();
});

app.post('/api/manualSettingsForm', (req, res) => {
    const body = req.body;
    log("INFO", "Got manual Settings Form");
    log("INFO", body);

    if (masterID) {
        io.to(masterID).emit("manualSettings", body);
    } else {
        log("ERROR", "Master is not connected, unable to send manual settings.");
        return res.status(500).send({ error: "Master is not connected." });
    }

    if (body) {
        res.status(200).send(body);  // Respond with success
    } else {
        res.status(400).send({ error: "Invalid data." });  // Handle invalid request body
    }
});

app.post('/api/timeSettingsForm', (req, res) => {
    const body = req.body;
    log("INFO", "Got Time Settings Form");
    log("INFO", body);

    if (masterID) {
        io.to(masterID).emit("timeSettings", body);  // Emit to the master by socket ID
    } else {
        log("ERROR", "Master is not connected, unable to send manual settings.");
        return res.status(500).send({ error: "Master is not connected." });
    }

    if (body) {
        res.status(200).send(body);
    } else {
        res.status(400).send({ error: "Invalid data." });
    }
});

app.post('/api/sendCurrentStateToStatistics', (req, res) => {
    const body = req.body;
    const newEntry = {
        timestamp: new Date().toISOString(),
        data: body
    };
    pushStatisticsToDB(body, newEntry, res);
    updateCurrentStateOnClientSide();
});

server.listen(PORT, () => {
    log("INFO", `Server running at ${PORT}`);
});
