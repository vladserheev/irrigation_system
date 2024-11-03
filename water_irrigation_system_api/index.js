const express = require('express');
const path  = require("path");
const { createServer } = require('node:http');
const { join } = require('node:path');
const { Server } = require('socket.io');
const { log } = require('./utils/logger');
const { pushStatisticsToDB,prepareDataSetTimedMode, getConfigForRootFromDB, updateZonesConfigTimedSettings, getZonesStateFromDB, updateCurrentStateOnClientSide, prepareDataFromEspForClient } = require('./src/core');
const app = express();
const server = createServer(app);
//const io = new Server(server);
const cors = require('cors');
const {response} = require("express");
const readline = require("readline");

const PORT = 8080;

let masterID = '';
let isConnectedToMaster = false;

app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use(express.static(path.join(__dirname, 'public')));
const io = require('socket.io')(server, {
    cors: {
        origin: "*",  // Allow all origins
        methods: ["GET", "POST"],
        transports: ['websocket']
    },
    reconnectionAttempts: 5, // Number of reconnection attempts
    reconnectionDelay: 1000,  // Delay between reconnections
    timeout: 2000
});
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

        // getConfigForRootFromDB("configForRoot.json")
        //     .then(configRoot => {
        //         console.log("Config Root:", configRoot);})
        //     .catch(error => {
        //         console.error(error);
        //     });


        io.to("clients").emit("isConnectedToMaster", isConnectedToMaster);

        getZonesStateFromDB("zones.json")
            .then(zonesState => {
                console.log("Zones State:", zonesState);
                io.to("clients").emit("timedModeSettings", zonesState)})
            .catch(error => {
                    console.error(error);
                }
            );

    });

    socket.on("master", (args, callback) => {
        log("INFO", `Master connected with ID: ${socket.id}`);
        masterID = socket.id;
        isConnectedToMaster = true;
        //log(getConfigForRootFromDB());
        io.to('clients').emit("isConnectedToMaster", isConnectedToMaster);
        getZonesStateFromDB("zones.json")
            .then(zonesState => {
                console.log("Zones State:", zonesState);
                socket.emit("zonesConfig", zonesState)})
            .catch(error => {
                    console.error(error);
                }
            );
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

// app.post('/api/timeSettingsForm',  (req, res) => {
//     const body = req.body;
//     log("INFO", "Got Time Settings Form");
//     log("INFO", body);
//     log("info", "updatingZones");
//
//     // const data = {
//     //     zones: [
//     //         {
//     //             name: "Zone 1",
//     //             startHour: parseInt(body.zone1Start.split(":")[0]),
//     //             startMinute: parseInt(body.zone1Start.split(":")[1]),
//     //             finishHour: parseInt(body.zone1End.split(":")[0]),
//     //             finishMinute: parseInt(body.zone1End.split(":")[1])
//     //         },
//     //         {
//     //             name: "Zone 2",
//     //             startHour: parseInt(body.zone2Start.split(":")[0]),
//     //             startMinute: parseInt(body.zone2Start.split(":")[1]),
//     //             finishHour: parseInt(body.zone2End.split(":")[0]),
//     //             finishMinute: parseInt(body.zone2End.split(":")[1])
//     //         }
//     //     ]
//     // };
//
//     const newSchedulesData = {
//         zones: [
//             {
//                 name: "Zone 1",
//                 schedules: [
//                     { startHour: 7, startMinute: 30, finishHour: 8, finishMinute: 0 },
//                     { startHour: 19, startMinute: 0, finishHour: 19, finishMinute: 30 }
//                 ]
//             },
//             {
//                 name: "Zone 2",
//                 schedules: [
//                     { startHour: 16, startMinute: 0, finishHour: 6, finishMinute: 45 }
//                 ]
//             }
//         ]
//     };
//
//     try {
//         updateZonesConfigTimedSettings('zones.json', newSchedulesData)
//             .then(res.status(200).send({ message: 'Zones configuration updated successfully.' })
//         );
//     } catch (error) {
//         res.status(500).send({ error: 'Failed to update zones configuration.' });
//     }
//     // updateZonesConfigTimedSettings("zones.json", newSchedulesData)
//     //     .then(schedulesData => {
            // if (masterID) {
            //     io.to(masterID).emit("timeSettings", newSchedulesData);  // Emit to the master by socket ID
            // } else {
            //     log("ERROR", "Master is not connected, unable to send manual settings.");
            //     return res.status(500).send({ error: "Master is not connected." });
            // }
//     //     })
//     //     .catch(error => {
//     //         res.status(400).send({ error: "Error while updating the DB." });
//     //     })
//
//
//
//     if (body) {
//         res.status(200).send(newSchedulesData);
//     } else {
//         res.status(400).send({ error: "Invalid data." });
//     }
// });

app.post('/api/timeSettingsForm', async (req, res) => {
    const body = req.body;
    log("INFO", "Got Time Settings Form");
    log("INFO", body);

    try {
        // const newSchedulesData = {
        //     zones: [
        //         {
        //             name: "Zone 1",
        //             schedules: [
        //                 { startHour: 7, startMinute: 30, finishHour: 8, finishMinute: 0 },
        //                 { startHour: 19, startMinute: 0, finishHour: 19, finishMinute: 30 }
        //             ]
        //         },
        //         {
        //             name: "Zone 2",
        //             schedules: [
        //                 { startHour: 16, startMinute: 0, finishHour: 6, finishMinute: 45 }
        //             ]
        //         }
        //     ]
        // }; // Assuming `req.body` has the new schedule data

        const newSchedulesData = body;
        await updateZonesConfigTimedSettings('zones.json', newSchedulesData)
        if (masterID) {
            io.to(masterID).emit("timeSettings", newSchedulesData);
            log("INFO", "sended to master!");// Emit to the master by socket ID
        } else {
            log("ERROR", "Master is not connected, unable to send manual settings.");
            res.status(500).send({ error: "Master is not connected." });
            throw "master is not defined";
        }
        res.status(200).send('Zones configuration updated successfully.' + newSchedulesData);
    } catch (error) {
        log("error", error);
        res.status(500).send('Failed to update zones configuration');
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
