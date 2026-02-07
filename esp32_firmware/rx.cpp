#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <RF24.h>

#define CE_PIN 4
#define CSN_PIN 5
#define LED_PIN 21

RF24 radio(CE_PIN, CSN_PIN);

const byte address[5] = {0x30, 0x30, 0x30, 0x30, 0x31};

char rx_buf[32];
uint32_t rx_count = 0;

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  delay(1000);

  WiFi.mode(WIFI_OFF);
  btStop();
  SPI.begin();

  if (!radio.begin()) {
    Serial.println("NRF24L01 init FAILED");
    while (1) { digitalWrite(LED_PIN, !digitalRead(LED_PIN)); delay(100); }
  }

  radio.setChannel(108);
  radio.setDataRate(RF24_1MBPS);
  radio.setPALevel(RF24_PA_MAX);
  radio.setCRCLength(RF24_CRC_16);
  radio.setPayloadSize(32);
  radio.setAutoAck(false);

  radio.openReadingPipe(1, address);
  radio.startListening();

  Serial.println("ESP32 RX ready - CH:108 1Mbps");
  radio.printPrettyDetails();

  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, HIGH); delay(100);
    digitalWrite(LED_PIN, LOW); delay(100);
  }
}

void loop() {
  if (radio.available()) {
    radio.read(&rx_buf, 32);
    rx_count++;

    uint16_t counter = ((uint8_t)rx_buf[0] << 8) | (uint8_t)rx_buf[1];
    Serial.printf("#%lu | STC counter: %u | ", rx_count, counter);
    for (int i = 0; i < 8; i++) {
      Serial.printf("%02X ", (uint8_t)rx_buf[i]);
    }
    Serial.println();

    digitalWrite(LED_PIN, HIGH);
    delay(200);
    digitalWrite(LED_PIN, LOW);
  }
}
