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
  radio.printPrettyDetails();

  // Send test message on boot (repeat to ensure STC is in RX mode)
  delay(5000);
  for (int t = 0; t < 3; t++) {
    const char *msg = "Hello STC!";
    uint8_t len = strlen(msg);
    memset(tx_buf, 0, 32);
    tx_buf[0] = len;
    memcpy(&tx_buf[1], msg, len);
    radio.write(&tx_buf, 32);
    delay(500);
  }
  Serial.println("Sent test: Hello STC! (x3)");
}

uint32_t ping_count = 0;

void loop() {
  if (Serial.available()) {
    memset(tx_buf, 0, 32);
    uint8_t count = 0;
    delay(20);  // small window to batch chars arriving together
    while (Serial.available() && count < 31) {
      tx_buf[count + 1] = Serial.read();
      count++;
    }
    tx_buf[0] = count;

    radio.write(&tx_buf, 32);
  } else {
    // Send periodic ping every 2 seconds so we can verify the link
    static unsigned long last_ping = 0;
    if (millis() - last_ping > 2000) {
      last_ping = millis();
      ping_count++;
      char msg[16];
      sprintf(msg, "ping %lu", ping_count);
      uint8_t len = strlen(msg);
      memset(tx_buf, 0, 32);
      tx_buf[0] = len;
      memcpy(&tx_buf[1], msg, len);
      radio.write(&tx_buf, 32);
      Serial.printf("Sent: %s\n", msg);
    }
  }
}
