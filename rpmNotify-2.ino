/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 4fafc201-1fb5-459e-8fcc-c5c9c331914b
   And has a characteristic of: beb5483e-36e1-4688-b7f5-ea07361b26a8

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   A connect hander associated with the server starts a background task that performs notification
   every couple of seconds.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
uint32_t cumulativeWheelRevolutions = 0;
int8_t measurement = 0;
int8_t lastMeasurement = 10; //initial value

byte flags = 0b00000001;
byte packet[7] = { flags, 0, 0, 0, 0, 0, 0 };


// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        BLEUUID((uint16_t)0x1816)
BLECharacteristic CSCMeasurementCharacteristic(BLEUUID((uint16_t)0x2A5B), BLECharacteristic::PROPERTY_NOTIFY);
BLECharacteristic CSCFeatureCharacteristic(BLEUUID((uint16_t)0x2A5C), BLECharacteristic::PROPERTY_READ);
BLECharacteristic SensorLocationCharacteristic(BLEUUID((uint16_t)0x2A5D), BLECharacteristic::PROPERTY_READ);
BLECharacteristic SCControlPointCharacteristic(BLEUUID((uint16_t)0x2A55), BLECharacteristic::PROPERTY_WRITE);
//BLEDescriptor ClientCharacteristicConfigurationDescriptor(BLEUUID((uint16_t)0x2902);


class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};



void setup() {
  Serial.begin(115200);

  // Create the BLE Device
  BLEDevice::init("MyESP32");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pService->addCharacteristic(&CSCMeasurementCharacteristic);
  CSCMeasurementCharacteristic.addDescriptor(new BLE2902());

  pService->addCharacteristic(&CSCFeatureCharacteristic);
  CSCFeatureCharacteristic.addDescriptor(new BLE2902());

  pService->addCharacteristic(&SensorLocationCharacteristic);
  SensorLocationCharacteristic.addDescriptor(new BLE2902());

  pService->addCharacteristic(&SCControlPointCharacteristic);
  SCControlPointCharacteristic.addDescriptor(new BLE2902());

  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

void loop() {
    // notify changed value
    if (deviceConnected) {
        lastMeasurement = measurement;
        measurement = hallRead();
        
        if (1){//(measurement < 10 || measurement > 25) && (lastMeasurement >= 10 && lastMeasurement <= 25)) {
          Serial.print("Last measurement = ");
          Serial.println(lastMeasurement);
          Serial.print("Measurement = ");
          Serial.println(measurement);
          
          cumulativeWheelRevolutions++;
          Serial.println(cumulativeWheelRevolutions);
          Serial.println();
          
          packet[1] = measurement;//cumulativeWheelRevolutions;
          CSCMeasurementCharacteristic.setValue(packet, FORMAT_UINT32);
          CSCMeasurementCharacteristic.notify();
        }
        
        delay(100); // bluetooth stack will go into congestion, if too many packets are sent
    }
    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}
