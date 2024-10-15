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
const {response} = require("express");

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

    // io.use((socket, next) => {
    //     const role = socket.handshake.query.role;
    //     if (role === 'master') {
    //         // Handle master connection
    //         log("INFO", `Master connected with ID: ${socket.id}`);
    //         masterID = socket.id;
    //         isConnectedToMaster = true;
    //         socket.to('clients').emit("isConnectedToMaster", true);  // Notify clients that master is connected
    //         next();
    //     } else if (role === 'client') {
    //         // Handle client connection
    //         log("INFO", `Client connected with ID: ${socket.id}`);
    //         socket.join('clients');
    //         next();
    //     }
    // });

io.on('connection', (socket) => {
    console.log(socket.server.eio.clients);
    socket.on('btnAction', (arg, callback) => {
        log("INFO", arg.btnName + " " + arg.action);
        if(isConnectedToMaster) {
            socket.to(masterID).emit("btnAction", {btnName: arg.btnName, btnVal: arg.action});
            callback(true, "OK!");
        }else{
            callback(false, "Not connected to master!");
        }
    });

    socket.on("client", (args, callback) => {
        log("INFO", `Client connected with ID: ${socket.id}`);
        socket.join('clients');
        console.log(socket.rooms);
        io.to("clients").emit("isConnectedToMaster", isConnectedToMaster);
    });

    socket.on("master", (args, callback) => {
        log("INFO", `Master connected with ID: ${socket.id}`);
        masterID = socket.id;
        isConnectedToMaster = true;
        io.to('clients').emit("isConnectedToMaster", isConnectedToMaster);
    })

    socket.on('kefteme', (arg, callback) => {
        log("INFO", "Received emit with JSON from ESP");
        if (arg && arg.val) {
            try {
                log("INFO", arg.val);
                io.to('clients').emit('updateCurrentStateOnClientSide', arg.val);
            } catch (e) {
                log("ERROR", e);
            }
        }
    });

    socket.on('checkIfConnectedToMaster', (arg, callback) => {
        log('INFO', `Client ${socket.id} checking if connected to master...`);
        if (isConnectedToMaster) {
            log('INFO', `Client ${socket.id} connected to master ${masterID}`);
            console.log(socket.rooms);
            io.emit("isConnectedToMaster", true);

        } else {
            io.to('clients').emit("isConnectedToMaster", false);
            log('INFO', `Client ${socket.id} NOT connected to master ${masterID}`);
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
            io.to('clients').emit("isConnectedToMaster", false);  // Notify clients that master is disconnected
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


    const data = {
        zones: [
            {
                name: "Zone 1",
                startHour: parseInt(body.zone1Start.split(":")[0]),
                startMinute: parseInt(body.zone1Start.split(":")[1]),
                finishHour: parseInt(body.zone1End.split(":")[0]),
                finishMinute: parseInt(body.zone1End.split(":")[1])
            },
            {
                name: "Zone 2",
                startHour: parseInt(body.zone2Start.split(":")[0]),
                startMinute: parseInt(body.zone2Start.split(":")[1]),
                finishHour: parseInt(body.zone2End.split(":")[0]),
                finishMinute: parseInt(body.zone2End.split(":")[1])
            }
        ]
    };


    if (masterID) {
        io.to(masterID).emit("timeSettings", data);  // Emit to the master by socket ID
    } else {
        log("ERROR", "Master is not connected, unable to send manual settings.");
        return res.status(500).send({ error: "Master is not connected." });
    }

    if (body) {
        res.status(200).send(data);
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
