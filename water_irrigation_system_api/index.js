const express = require('express');
const { createServer } = require('node:http');
const { join } = require('node:path');
const { Server } = require('socket.io');

const app = express();
const server = createServer(app);
const io = new Server(server);

const PORT = 8080;

let masterID='';

app.get('/', (req, res) => {
    res.sendFile(join(__dirname, 'src/index.html'));
});

io.on('connection', (socket) => {
    console.log('a user connected');

    socket.on('pump', (arg, callback) => {
        console.log("pump ", arg);

        socket.to(masterID).emit('pump', arg);
    })

    socket.on('master', (arg, callback) => {
        console.log("masterID: " + socket.id);
        masterID = socket.id;
    })
});


server.listen(PORT, () => {
    console.log(`server running at ${PORT}`);
});