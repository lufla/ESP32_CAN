#include "../lib/ESP_CAN.h"
#include "../lib/ESP_CAN.cpp"

// Define the pins for CAN communication
const int CAN_RX_PIN = 5;
const int CAN_TX_PIN = 4;  // Not used for receiving, but required by library

// Initialize our CAN library
ESP_CAN can(CAN_RX_PIN, CAN_TX_PIN);

void setup() {
  Serial.begin(115200);
  Serial.println("CAN Receiver Initialized");
  
  // Start CAN communication at 125 kbps
  can.begin(125000);
  Serial.println("Waiting for CAN frames...");
}

void loop() {
  // 1. Create a CAN frame to store incoming data
  CAN_Frame rxFrame;

  // 2. Try to receive a frame
  if (can.receiveFrame(rxFrame)) {
    Serial.println("--- Frame Received! ---");
    Serial.printf("ID: 0x%X\n", rxFrame.id);
    Serial.printf("DLC: %d\n", rxFrame.dlc);
    Serial.print("Data: ");
    for (int i = 0; i < rxFrame.dlc; i++) {
      Serial.printf("0x%02X ", rxFrame.data[i]);
    }
    Serial.println("\n-----------------------\n");
  }
}
