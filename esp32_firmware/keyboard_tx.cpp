#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <RF24.h>

#define CE_PIN 4
#define CSN_PIN 5

RF24 radio(CE_PIN, CSN_PIN);
const byte address[5] = {0x30, 0x30, 0x30, 0x30, 0x31};
char tx_buf[32];

void setup() {
  Serial.begin(115200);

  WiFi.mode(WIFI_OFF);
  btStop();
  SPI.begin();

  if (!radio.begin()) {
    Serial.println("NRF24L01 init FAILED");
    while (1) delay(1000);
  }

  radio.setChannel(108);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setCRCLength(RF24_CRC_16);
  radio.setPayloadSize(32);
  radio.setAutoAck(false);

  radio.openWritingPipe(address);
  radio.stopListening();

  Serial.println("Keyboard TX ready. Send chars over serial.");
}

void loop() {
  if (Serial.available()) {
    memset(tx_buf, 0, 32);
    uint8_t count = 0;
    delay(20);
    while (Serial.available() && count < 31) {
      tx_buf[count + 1] = Serial.read();
      count++;
    }
    tx_buf[0] = count;
    radio.write(&tx_buf, 32);
  }
}
