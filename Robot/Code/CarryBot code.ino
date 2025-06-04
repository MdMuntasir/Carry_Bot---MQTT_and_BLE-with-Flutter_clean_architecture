#include <ArduinoJson.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h> 
#include <ESP32Servo.h>
#include "soc/rtc.h"
#include <HX711.h>

#define distanceTrig 33
#define distanceEcho 35
#define depthTrig 32
#define depthEcho 34
#define leftPin 14
#define rightPin 12
#define servoPin 13
#define dt 25
#define sck 26

#define motor1A 23  // Motor 1 forward
#define motor1B 22  // Motor 1 backward
#define motor2A 4  // Motor 2 forward
#define motor2B 15  // Motor 2 backward
#define motor3A 21  // Motor 3 forward
#define motor3B 19  // Motor 3 backward
#define motor4A 18  // Motor 4 forward
#define motor4B 5  // Motor 4 



int highSpeed = 255;  
int lowSpeed = 100;  

const int PWM_FREQ = 1000;  
const int PWM_RESOLUTION = 8;


// Constants for object following
#define MIN_DISTANCE 5.0     
#define MAX_DISTANCE 30.0     
#define MIN_DEPTH 2.0     
#define MAX_DEPTH 6.0
#define SERVO_STEP 5          
#define SERVO_MIN 0           
#define SERVO_MAX 180  



HX711 scale;
Servo servo;


BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define PASSKEY 123456
bool deviceConnected = false;

bool left = false;
bool right = false;
bool objectDetected = false;
int servoPastVal = 0;
int servoAngle = 90;

bool manualControl = false;









// Control the car's motors

void stopMotors() {
  ledcWrite(motor1A, 0); ledcWrite(motor1B, 0);
  ledcWrite(motor2A, 0); ledcWrite(motor2B, 0);
  ledcWrite(motor3A, 0); ledcWrite(motor3B, 0);
  ledcWrite(motor4A, 0); ledcWrite(motor4B, 0);
}

void setMotor(int fwdChannel, int bwdChannel, int speed, bool direction) {
  if (speed == 0) {
      ledcWrite(fwdChannel, 0);
      ledcWrite(bwdChannel, 0);
  } else if (direction) {
      ledcWrite(fwdChannel, speed);
      ledcWrite(bwdChannel, 0);
  } else {
      ledcWrite(fwdChannel, 0);
      ledcWrite(bwdChannel, speed);
  }
}


void stopCar(int time){
  delay(time);
  stopMotors();
}


void moveCar(String command) {
  command.trim();
  command.toLowerCase();
  Serial.print("Command: "); Serial.println(command);
  stopMotors();

  if (command == "front") {
      setMotor(motor1A, motor1B, highSpeed, true);
      setMotor(motor2A, motor2B, highSpeed, true);
      setMotor(motor3A, motor3B, highSpeed, true);
      setMotor(motor4A, motor4B, highSpeed, true);
      Serial.println("Moving forward");
  }
  else if (command == "back") {
      setMotor(motor1A, motor1B, highSpeed, false);
      setMotor(motor2A, motor2B, highSpeed, false);
      setMotor(motor3A, motor3B, highSpeed, false);
      setMotor(motor4A, motor4B, highSpeed, false);
      Serial.println("Moving backward");
  }
  else if (command == "right") {
      setMotor(motor1A, motor1B, highSpeed, true);
      setMotor(motor2A, motor2B, highSpeed, false);
      setMotor(motor3A, motor3B, highSpeed, true);
      setMotor(motor4A, motor4B, highSpeed, false);
      Serial.println("Turning right");
  }
  else if (command == "left") {
      setMotor(motor1A, motor1B, highSpeed, false);
      setMotor(motor2A, motor2B, highSpeed, true);
      setMotor(motor3A, motor3B, highSpeed, false);
      setMotor(motor4A, motor4B, highSpeed, true);
      Serial.println("Turning left");
  }
  else if (command == "fl") {
      setMotor(motor1A, motor1B, 0, true);      
      setMotor(motor2A, motor2B, highSpeed, true); 
      setMotor(motor3A, motor3B, 0, true);      
      setMotor(motor4A, motor4B, highSpeed, true);  
      Serial.println("Front-right diagonal");
  }
  else if (command == "fr") {
      setMotor(motor1A, motor1B, highSpeed, true);  
      setMotor(motor2A, motor2B, 0, true);      
      setMotor(motor3A, motor3B, highSpeed, true); 
      setMotor(motor4A, motor4B, 0, true);      
      Serial.println("Front-left diagonal");
  }
  else if (command == "bl") {
      setMotor(motor1A, motor1B, 0, false);     
      setMotor(motor2A, motor2B, highSpeed, false);
      setMotor(motor3A, motor3B, 0, false);      
      setMotor(motor4A, motor4B, highSpeed, false);
      Serial.println("Back-right reverse");
  }
  else if (command == "br") {
      setMotor(motor1A, motor1B, highSpeed, false);
      setMotor(motor2A, motor2B, 0, false);      
      setMotor(motor3A, motor3B, highSpeed, false);
      setMotor(motor4A, motor4B, 0, false);      
      Serial.println("Back-left reverse");
  }
  else if (command == "stop"){
    stopCar(50);
  }
  else {
      stopMotors();
      Serial.println("❌ Invalid command - Car stopped");
  }

  
}


// Send message as the robot
void sendMessage(const char* message){
        StaticJsonDocument<200> jsonDoc;
        jsonDoc["message"] = message;

        // Convert JSON to string
        String jsonString;
        serializeJson(jsonDoc, jsonString);

        // Send JSON over BLE if a device is connected
        if (deviceConnected) {
            pCharacteristic->setValue(jsonString.c_str());  
            pCharacteristic->notify();  
            Serial.print("📤 Sent JSON via BLE: ");
            Serial.println(jsonString);
        } else {
            Serial.println("⚠️ No BLE device connected.");
        }
}



// Side IR sensor detection
void sideDetector() {
  left = !digitalRead(leftPin);
  right = !digitalRead(rightPin);

  // Update servo angle based on IR sensor readings
  if (left && !right) {
    // Object detected on left, gradually turn servo left
    servoAngle = max(servoAngle - SERVO_STEP, SERVO_MIN);
    for(int i=0; i<5; i++)  
      moveCar("left");
  } else if (right && !left) {
    // Object detected on right, gradually turn servo right
    servoAngle = min(servoAngle + SERVO_STEP, SERVO_MAX);
    for(int i=0; i<5; i++)
      moveCar("right");
  }

  servo.write(servoAngle);
}

int autoPrePhase = 0;
int autoCurrentPhase = 0;

// Automatic object-following function
void autoFollowObject(float distance) {
  sideDetector();
  Serial.print(" | Distance: "); Serial.print(distance);
  Serial.print(" | Servo Angle: "); Serial.println(servoAngle);

  // Object too close: stop to avoid collision
  if (distance < MIN_DISTANCE) {
    autoCurrentPhase = 1;
    moveCar("stop");
    if(autoPrePhase != autoCurrentPhase){
      autoPrePhase = autoCurrentPhase;
      sendMessage("Too close, stopping");
    }
    
    return;
  }

  // Object within following distance range
  if (distance >= MIN_DISTANCE && distance <= MAX_DISTANCE) {
    // Check if the head is centered on the object
    if (servoAngle >= (90 - SERVO_STEP) && servoAngle <= (90 + SERVO_STEP)) {
      autoCurrentPhase = 2;
      // Head is centered, move the car forward
      moveCar("front");
      if(autoPrePhase != autoCurrentPhase){
        autoPrePhase = autoCurrentPhase;
        sendMessage("Head centered, moving forward"); 
      }
      
    } else {
      autoCurrentPhase = 3;
      // Head is not centered, stop the car and wait for sideDetector to center it
      moveCar("stop");
      if(autoPrePhase != autoCurrentPhase){
        autoPrePhase = autoCurrentPhase;
        sendMessage("Head not centered, stopping car to center head");
      }
      
    }
  }
  // Object lost: scan with servo
  else if (distance > MAX_DISTANCE) {
    autoCurrentPhase = 4;
    moveCar("stop");
    if(autoPrePhase != autoCurrentPhase){
      autoPrePhase = autoCurrentPhase;
      sendMessage("Object lost, scanning");
    }
    
    // Scan left or right based on last known direction
    if (servoAngle > servoPastVal && servoAngle < 180 || servoAngle<=0) {
      servoPastVal =  servoAngle;
      servoAngle += SERVO_STEP;
    } else {
      servoPastVal =  servoAngle;
      servoAngle -= SERVO_STEP;
    }
    servo.write(servoAngle);
  }
}





// Initialize Load Sensor

void loadSensorInit(){
  scale.begin(dt, sck);
  while(!scale.is_ready()){
    Serial.println("Waiting for HX711 to be ready...");
    delay(100);
  }
  scale.tare();

  // scale.set_scale(100);
}


// Initialize Servo Motor
void servoInit(){
  servo.attach(servoPin);
  servo.write(servoAngle);
}




void parseJson(String jsonString) {
  StaticJsonDocument<200> doc;

  DeserializationError error = deserializeJson(doc, jsonString);
  
  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char* move = doc["move"];
  const char* mode = doc["mode"];
  if (move) {
    float depth = getDepth();
    float distance = getDistance();

    if(move == "front"){
      if(depth > MAX_DEPTH || distance < MIN_DISTANCE) {
        if(depth> MAX_DEPTH ){
          sendMessage("I can't cross this depth");
        }
        if(distance< MIN_DISTANCE){
          sendMessage("I will crash if I go further");
        }
        stopCar(10); 
      }
      else
      moveCar(move);
    }
    else
    moveCar(move);
  } 
  
  else if(mode){
    stopCar(10);
    manualControl = strcmp(mode,"manual") == 0;
    Serial.print("Mode: ");
    Serial.println(mode);
    manualControl?
    sendMessage("Manual Mode") : sendMessage("Auto Mode");
  }

  else {
    Serial.println("Invalid keys in JSON");
  }
}


// BLE Connection Setup

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    String value = pCharacteristic->getValue();
    if (value.length() > 0) {
      String jsonString = String(value.c_str());
      Serial.print("Received JSON: ");
      Serial.println(jsonString);

      parseJson(jsonString);
    }
  }
};


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("Device Connected!");
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("Device Disconnected!");
        BLEDevice::startAdvertising();  
    }
};



class ServerCallback: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      Serial.println(" - ServerCallback - onConnect");
    };


    void onDisconnect(BLEServer* pServer) {
      Serial.println(" - ServerCallback - onDisconnect");
    }
};


class SecurityCallback : public BLESecurityCallbacks {


  uint32_t onPassKeyRequest(){
    return 000000;
  }


  void onPassKeyNotify(uint32_t pass_key){}


  bool onConfirmPIN(uint32_t pass_key){
    vTaskDelay(5000);
    return true;
  }


  bool onSecurityRequest(){
    return true;
  }


  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl){
    if(cmpl.success){
      Serial.println("   - SecurityCallback - Authentication Success");       
      deviceConnected = true;
    }else{
      Serial.println("   - SecurityCallback - Authentication Failure*");
      pServer->removePeerDevice(pServer->getConnId(), true);
      deviceConnected = false;
    }
    BLEDevice::startAdvertising();
  }
};


void bleSecurity(){
  esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
  esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;          
  uint8_t key_size = 16;     
  uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  uint32_t passkey = PASSKEY;
  uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
}


void bleInit(){
  BLEDevice::init("Carry Bot");
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  BLEDevice::setSecurityCallbacks(new SecurityCallback());


  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallback());


  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY 
                    );


  pCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());
  pService->start();


  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();


  bleSecurity();
}







// Send data through BLE

const char* getSituation(int low, int high, float warn, int val){
    if (val < low || val > high) return "R";  // Red (Danger)
    if ((val > low && val < low + warn) || (val < high && val > high - warn)) return "Y";  // Yellow (Warning)
    return "G";  // Green (Safe)
}

void sensorJson(JsonObject sensor, const char* name, float value, const char* situation) {
    sensor["name"] = name;
    sensor["value"] = static_cast<float>(value);
    sensor["situation"] = situation;
}

float getDistance() {
    digitalWrite(distanceTrig, LOW);
    delayMicroseconds(2);
    digitalWrite(distanceTrig, HIGH);
    delayMicroseconds(8);
    digitalWrite(distanceTrig, LOW);

    long duration = pulseIn(distanceEcho, HIGH);
    float distance = duration * 0.034 / 2; 
    return distance;
}

float getDepth(){
    digitalWrite(depthTrig, LOW);
    delayMicroseconds(2);
    digitalWrite(depthTrig, HIGH);
    delayMicroseconds(5);
    digitalWrite(depthTrig, LOW);

    long duration = pulseIn(depthEcho, HIGH);
    float depth = duration * 0.034 / 2; // Convert to cm
    return depth;
}

float getWeight(){
  if (scale.is_ready()) {
    long reading = scale.get_units(3);
    return reading;
  } 
  else {
    return 0.0;
  }
}

void sendSensorData(float distance, float depth){
  StaticJsonDocument<300> jsonArray;
    JsonArray data = jsonArray.createNestedArray("sensors");

    float weight =  getWeight();

    
    JsonObject sensor1 = data.createNestedObject();
    sensorJson(sensor1, "Distance Sensor",distance,getSituation(MIN_DISTANCE, MAX_DISTANCE, 5, distance));

    JsonObject sensor2 = data.createNestedObject(); 
    sensorJson(sensor2, "Depth Sensor",depth,getSituation(MIN_DEPTH, MAX_DEPTH, 0, depth));

    JsonObject sensor3 = data.createNestedObject(); 
    sensorJson(sensor3, "Weight Sensor",weight,getSituation(0,10000,500,weight));

    char buffer[512];  
    serializeJson(jsonArray, buffer);

    

    if (deviceConnected) {
        Serial.print("📤 Sending JSON List: ");
        Serial.println(buffer);

        pCharacteristic->setValue(buffer);
        pCharacteristic->notify();
    }
}




void testMotors() {
    Serial.println("Testing Wheel 1 Forward");
    digitalWrite(motor1A, HIGH); digitalWrite(motor1B, LOW);
    delay(1000);
    stopMotors();
    delay(500);

    Serial.println("Testing Wheel 1 Backward");
    digitalWrite(motor1A, LOW); digitalWrite(motor1B, HIGH);
    delay(1000);
    stopMotors();
    delay(500);

    Serial.println("Testing Wheel 2 Forward");
    digitalWrite(motor2A, HIGH); digitalWrite(motor2B, LOW);
    delay(1000);
    stopMotors();
    delay(500);


    Serial.println("Testing Wheel 2 Backward");
    digitalWrite(motor2A, LOW); digitalWrite(motor2B, HIGH);
    delay(1000);
    stopMotors();
    delay(500);


    Serial.println("Testing Wheel 3 Forward");
    digitalWrite(motor3A, HIGH); digitalWrite(motor3B, LOW);
    delay(1000);
    stopMotors();
    delay(500);

    Serial.println("Testing Wheel 3 Backward");
    digitalWrite(motor3A, LOW); digitalWrite(motor3B, HIGH);
    delay(1000);
    stopMotors();
    delay(500);

    Serial.println("Testing Wheel 4 Forward");
    digitalWrite(motor4A, HIGH); digitalWrite(motor4B, LOW);
    delay(1000);
    stopMotors();
    delay(500);


    Serial.println("Testing Wheel 4 Backward");
    digitalWrite(motor4A, LOW); digitalWrite(motor4B, HIGH);
    delay(1000);
    stopMotors();
    delay(500);
  
}






// Initialize pins

void pinInit(){
  //Distance measuring Sonar Sensor
  pinMode(distanceTrig, OUTPUT);
  pinMode(distanceEcho, INPUT);

  //Depth measuring Sonar Sensor
  pinMode(depthTrig, OUTPUT);
  pinMode(depthEcho, INPUT);

  //Both side IR Sensors
  pinMode(leftPin, INPUT_PULLUP);
  pinMode(rightPin, INPUT_PULLUP);

  //For Motors
  ledcAttach(motor1A, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(motor1B, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(motor2A, PWM_FREQ, PWM_RESOLUTION); 
  ledcAttach(motor2B, PWM_FREQ, PWM_RESOLUTION); 
  ledcAttach(motor3A, PWM_FREQ, PWM_RESOLUTION); 
  ledcAttach(motor3B, PWM_FREQ, PWM_RESOLUTION);
  ledcAttach(motor4A, PWM_FREQ, PWM_RESOLUTION); 
  ledcAttach(motor4B, PWM_FREQ, PWM_RESOLUTION); 
  stopMotors();


}



void setup() {
  Serial.begin(115200);
  pinInit();
  bleInit();
  loadSensorInit();
  servoInit();
}


int sensorSendDelay = 10;
int objectFollowDelay = 2;



void loop() {
  if(deviceConnected){
    float depth = getDepth();
    float distance = getDistance();

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.print(" | Depth: ");
    Serial.println(depth);


    if (!manualControl) {
      if(objectFollowDelay < 2)
        objectFollowDelay++;
      
      else{
        autoFollowObject(distance);
        objectFollowDelay = 1;
      }
    }


    if(sensorSendDelay < 5){
      sensorSendDelay++;
    }else{
      sendSensorData(distance, depth);
      sensorSendDelay = 1;
    }
    
    delay(10);
  
  }



}
