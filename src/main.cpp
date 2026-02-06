#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>

// Pin definitions for ESP32-DevKitC V4
#define CE_PIN 4
#define CSN_PIN 5

// Create RF24 object
RF24 radio(CE_PIN, CSN_PIN);

// Pipe address for communication (must match receiver)
const uint8_t address[6] = "00001";

// Message structure
struct PayloadStruct {
  unsigned long counter;
  char message[24];
};

PayloadStruct payload;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== NRF24L01 Transmitter ===");
  Serial.println("Initializing...");

  // Initialize SPI bus (ESP32 VSPI by default)
  SPI.begin();

  // Initialize radio
  if (!radio.begin()) {
    Serial.println("ERROR: Radio hardware not responding!");
    Serial.println("Check wiring:");
    Serial.println("  CE  -> GPIO 4");
    Serial.println("  CSN -> GPIO 5");
    Serial.println("  SCK -> GPIO 18");
    Serial.println("  MOSI-> GPIO 23");
    Serial.println("  MISO-> GPIO 19");
    while (1) {
      delay(1000);
    }
  }

  // Configure radio
  radio.setPALevel(RF24_PA_LOW);        // Power level (LOW for testing, MAX for range)
  radio.setDataRate(RF24_250KBPS);      // Data rate (lower = more reliable)
  radio.setChannel(76);                  // Channel (0-125)
  radio.openWritingPipe(address);       // Set the transmit pipe
  radio.stopListening();                 // Set as transmitter

  Serial.println("Radio initialized successfully!");
  Serial.println();

  // Print radio details for verification
  Serial.println("=== Radio Configuration ===");
  Serial.print("Channel: ");
  Serial.println(radio.getChannel());
  Serial.print("Data Rate: ");
  Serial.println(radio.getDataRate());
  Serial.print("PA Level: ");
  Serial.println(radio.getPALevel());
  Serial.print("Pipe Address: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(address[i], HEX);
  }
  Serial.println();

  // Print detailed status
  radio.printDetails();
  Serial.println("===========================");
  Serial.println();

  // Initialize payload
  payload.counter = 0;
  strcpy(payload.message, "Hello World");

  Serial.println("Starting transmission...");
  Serial.println();
}

void loop() {
  // Prepare payload
  payload.counter++;

  // Transmit
  Serial.print("Sending #");
  Serial.print(payload.counter);
  Serial.print(": ");
  Serial.print(payload.message);
  Serial.print(" ... ");

  bool success = radio.write(&payload, sizeof(payload));

  if (success) {
    Serial.println("SUCCESS");
  } else {
    Serial.println("FAILED");
  }

  // Wait before next transmission
  delay(1000);
}
