const fs = require('fs').promises; // Ensure fs is required
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

const pushSensorsDataToDB = async (filename, newEntry) => {
    console.log("new data");
    console.log(newEntry);
    try {
        // Read the current zones configuration
        const data = await fs.readFile(filename, 'utf8');
        log('INFO', 'Current sensors data read successfully.');

        let sensorsData = JSON.parse(data); // Parse the existing zones configuration

        // Update only the schedules for each zone based on new data
        sensorsData.sensorsData.push(newEntry);

        // Write the updated configuration back to the file
        await fs.writeFile(filename, JSON.stringify(sensorsData, null, 2));
        log('INFO', 'Zones configuration updated successfully.');

        return sensorsData;
    } catch (error) {
        log('ERROR', `Failed to update zones configuration: ${error}`);
        throw new Error('Zones configuration update failed');
    }
}

const pushWateringLogDataToDB = async (filename, newEntry) => {
    console.log("new data");
    console.log(newEntry);
    try {
        // Read the current zones configuration
        const data = await fs.readFile(filename, 'utf8');
        log('INFO', 'Current sensors data read successfully.');

        let dataFromDb = JSON.parse(data); // Parse the existing zones configuration

        // Update only the schedules for each zone based on new data
        newEntry.forEach((item) => dataFromDb.zonesWateringData.push(item));
        //dataFromDb.zonesWateringData.push(newEntry);

        // Write the updated configuration back to the file
        await fs.writeFile(filename, JSON.stringify(dataFromDb, null, 2));
        log('INFO', 'Zones configuration updated successfully.');

        return dataFromDb;
    } catch (error) {
        log('ERROR', `Failed to update zones configuration: ${error}`);
        throw new Error('Zones configuration update failed');
    }
}

const getConfigForRootFromDB = async (filename) => {
    return readFromDB(filename);
}

const getZonesStateFromDB = async (filename) => {
    return readFromDB(filename);
}

const getDataForChartFromDB = async (filename) => {
    return readFromDB(filename);
}



// const updateZonesConfigTimedSettings = (filename, newSchedulesData) => {
//     // Read the current zones configuration
//     return new Promise((resolve, reject) => {
//         fs.readFile(filename, 'utf8', (err, data) => {
//             if (err) {
//                 log('ERROR', `Error reading file: ${err}`);
//                 reject(`Error reading file: ${err}`);
//                 return;
//             }
//
//             let zonesConfig;
//             try {
//                 zonesConfig = JSON.parse(data); // Parse the existing zones configuration
//                 log('INFO', 'Current zones configuration read successfully.');
//             } catch (parseError) {
//                 log('ERROR', `Error parsing JSON: ${parseError}`);
//                 reject(`Error parsing JSON: ${parseError}`);
//                 return;
//             }
//
//             // Update only the schedules for each zone based on new data
//             newSchedulesData.zones.forEach(newZone => {
//                 const zoneToUpdate = zonesConfig.zones.find(zone => zone.name === newZone.name);
//                 if (zoneToUpdate) {
//                     zoneToUpdate.schedules = newZone.schedules; // Update schedules only
//                 }
//             });
//
//             // Write the updated configuration back to the file
//             fs.writeFile(filename, JSON.stringify(zonesConfig, null, 2), (writeErr) => {
//                 if (writeErr) {
//                     log('ERROR', `Error writing file: ${writeErr}`);
//                     return;
//                 }
//                 log('INFO', 'Zones configuration updated successfully.');
//                 resolve(zonesConfig);
//             });
//         });
//     });
// };




const updateZonesConfigTimedSettings = async (filename, newSchedulesData) => {
    console.log("new data");
    console.log(newSchedulesData);
    try {
        // Read the current zones configuration
        const data = await fs.readFile(filename, 'utf8');
        log('INFO', 'Current zones configuration read successfully.');

        let zonesConfig = JSON.parse(data); // Parse the existing zones configuration

        // Update only the schedules for each zone based on new data
        newSchedulesData.zones.forEach(newZone => {
            const zoneToUpdate = zonesConfig.zones.find(zone => zone.name === newZone.name);
            if (zoneToUpdate) {
                console.log('update schedule');
                zoneToUpdate.schedules = newZone.schedules; // Update schedules only
            }
        });

        // Write the updated configuration back to the file
        await fs.writeFile(filename, JSON.stringify(zonesConfig, null, 2));
        log('INFO', 'Zones configuration updated successfully.');

        return zonesConfig;
    } catch (error) {
        log('ERROR', `Failed to update zones configuration: ${error}`);
        throw new Error('Zones configuration update failed');
    }
};



//const fs = require('fs').promises; // Use fs.promises for promise-based file system operations

const readFromDB = async (filename) => {
    try {
        const data = await fs.readFile(filename, 'utf8');
        let configRoot = [];

        // Parse file contents or create an empty array if file is empty
        configRoot = JSON.parse(data) || [];
        log('INFO', 'Data read successfully.');
        return configRoot; // Return the parsed data
    } catch (error) {
        if (error.code === 'ENOENT') {
            log('ERROR', `File not found: ${filename}`);
            return "ERROR: File not found.";
        } else if (error instanceof SyntaxError) {
            log('ERROR', `Error parsing JSON: ${error.message}`);
            return "ERROR: Invalid JSON format in " + filename;
        } else {
            log('ERROR', `Error reading file: ${error.message}`);
            return "ERROR: Unable to read " + filename;
        }
    }
};


const prepareDataSetTimedMode = () => {
    return readFromDB("zones.json")
        .then(data => {
            console.log(data);
            return data;
        }).catch(error => {
            console.log(error);
            return error;
        })
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

module.exports = { pushStatisticsToDB, pushWateringLogDataToDB, pushSensorsDataToDB, getDataForChartFromDB, getZonesStateFromDB,prepareDataSetTimedMode, prepareDataFromEspForClient,updateZonesConfigTimedSettings, getConfigForRootFromDB };
