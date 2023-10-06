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
BLECharacteristic* pHandshakeCharacteristic = NULL;
BLECharacteristic* pSignatureCharacteristic = NULL;
bool deviceConnected = false;

const char* SERVICE_UUID = "2b5dc6e0-2727-453f-b69d-7fa0f50c7705";
const char* HANDSHAKE_CHARACTERISTIC_UUID = "0abd0e1d-b033-486d-bd6f-a9423c091146";
const char* SIGNATURE_CHARACTERISTIC_UUID ="c029a5f5-03fe-40ff-a379-896dfa677240";


void HandShake() {
  String elsalla;
  for (int i = 0; i < PUBLIC_KEY_SIZE; i++) {
    if (publicKey[i] < 16) {
      elsalla += '0';
    }
    elsalla += String(publicKey[i], HEX);
  }
  pHandshakeCharacteristic->setValue(elsalla.c_str());
  pHandshakeCharacteristic->notify();
}


// UUID'ler
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      HandShake();  // Cihaz bağlandığında HandShake fonksiyonunu otomatik olarak çağır
    }
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};



class SignatureCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string value = pCharacteristic->getValue();
      if (value.length() > 0) {
        // Gelen veriyi imzala
        uint8_t signature[64];
        Ed25519::sign(signature, privateKey, publicKey, (uint8_t*)value.c_str(), value.length());
        
        // İmzayı karakteristiğin değeri olarak ayarla
        pCharacteristic->setValue(signature, 64);
        
        // İmzayı istemciye bildir
        pCharacteristic->notify();
      }
    }
};

class HandshakeCallbacks: public BLECharacteristicCallbacks {
     
      HandShake();  // Cihaz bağlandığında HandShake fonksiyonunu otomatik olarak çağır
   
    
};

void setup() {
  Serial.begin(115200);

  // Ed25519 anahtar çiftini oluştur
  Ed25519::generatePrivateKey(privateKey);
  Ed25519::derivePublicKey(publicKey, privateKey);

  // BLE aygıtını başlat
  BLEDevice::init("ESP32_BLE");
  BLEServer* pServer = BLEDevice::createServer();
  BLEService* pService = pServer->createService(SERVICE_UUID);

  // İmza karakteristiğini oluştur
  pSignatureCharacteristic = pService->createCharacteristic(
                     SIGNATURE_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_READ   |
                     BLECharacteristic::PROPERTY_WRITE  |
                     BLECharacteristic::PROPERTY_NOTIFY
                   );
  pSignatureCharacteristic->setCallbacks(new SignatureCallbacks());
  pSignatureCharacteristic->addDescriptor(new BLE2902());

  // Handshake karakteristiğini oluştur
  pHandshakeCharacteristic = pService->createCharacteristic(
                     HANDSHAKE_CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_READ
                   );
  pHandshakeCharacteristic->setCallbacks(new HandshakeCallbacks());

  // Servisi başlat
  pService->start();

  // BLE reklamını başlat
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();
}

void loop() {
  // Ana döngüde özel bir işlem yapmıyoruz
}