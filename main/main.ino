#define PUMP_IN 2
#define VALVE_1_IN 4
#define VALVE_2_IN 5
#define VALVE_3_IN 6
#define WORK_LAMP 7
#define STOP_LAMP 8

int przeslanyBajt = 0;
String stringFromPort = "";

int minValHumidity = 240;
int maxValHumidity = 600;

void setup() {
  Serial.begin(9600);
  Serial.println("Start!");

  pinMode(PUMP_IN, OUTPUT);
  pinMode(VALVE_1_IN, OUTPUT);
  pinMode(VALVE_2_IN, OUTPUT);
  pinMode(VALVE_3_IN, OUTPUT);
  pinMode(WORK_LAMP, OUTPUT);
  pinMode(STOP_LAMP, OUTPUT);

  digitalWrite(PUMP_IN, LOW);    // Wyłącz pompę
  digitalWrite(VALVE_1_IN, LOW);
  digitalWrite(VALVE_2_IN, LOW);
  digitalWrite(VALVE_3_IN, LOW);
  }
void loop() {
  if (Serial.available() > 0) {
    stringFromPort = Serial.readStringUntil('\n');
    Serial.print("Otrzymanov \n");
    if (stringFromPort == "mode basic") {
      Serial.println("BasicMode is running");
      BasicMode();
    }else if(stringFromPort == "mode 1"){
      Serial.println("Mode 1 is running");
      ModeOne();
    }else if(stringFromPort == "stop"){
      Serial.println("StopMode is running!");
      StopMode();
    }
  }
}

void BasicMode(){
  SetWorkSignalisation();
  digitalWrite(VALVE_1_IN, HIGH);
  digitalWrite(VALVE_2_IN, HIGH);
  digitalWrite(VALVE_3_IN, HIGH);
  delay(5000);
  digitalWrite(PUMP_IN, HIGH);
}

void ModeOne(){
  SetWorkSignalisation();
  digitalWrite(VALVE_1_IN, HIGH);
  delay(5000);
  digitalWrite(PUMP_IN, HIGH);
}

void StopMode(){
  SetStopSignalisation();
  digitalWrite(PUMP_IN, LOW);
  delay(5000);
  digitalWrite(VALVE_1_IN, LOW);
  delay(5000);
  digitalWrite(VALVE_2_IN, LOW);
  digitalWrite(VALVE_3_IN, LOW);
}

void SetWorkSignalisation(){
  digitalWrite(WORK_LAMP, HIGH);
  digitalWrite(STOP_LAMP, LOW);
}

void SetStopSignalisation(){
  digitalWrite(WORK_LAMP, LOW);
  digitalWrite(STOP_LAMP, HIGH);
}


