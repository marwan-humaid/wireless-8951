#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <RF24.h>

#define CE_PIN 4
#define CSN_PIN 5

RF24 radio(CE_PIN, CSN_PIN);
const byte address[5] = {0x30, 0x30, 0x30, 0x30, 0x31};
char tx_buf[32];
uint8_t seq_num = 0;

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
    while (Serial.available() && count < 30) {
      tx_buf[count + 1] = Serial.read();
      count++;
    }
    tx_buf[0] = count;
    tx_buf[31] = seq_num;  /* sequence number for dedup */

    Serial.printf("TX seq=%02X cnt=%d data=", seq_num, count);
    for (uint8_t j = 0; j < count; j++) {
      char c = tx_buf[j + 1];
      if (c >= 0x20 && c <= 0x7E) Serial.print(c);
      else Serial.printf("[%02X]", (uint8_t)c);
    }
    Serial.println();

    /* Send 5 times for redundancy (no auto-ack, clone modules) */
    for (uint8_t r = 0; r < 5; r++) {
      radio.write(&tx_buf, 32);
      if (r < 4) delayMicroseconds(1500);
    }
    seq_num++;
  }
}
