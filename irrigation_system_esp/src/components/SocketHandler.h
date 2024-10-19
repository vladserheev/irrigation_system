#ifndef SOCKETHANDLER
#define SOCKETHANDLER

#include <Arduino.h>
#include <SocketIOclient.h>
#include <ArduinoLog.h>
//#include "ModeHandler.h"

SocketIOclient socketIO;

class SocketHandler {
    ModeHandler& modeHandler; // Reference to ModeHandler

public:
    SocketHandler(ModeHandler& handler) : modeHandler(handler) {}

    void handlingSocketEvent(String eventName, DynamicJsonDocument doc) {
        Serial.printf("Handling socket event: %s \n", eventName);

        if (eventName == "btnAction") {
            Serial.println("Sensor Control!");
            // Handle button action here
        } else if (eventName == "sensorSettings") {
            Serial.println("Sensor Setting!");
        } else if (eventName == "timeSettings") {
            Serial.println("Time Setting!");
            modeHandler.addNewTimeSettingsToZones(doc); // Call the method on modeHandler
        }
    }

    void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length) {
        switch (type) {
            case sIOtype_DISCONNECT:
                Log.notice("[IOc] Disconnected!\n");
                break;
            case sIOtype_CONNECT:
                Log.notice("[IOc] Connected to url: %s\n", payload);
                socketIO.send(sIOtype_CONNECT, "/");
                sendEmit("master", "true");
                break;
            case sIOtype_EVENT: {
                char *sptr = NULL;
                int id = strtol((char *)payload, &sptr, 10);
                Log.notice("[IOc] get event: %s id: %d\n", payload, id);
                if (id) {
                    payload = (uint8_t *)sptr;
                }
                DynamicJsonDocument doc(1024);
                DeserializationError error = deserializeJson(doc, payload, length);
                if (error) {
                    Log.notice(F("deserializeJson() failed: "));
                    Log.notice(error.c_str());
                    return;
                }

                String eventName = doc[0];
                handlingSocketEvent(eventName, doc); // Call instance method

                Log.notice("[IOc] event name: %s\n", eventName.c_str());

                // Message Includes an ID for a ACK (callback)
                if (id) {
                    // Create JSON message for Socket.IO (ack)
                    DynamicJsonDocument docOut(1024);
                    JsonArray array = docOut.to<JsonArray>();
                    JsonObject param1 = array.createNestedObject();
                    param1["now"] = millis();

                    String output;
                    output += id;
                    serializeJson(docOut, output);

                    // Send event
                    socketIO.send(sIOtype_ACK, output);
                }
                break;
            }
            case sIOtype_ACK:
                Log.notice("[IOc] get ack: %u\n", length);
                break;
            case sIOtype_ERROR:
                Log.error("[IOc] get error: %u\n", length);
                break;
            case sIOtype_BINARY_EVENT:
                Log.notice("[IOc] get binary: %u\n", length);
                break;
            case sIOtype_BINARY_ACK:
                Log.notice("[IOc] get binary ack: %u\n", length);
                break;
        }
    }

    void socketConnect(const char* ip, u16_t port, const char* query) {
        socketIO.begin(ip, port, query);  // Use your server's address and port
        socketIO.onEvent([this](socketIOmessageType_t type, uint8_t *payload, size_t length) {
            socketIOEvent(type, payload, length); // Pass the event to the instance method
        });
    }

    void sendEmit(String name, String val) {
        DynamicJsonDocument doc(1024);
        JsonArray array = doc.to<JsonArray>();
        array.add(name);

        JsonObject param1 = array.createNestedObject();
        param1["val"] = val;

        String output;
        serializeJson(doc, output);

        socketIO.sendEVENT(output);
        Log.notice("Sent emit to socket server!\n");
    }

    void sendEmitJson(String name, JsonDocument jsonString) {
        DynamicJsonDocument doc(1024);
        JsonArray array = doc.to<JsonArray>();
        array.add(name);

        JsonObject param1 = array.createNestedObject();
        param1["val"] = jsonString;

        String output;
        serializeJson(doc, output);
        socketIO.sendEVENT(output);
        Log.notice("Sent emit with Json to socketIo server!\n");
    }
};

#endif
