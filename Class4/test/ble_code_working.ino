#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ESP32Servo.h>

Servo servo;
#define SERVO_PIN 1
#define TRIG 21
#define ECHO 20

// Nordic UART Service UUIDs (The standard for Serial-over-BLE)
#define SERVICE_UUID           "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_RX "6e400002-b5a3-f393-e0a9-e50e24dcca9e"
#define CHARACTERISTIC_UUID_TX "6e400003-b5a3-f393-e0a9-e50e24dcca9e"

BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
String receivedData = ""; 
bool isLocked = true;
bool useAuto = true;

// Helper to send data to Phone and Serial Monitor
void sendUpdate(String msg) {
  Serial.println(msg);
  if (deviceConnected) {
    pTxCharacteristic->setValue(msg.c_str());
    pTxCharacteristic->notify();
  }
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) { deviceConnected = true; };
    void onDisconnect(BLEServer* pServer) { 
      deviceConnected = false;
      BLEDevice::startAdvertising(); 
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String rxValue = pCharacteristic->getValue(); 
      if (rxValue.length() > 0) {
        receivedData = rxValue; 
      }
    }
};

void setup() {
  Serial.begin(115200);
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  
  ESP32PWM::allocateTimer(0);
  servo.setPeriodHertz(50);
  servo.attach(SERVO_PIN, 500, 2400);
  servo.write(0); 

  BLEDevice::init("S3-SmartLock");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("Waiting for phone connection...");
}

void loop() {
  // Handle Bluetooth Input
  if (receivedData.length() > 0) {
    char cmd = receivedData[0];
    if (cmd == 'U' || cmd == 'u') { 
      servo.write(90); isLocked = false; useAuto = true; 
      sendUpdate(">>> UNLOCKED"); 
    }
    else if (cmd == 'L' || cmd == 'l') { 
      servo.write(0); isLocked = true; useAuto = false; 
      sendUpdate(">>> LOCKED (Auto OFF)"); 
    }
    else if (cmd == 'A' || cmd == 'a') { 
      useAuto = true; 
      sendUpdate(">>> AUTO MODE ON"); 
    }
    receivedData = ""; 
  }

  // Blocking Logic (Your original requirement)
  if (useAuto) {
    long distance = getDistance();
    if (distance < 30 && distance > 3 && isLocked) {
      servo.write(90);
      isLocked = false;
      sendUpdate("AUTO: Entry detected - UNLOCKED");
    }
    if (!isLocked && distance >= 30) {
      sendUpdate("AUTO: No one detected. Locking in 10s...");
      for (int i = 10; i > 0; i--) { 
        sendUpdate("Locking in: " + String(i) + "s");
        delay(1000); 
      }
      servo.write(0);
      isLocked = true;
      sendUpdate("AUTO: LOCKED");
    }
  }
  delay(100);
}

long getDistance() {
  digitalWrite(TRIG, LOW); delayMicroseconds(2);
  digitalWrite(TRIG, HIGH); delayMicroseconds(10);
  digitalWrite(TRIG, LOW);
  return pulseIn(ECHO, HIGH, 30000) * 0.034 / 2;
}
