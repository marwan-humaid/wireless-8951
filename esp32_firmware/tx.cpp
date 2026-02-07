#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <RF24.h>

#define CE_PIN 4
#define CSN_PIN 5
#define LED_PIN 21

RF24 radio(CE_PIN, CSN_PIN);

const byte address[5] = {0x30, 0x30, 0x30, 0x30, 0x31};

uint32_t tx_count = 0;
char tx_buf[32];

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);

  // Solid LED for 2s = board is alive
  digitalWrite(LED_PIN, HIGH);
  delay(2000);
  digitalWrite(LED_PIN, LOW);
  delay(500);

  WiFi.mode(WIFI_OFF);
  btStop();
  SPI.begin();

  if (!radio.begin()) {
    // Rapid blink = radio init failed
    while (1) { digitalWrite(LED_PIN, !digitalRead(LED_PIN)); delay(100); }
  }

  radio.setChannel(108);
  radio.setPALevel(RF24_PA_MAX);
  radio.setCRCLength(RF24_CRC_16);
  radio.setPayloadSize(32);
  radio.setAutoAck(false);

  radio.openWritingPipe(address);
  radio.stopListening();

  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH); delay(100);
    digitalWrite(LED_PIN, LOW); delay(100);
  }
}

void loop() {
  memset(tx_buf, 0, 32);
  tx_buf[0] = (tx_count >> 8) & 0xFF;
  tx_buf[1] = tx_count & 0xFF;
  tx_buf[2] = 0xAA;
  tx_buf[3] = 0x55;

  radio.write(&tx_buf, 32);
  tx_count++;

  digitalWrite(LED_PIN, HIGH);
  delay(50);
  digitalWrite(LED_PIN, LOW);
  delay(950);
}
