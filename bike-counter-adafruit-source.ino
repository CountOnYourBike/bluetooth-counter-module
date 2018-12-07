/**
 * Engineering Thesis project:
 * 
 * title:       Wireless bike computer using mobile devices with Android system
 * authors:     Rafał Stencel, Karol Kuppe, Aleksander Kondrat
 * supervisor:  dr inż. Krzysztof Bikonis
 * university:  Gdansk University of Technology
 * department:  Faculty of Electronics, Telecommunications and Informatics
 * last update: 7.12.2018 by Rafał Stencel
 * 
 * source code based on Neil Kolban BLE_notify example and libraries from: https://github.com/nkolban/ESP32_BLE_Arduino
 */
 
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <time.h>

#define SERVICE_UUID BLEUUID((uint16_t)0x1816)
#define MIN_WHEEL_EVENT_TIME_MS 200
#define REV_DETECTION_THRESHOLD 30

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

bool deviceConnected = false;
bool oldDeviceConnected = false;

int8_t msmt = 0;
bool wheelRevDetected = false;
uint32_t cumWheelRevs = 0;

clock_t startTime = NULL;
uint16_t wheelEventTime;

byte flags = 0b00000001;
byte packet[7] = { flags,  0, 0, 0, 0,  0, 0 };

BLECharacteristic CSCMeasurementCharacteristic(BLEUUID((uint16_t)0x2A5B), BLECharacteristic::PROPERTY_NOTIFY);
BLECharacteristic CSCFeatureCharacteristic(BLEUUID((uint16_t)0x2A5C), BLECharacteristic::PROPERTY_READ);
BLECharacteristic SensorLocationCharacteristic(BLEUUID((uint16_t)0x2A5D), BLECharacteristic::PROPERTY_READ);
BLECharacteristic SCControlPointCharacteristic(BLEUUID((uint16_t)0x2A55), BLECharacteristic::PROPERTY_WRITE);

class BikeCounterServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void setup() {
  BLEDevice::init("CountOnYourBike");

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BikeCounterServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  pService->addCharacteristic(&CSCMeasurementCharacteristic);
  CSCMeasurementCharacteristic.addDescriptor(new BLE2902());

  pService->addCharacteristic(&CSCFeatureCharacteristic);
  CSCFeatureCharacteristic.addDescriptor(new BLE2902());

  pService->addCharacteristic(&SensorLocationCharacteristic);
  SensorLocationCharacteristic.addDescriptor(new BLE2902());

  pService->addCharacteristic(&SCControlPointCharacteristic);
  SCControlPointCharacteristic.addDescriptor(new BLE2902());

  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
  pService->start();
  pServer->getAdvertising()->start();
}

void loop() {
  if (deviceConnected) {
    msmt = hallRead();
    
    if ((msmt > REV_DETECTION_THRESHOLD) && !wheelRevDetected) {
      wheelRevDetected = true;
      cumWheelRevs++;

      if (startTime == NULL) {
        startTime = clock();
      }
      
      wheelEventTime = (clock() - startTime) * 1000 / CLOCKS_PER_SEC;

      if (wheelEventTime > MIN_WHEEL_EVENT_TIME_MS) {
        convertAndSetPacket(packet, cumWheelRevs, wheelEventTime);
        CSCMeasurementCharacteristic.setValue(packet, 7);
        CSCMeasurementCharacteristic.notify();

        startTime = clock();
        cumWheelRevs = 0;
      }
    }
    else if ((msmt < 20) && wheelRevDetected) {
      wheelRevDetected = false;
    }
  }
  if (!deviceConnected && oldDeviceConnected) {
    delay(500);
    pServer->startAdvertising();
    oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}

void convertAndSetPacket(byte *packet, uint32_t cumWheelRevs, uint16_t wheelEventTime) {
  packet[1] = (byte)cumWheelRevs;
  packet[2] = (byte)(cumWheelRevs >> 8);
  packet[3] = (byte)(cumWheelRevs >> 16);
  packet[4] = (byte)(cumWheelRevs >> 24);
  packet[5] = (byte)wheelEventTime;
  packet[6] = (byte)(wheelEventTime >> 8);
}
