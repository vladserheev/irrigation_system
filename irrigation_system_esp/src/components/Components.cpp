#include "Components.h"

Components::Components() : valves(), pump(0, "", false) {}

Components::Components(const std::vector<Valve>& valves, const Pump& pump)
    : valves(valves), pump(pump) {}

void Components::print() const{
    Serial.println("--- Valves ---");
    for (const auto& valve : valves) {
        valve.print();
        Serial.println();  // Print newline after each valve
    }
    Serial.println("--- Pump ---");
    pump.print();
}

bool Components::canTurnOnPump() {
    bool valve1Open = false, valve2Open = false, valve3Open = false;

    // Check the status of each valve by name
    for (const auto& valve : valves) {
        if (valve.name == "valve1"){valve1Open = valve.status;}
        if (valve.name == "valve2"){valve2Open = valve.status;}
        if (valve.name == "valve3"){valve3Open = valve.status;}
    }
    // The pump can be turned on if both Valve1 && Valve2 are open, or any one valve is open
    //return (valve1Open && valve2Open) || (valve1Open || valve3Open);
    return true;
}

bool Components::getValveStatusByName(String name) {
    for (const Valve& valve : valves) {
        // Convert String to std::string for comparison
        if (valve.name == name.c_str()) {  // name.c_str() converts String to a C-style string
            return valve.status;
        }
    }
    return false; // Return false if valve not found
}


// Method to check if a valve can be closed
bool Components::canCloseValves() {
    bool valve1Open = false, valve2Open = false, valve3Open = false;

    // Check the status of each valve by name
    for (const Valve& valve : valves) {
        if (valve.name == "valve1") valve1Open = valve.status;
        if (valve.name == "valve2") valve2Open = valve.status;
        if (valve.name == "valve3") valve3Open = valve.status;
    }
    // Valves cannot be closed if the pump is running
    //return !pump.status && (!(valve1Open && valve2Open) || !(valve1Open || valve3Open));
    return true;
}

void Components::updateComponentStateByName(std::map<String, int> pinMap, std::string componentName, std::string componentStatus){
    bool componentStatusBool = (componentStatus == "ON");
    Log.notice("Components: Updating componets with name %s"CR, componentName.c_str());
    if (componentName == pump.name) {
        if (componentStatusBool && !pump.status) { 
            Serial.println("---- canTurnonPump -----");
            Serial.println(canTurnOnPump());
            Serial.println("----");// If trying to turn on the pump
            if (canTurnOnPump()) {
                pump.digitalUpdate(pinMap, true);  // Turn on the pump
            } else {
                Serial.println("Cannot turn on the pump: No valves are open.");
            }
        } else if (!componentStatusBool && pump.status) {  // If trying to turn off the pump
            pump.digitalUpdate(pinMap, false);  // Turn off the pump
        }
    } else {
        // Update the status of a valve
        for (Valve& valve : valves) {
            if (valve.name == componentName) {
            Serial.println(canCloseValves());
                // if (!componentStatusBool && canCloseValves()) {
                if(false){
                    Serial.println("Cannot close valve: Pump is running.");
                } else {
                    if(valve.digitalUpdate(pinMap, componentStatusBool)){
                        Log.verbose("Components: %s updated successfully!"CR, componentName.c_str());
                    }
                }
            }
        }
    }
}

JsonDocument Components::toJson(){
    JsonDocument jsonString;

    for(Valve valve : valves){
        jsonString["Valves"].add(valve.toJson());
    }
    jsonString["Pump"]=pump.toJson();
    return jsonString;
}