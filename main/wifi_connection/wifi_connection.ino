#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>

// WiFi Credentials
const char* ssid = "your_wifi_ssid";
const char* password = "your_wifi_password";

// WebSocket server address and port (Node.js server)
const char* webSocketServer = "your_server_ip";  // or domain name if you are using DNS
const int webSocketPort = 3000;

WebSocketsClient webSocket;

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

  // Initialize WebSocket connection
  webSocket.begin(webSocketServer, webSocketPort, "/");
  
  // Event handler for WebSocket events
  webSocket.onEvent(webSocketEvent);
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.println("WebSocket Disconnected");
      break;
    case WStype_CONNECTED:
      Serial.println("WebSocket Connected");
      // Send a message when connected
      webSocket.sendTXT("Hello from ESP8266!");
      break;
    case WStype_TEXT:
      // Handle incoming messages from the server
      Serial.printf("Message from server: %s\n", payload);
      break;
  }
}

void loop() {
  webSocket.loop(); // Keep WebSocket connection alive
}
