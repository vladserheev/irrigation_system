#include <SoftwareSerial.h>

SoftwareSerial esp8266(10, 11); // RX, TX for SoftwareSerial

void setup() {
  Serial.begin(9600);   // Communication with PC (Serial Monitor)
  esp8266.begin(115200); // ESP8266 default baud rate is 115200, you may need to change this
  
  delay(2000);          // Let the module boot

  // Send AT command to check communication
  sendATCommand("AT", 2000, true);

  // Set mode to station mode (client mode)
  sendATCommand("AT+CWMODE=1", 2000, true);

  // Connect to WiFi Network
  sendATCommand("AT+CWJAP=\"iPhone\",\"11111111\"", 5000, true);
  
}

void loop() {
  if (esp8266.available()) {
    while (esp8266.available()) {
      char c = esp8266.read();
      Serial.write(c);
    }
  }
  
  if (Serial.available()) {
    while (Serial.available()) {
      char c = Serial.read();
      esp8266.write(c);
    }
  }
}

void sendATCommand(String cmd, int timeout, boolean debug) {
  esp8266.println(cmd);
  long int time = millis();
  while ((time + timeout) > millis()) {
    while (esp8266.available()) {
      char c = esp8266.read();
      if (debug) Serial.write(c);
    }
  }
}
