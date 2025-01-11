const express = require('express');
const path  = require("path");
const { createServer } = require('node:http');
const { join } = require('node:path');
const { Server } = require('socket.io');
const { log } = require('./utils/logger');
const { pushStatisticsToDB,prepareDataSetTimedMode,pushWateringLogDataToDB, getConfigForRootFromDB, pushSensorsDataToDB,getDataForChartFromDB, updateZonesConfigTimedSettings, getZonesStateFromDB, updateCurrentStateOnClientSide, prepareDataFromEspForClient } = require('./src/core');
const app = express();
const server = createServer(app);
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

        io.to("clients").emit("isConnectedToMaster", isConnectedToMaster);

        getZonesStateFromDB("zones.json")
            .then(zonesState => {
                console.log("Zones State:", zonesState);
                io.to("clients").emit("timedModeSettings", zonesState)})
            .catch(error => {
                    console.error(error);
                }
            );
        getDataForChartFromDB("dataForChart.json")
            .then(dataForChart => {
                console.log("Date for chart:", dataForChart);
                io.to("clients").emit("getDataForStatistics", dataForChart)})
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
                log("INFO", arg.val.System);
                io.to('clients').emit('updateCurrentStateOnClientSide', arg.val);
            } catch (e) {
                log("ERROR", e);
            }
        }
    });

    socket.on('sensorsData',async (arg, callback) => {
        log("INFO", "Received emit with Sensors data from ESP");
        if (arg && arg.val) {
            try {
                log("INFO", arg.val);
                //console.log(arg.val[2].timestamp.toISOString());
                const date = new Date(arg.val[2].timestamp);
                console.log(date.toISOString());

                const newEntry = {
                    "date": date.toISOString(),
                    "temperature": arg.val[0].tempValue,
                    "soilHumidity": arg.val[1].value,
                    "airHumidity": arg.val[0].humValue
                }
                console.log(newEntry);
                await pushSensorsDataToDB("dataForChart.json", newEntry);
                //io.to('clients').emit('updateCurrentStateOnClientSide', arg.val);
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

    socket.on('wateringLog',async (arg, callback) => {
        log("INFO", "Received watering logs");
        //socket.emit('sendCurrentState', arg);
        log("info", arg);

        if (arg && arg.val) {
            try {
                log("INFO", arg.val);
                //console.log(arg.val[2].timestamp.toISOString());
                const date = new Date(arg.val[0].timestampStart)
                console.log(date.toISOString());
                const newEntry=[];
                arg.val.forEach((record) => {
                    let validatedRecord = {
                        "zone": record.name,
                        "start": new Date(record.timestampStart).toISOString(),
                        "end": new Date(record.timestampFinish).toISOString(),
                    }
                    newEntry.push(validatedRecord);
                })

                console.log(newEntry);
                await pushWateringLogDataToDB("dataForChart.json", newEntry);
                //io.to('clients').emit('updateCurrentStateOnClientSide', arg.val);
            } catch (e) {
                log("ERROR", e);
            }
        }
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
    }
});

app.post('/api/timeSettingsForm', async (req, res) => {
    const body = req.body;
    log("INFO", "Got Time Settings Form");
    log("INFO", body);

    try {

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
        //res.status(500).send('Failed to update zones configuration');
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
    const interfaces = require('os').networkInterfaces();
    console.log(`Server is running on:`);
    for (const interfaceName in interfaces) {
        interfaces[interfaceName].forEach(interfaceInfo => {
            if (interfaceInfo.family === 'IPv4' && !interfaceInfo.internal) {
                console.log(`- http://${interfaceInfo.address}:${PORT}`);
            }
        });
    }
    log("INFO", `Server running at ${PORT}`);
});
