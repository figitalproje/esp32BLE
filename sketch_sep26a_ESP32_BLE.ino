#include <EEPROM.h>
#include <Ed25519.h>
#include <ESP32Servo.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

const int PRIVATE_KEY_SIZE = 32;
const int PUBLIC_KEY_SIZE = 32;
const int PRIVATE_KEY_ADDR = 0;
const int PUBLIC_KEY_ADDR = PRIVATE_KEY_ADDR + PRIVATE_KEY_SIZE;

uint8_t privateKey[PRIVATE_KEY_SIZE];
uint8_t publicKey[PUBLIC_KEY_SIZE];
Servo myservo;

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;

const char* SERVICE_UUID = "2b5dc6e0-2727-453f-b69d-7fa0f50c7705";
const char* CHARACTERISTIC_UUID = "0abd0e1d-b033-486d-bd6f-a9423c091146";

void clearEEPROM() {
    for (int i = 0; i < EEPROM.length(); i++) {
        EEPROM.write(i, 0xFF);
    }
}

void asd() {
  String elsalla;
  int sayac = 0;
  for (int i = 0; i < EEPROM.length(); i++) {
    elsalla = EEPROM.read(i + 1);
    Serial.println(elsalla);
  }
}

void HandShake() {
  String elsalla;
  for (int i = 0; i < PUBLIC_KEY_SIZE; i++) {
    if (publicKey[i] < 16) {
      elsalla += '0';
    }
    elsalla += String(publicKey[i], HEX);
  }
  Serial.print(elsalla);
}

void HandShake0() {
  String elsalla = "Private Key: ";
  for (int i = 0; i < PRIVATE_KEY_SIZE; i++) {
    if (privateKey[i] < 16) {
      elsalla += '0';
    }
    elsalla += String(privateKey[i], HEX);
  }
  Serial.println(elsalla);
}

void setup() {
  //clearEEPROM();
  EEPROM.begin(512);
  bool keyExists = true;
  for (int i = 0; i < PRIVATE_KEY_SIZE; i++) {
    if (EEPROM.read(PRIVATE_KEY_ADDR + i) == 0xFF) {
      keyExists = false;
      break;
    }
  }

  if (!keyExists) {
    Ed25519::generatePrivateKey(privateKey);
    Ed25519::derivePublicKey(publicKey, privateKey);
    for (int i = 0; i < PRIVATE_KEY_SIZE; i++) {
      EEPROM.put(PRIVATE_KEY_ADDR + i, privateKey[i]);
      EEPROM.commit();
    }
    for (int i = 0; i < PUBLIC_KEY_SIZE; i++) {
      EEPROM.put(PUBLIC_KEY_ADDR + i, publicKey[i]);
      EEPROM.commit();
    }
  } else {
    for (int i = 0; i < PRIVATE_KEY_SIZE; i++) {
      privateKey[i] = EEPROM.read(PRIVATE_KEY_ADDR + i);
    }
    for (int i = 0; i < PUBLIC_KEY_SIZE; i++) {
      publicKey[i] = EEPROM.read(PUBLIC_KEY_ADDR + i);
    }
  }

  myservo.attach(13);
  Serial.begin(9600);

  // BLE aygıtını başlat
  BLEDevice::init("ESP32");
  pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                     CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_READ   |
                     BLECharacteristic::PROPERTY_WRITE  |
                     BLECharacteristic::PROPERTY_NOTIFY |
                     BLECharacteristic::PROPERTY_INDICATE
                   );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void loop() {
  if (deviceConnected) {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0) {
      char command = rxValue[0];
      if (command == 'H') {
        HandShake();
      }
      if (command == 'P') {
        HandShake0();
      }
      if (command == 'A') {
        asd();
      }
      if (command == 'S') {
        String posStr = rxValue.substr(1).c_str();
        int pos = posStr.toInt();
        myservo.write(pos);
      }
      if (command == 'M') {
        String mesaj = rxValue.substr(1).c_str();
        uint8_t signature[64];
        Ed25519::sign(signature, privateKey, publicKey, mesaj.c_str(), mesaj.length());
        String signatureString;
        for (int i = 0; i < 64; i++) {
          if (signature[i] < 16) {
            signatureString += '0';
          }
          signatureString += String(signature[i], HEX);
        }
        pCharacteristic->setValue(signatureString.c_str());
        pCharacteristic->notify();
      }
    }
  }
}
