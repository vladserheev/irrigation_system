const express = require('express');
const app = express();
const fs = require('fs');

const { createServer } = require('node:http');
const { join } = require('node:path');
const { Server } = require('socket.io');

const PORT = 8080;

const server = createServer(app);
const io = new Server(server);

app.use(express.json());
app.use((req, res, next) => {
    console.log(`Received ${req.method} request for ${req.url}`);
    next();
});

app.get('/', (req, res) => {
    console.log(req.body);
    res.send('Welcome to my server!');
    console.log("connected");
});

io.on('connection', (socket) => {
    console.log('a user connected');
});

app.get('/api', (req, res) => {
    console.log('connected to api');
    res.status(200).send({
        temp: 10,
        hum: 50
    });
});

app.post('/api/sendCurrentState', (req, res) => {
    const body = req.body;
    const newEntry = {
        timestamp: new Date().toISOString(),
        data: body
    };

    pushStatisticsToDB(body, newEntry, res); // Pass res to handle the response
});

function pushStatisticsToDB(body, newEntry, res) {
    if (!body || Object.keys(body).length === 0) {
        return res.status(400).send("ERROR: Body is missing");
    }

    // Read current data from statistics.json
    fs.readFile('statistics.json', 'utf8', (err, data) => {
        if (err) {
            console.error("Error reading file:", err);
            return res.status(500).send("ERROR: Unable to read statistics.json");
        }

        let statistics = [];

        // Parse file contents or create an empty array if file is empty
        try {
            statistics = JSON.parse(data) || [];
            console.log(statistics);
        } catch (parseError) {
            console.error("Error parsing JSON:", parseError);
            return res.status(500).send("ERROR: Invalid JSON format in statistics.json");
        }

        // Add new entry to the statistics array
        console.log(newEntry);
        statistics.push(newEntry);

        // Write updated data back to statistics.json
        fs.writeFile('statistics.json', JSON.stringify(statistics, null, 2), (writeErr) => {
            if (writeErr) {
                console.error("Error writing file:", writeErr);
                return res.status(500).send("ERROR: Unable to write to statistics.json");
            }

            // Send a response after writing to the file
            res.status(200).send({ message: "Data added successfully", newEntry });
        });
    });
}

server.listen(PORT, '0.0.0.0', () => console.log(`App is listening on port ${PORT}`));
