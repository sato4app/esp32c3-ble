#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// BLEシリアル通信の標準的なUUID（Nordic UART Service）
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E" // Web → ESP32
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E" // ESP32 → Web

#define LED_PIN 8 // 動作確認用LEDのGPIO番号

BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
bool deviceConnected = false;

// 接続状態のコールバック
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      Serial.println("スマホと接続しました！");
    };
    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      Serial.println("スマホとの接続が切れました。");
      pServer->startAdvertising(); // 切断後に再検索可能にする
    }
};

// Webからデータを受信したときのコールバック
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String rxValue = pCharacteristic->getValue().c_str();
      if (rxValue.length() > 0) {
        Serial.print("Webから受信: ");
        Serial.println(rxValue);
      }
    }
};

void setup() {
  Serial.begin(115200);
  // ESP32-C3のUSBシリアル認識待ちを兼ねて、LEDを0.3秒間隔で10回点滅させる
  pinMode(LED_PIN, OUTPUT);
  for (int i = 0; i < 10; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(300);
    digitalWrite(LED_PIN, LOW);
    delay(300);
  }
  Serial.println("BLE起動中...");

  BLEDevice::init("ESP32-C3-BLE"); // スマホに表示される名前
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 送信用キャラクタリスティックの作成 (Notify)
  pTxCharacteristic = pService->createCharacteristic(
                        CHARACTERISTIC_UUID_TX,
                        BLECharacteristic::PROPERTY_NOTIFY
                      );
  pTxCharacteristic->addDescriptor(new BLE2902());

  // 受信用キャラクタリスティックの作成 (Write)
  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                       BLECharacteristic::PROPERTY_WRITE
                     );
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();
  pServer->getAdvertising()->start();
  Serial.println("準備完了！スマホからの接続を待っています。");
}

void loop() {
  // シリアルモニタから入力された文字をWebへ送信
  if (deviceConnected && Serial.available()) {
    String txValue = Serial.readStringUntil('\n');
    txValue.trim(); // 余分な改行を削除
    if (txValue.length() > 0) {
      pTxCharacteristic->setValue(txValue.c_str());
      pTxCharacteristic->notify(); // Web側に通知
      Serial.print("Webへ送信: ");
      Serial.println(txValue);
    }
  }
  delay(50);
}