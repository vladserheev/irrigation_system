#ifndef SOCKETHANDLER
#define SOCKETHANDLER

#include <Arduino.h>
#include <SocketIOclient.h>
#include <ArduinoLog.h>
#include "EventEmiter.h"

class SocketHandler {
    EventEmitter& eventEmitter;
    SocketIOclient socketIO;

    const char* socketioIp;
    uint16_t socketioPort;
    const char* socketioQuery;

    DynamicJsonDocument doc{1024}; // Резервирование памяти только один раз

public:
    SocketHandler(EventEmitter& eventEmitter, const char* ip, uint16_t port, const char* query)
        : eventEmitter(eventEmitter), socketioIp(ip), socketioPort(port), socketioQuery(query) {}

    void initializeSocket() {
        Log.notice("Socket initilisation..."CR);
        socketIO.begin(socketioIp, socketioPort, socketioQuery);
        socketIO.onEvent([this](socketIOmessageType_t type, uint8_t *payload, size_t length) {
            socketIOEvent(type, payload, length);
        });
    }

    void loop() {
        socketIO.loop(); // Call this method in your main loop function
    }

    bool isConnected(){
        return socketIO.isConnected();
    }

    void handlingSocketEvent(const String& eventName, const DynamicJsonDocument& doc) {
        if (eventName == "btnAction") {
            eventEmitter.emitEvent("button_event", doc);
        } else if (eventName == "sensorSettings") {
            Log.notice("Sensor Setting received!" CR);
        } else if (eventName == "timeSettings") {
            eventEmitter.emitEvent("timed_mode", doc);
        } else if (eventName == "zonesConfig"){
            eventEmitter.emitEvent("zonesConfig", doc);
        }else {
            Log.warning("Unhandled socket event: %s\n", eventName.c_str());
        }
    }

    void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length) {
        switch (type) {
            case sIOtype_DISCONNECT:
                Log.notice("[IOc] Disconnected!");
                // Переподключение, если необходимо
                initializeSocket();
                break;
            case sIOtype_CONNECT:
                Log.notice("[IOc] Connected to url: %s\n", payload);
                //Log.verbose("Socketio connection: %t"CR, socketIO.isConnected());
                socketIO.send(sIOtype_CONNECT, "/");
                sendEmit("master", "true");
                break;
            case sIOtype_EVENT:
                parseSocketEvent(payload, length);
                break;
            default:
                Log.notice("[IOc] Event Type: %d, Length: %u\n", type, length);
                break;
        }
    }

private:
    void parseSocketEvent(uint8_t* payload, size_t length) {
        DeserializationError error = deserializeJson(doc, payload, length);
        if (error) {
            Log.error(F("deserializeJson() failed: %s"), error.c_str());
            return;
        }

        String eventName = doc[0].as<String>();
        handlingSocketEvent(eventName, doc);
    }

public:
    void sendEmit(const String& name, const String& val) {
        doc.clear();
        JsonArray array = doc.to<JsonArray>();
        array.add(name);
        JsonObject param = array.createNestedObject();
        param["val"] = val;

        String output;
        serializeJson(doc, output);
        socketIO.sendEVENT(output);
        Log.notice("Sent emit to socket server!\n");
    }

    void sendEmitJson(const String& name, const JsonDocument& jsonString) {
        doc.clear();
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
