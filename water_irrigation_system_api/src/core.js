const fs = require('fs'); // Ensure fs is required
const { log } = require('../utils/logger'); // Import your log function

const pushStatisticsToDB = (body, newEntry, res) => {
    log("INFO", "pushing state to BD...");
    if (!body || Object.keys(body).length === 0) {
        return res.status(400).send("ERROR: Body is missing");
    }

    // Read current data from statistics.json
    fs.readFile('statistics.json', 'utf8', (err, data) => {
        if (err) {
            log('ERROR', `Error reading file: ${err}`);
            return res.status(500).send("ERROR: Unable to read statistics.json");
        }

        let statistics = [];

        // Parse file contents or create an empty array if file is empty
        try {
            statistics = JSON.parse(data) || [];
            log('INFO', 'Statistics data read successfully.');
        } catch (parseError) {
            log('ERROR', `Error parsing JSON: ${parseError}`);
            return res.status(500).send("ERROR: Invalid JSON format in statistics.json");
        }

        // Add new entry to the statistics array
        statistics.push(newEntry);

        // Write updated data back to statistics.json
        fs.writeFile('statistics.json', JSON.stringify(statistics, null, 2), (writeErr) => {
            if (writeErr) {
                log('ERROR', `Error writing file: ${writeErr}`);
                return res.status(500).send("ERROR: Unable to write to statistics.json");
            }

            // Log success and send a response after writing to the file
            log('INFO', 'Data written successfully to statistics.json.');
            res.status(200).send({ message: "Data added successfully", newEntry });
        });
    });
}

function updateCurrentStateOnClientSide () {

}



const prepareDataFromEspForClient = (data) => {
    // Extracting valves, pump, and sensor statuses
    const valves = data.data.data.system.components.valves;
    const sensors = data.data.data.system.components.sensors;
    const set = {};

// Declaring statuses for valves
    set.valve1 = valves[0].status; // Status of Valve 1
    set.valve2 = valves[1].status; // Status of Valve 2
    set.valve3 = valves[2].status; // Status of Valve 3

// Declaring status for pump
    set.pump = data.data.data.system.components.pump.status;

// Declaring sensor values
    set.soilMoistureSensors = {};
    for (const sensor of sensors.soilMoistureSensors) {
        set[`soilMoistureSensor${sensor.id}`] = {
            value: sensor.value,
            unit: sensor.unit,
            location: sensor.location
        };
    }

    set.temperatureSensor = {
        value: sensors.temperatureSensor.value,
        unit: sensors.temperatureSensor.unit
    };

    set.waterLevelSensor = {
        value: sensors.waterLevelSensor.value,
        unit: sensors.waterLevelSensor.unit,
        location: sensors.waterLevelSensor.location
    };

    set.airHumiditySensor = {
        value: sensors.airHumiditySensor.value,
        unit: sensors.airHumiditySensor.unit
    };

// Logging the results
    console.log("Pump status:", set.pump);
    console.log("Valve statuses:", {
        valve1: set.valve1,
        valve2: set.valve2,
        valve3: set.valve3
    });
    console.log("Soil Moisture Sensors:", {
        soilMoistureSensor1: set.soilMoistureSensor1,
        soilMoistureSensor2: set.soilMoistureSensor2
    });
    console.log("Temperature Sensor:", set.temperatureSensor);
    console.log("Water Level Sensor:", set.waterLevelSensor);
    console.log("Air Humidity Sensor:", set.airHumiditySensor);
    return set;
}

module.exports = { pushStatisticsToDB, updateCurrentStateOnClientSide, prepareDataFromEspForClient };
