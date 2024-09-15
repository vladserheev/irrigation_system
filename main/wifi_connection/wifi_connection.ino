#include <SoftwareSerial.h>

// Define SoftwareSerial pins
SoftwareSerial espSerial(10, 11); // RX, TX

// Your WiFi credentials
const char* ssid = "iPhone";
const char* password = "11111111";

// Your web server details
const char* host = "172.20.10.3"; // Replace with the local IP address of your server
const int httpPort = 8080; // Replace with the port your server is listening on

void setup() {
  Serial.begin(9600);         // Start serial for debugging
  espSerial.begin(115200);    // Start software serial for ESP-01

  // Initialize ESP-01
  sendATCommand("AT", 1000);
  sendATCommand("AT+CWMODE=1", 1000);   // Set WiFi mode to STA
  
  // Connect to WiFi
  connectToWiFi();
  
  // Connect to the web server and send a GET request
  sendGetRequest();
}

void loop() {
  // Print responses from ESP-01
  while (espSerial.available()) {
    Serial.print((char)espSerial.read());
  }
}

void connectToWiFi() {
  String cmd = "AT+CWJAP=\"";
  cmd += ssid;
  cmd += "\",\"";
  cmd += password;
  cmd += "\"";
  
  sendATCommand(cmd, 30000); // Increased timeout for connecting to WiFi
}

void sendGetRequest() {
  // Start connection to the server
  String connectCmd = "AT+CIPSTART=\"TCP\",\"";
  connectCmd += host; // IP address or domain name
  connectCmd += "\",";
  connectCmd += String(httpPort); // Port number
  
  sendATCommand(connectCmd, 10000); // Connect to the server
  
  // Send GET request
  String httpRequest = "GET /api/kurwa HTTP/1.1\r\n";
  httpRequest += "Host: ";
  httpRequest += host; // The same IP address or domain name
  httpRequest += "\r\n";
  httpRequest += "Connection: close\r\n\r\n";
  
  // Send length of the request to the ESP-01
  String sendCmd = "AT+CIPSEND=";
  sendCmd += String(httpRequest.length());
  sendATCommand(sendCmd, 1000); // Send length command
  
  // Delay to allow ESP-01 to process the command
  delay(1000);
  
  // Send actual GET request
  espSerial.print(httpRequest);
}


void sendATCommand(String command, const int timeout) {
  Serial.println("Sending command: " + command); // Debugging
  espSerial.println(command);
  long int time = millis();
  while ((millis() - time) < timeout) {
    while (espSerial.available()) {
      char c = espSerial.read();
      Serial.print(c);  // Print ESP-01 response to Serial Monitor
    }
  }
  Serial.println(); // Newline for readability
}
