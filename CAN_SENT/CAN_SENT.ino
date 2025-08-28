#include "../lib/ESP_CAN.h"
#include "../lib/ESP_CAN.cpp"

// Define the pins for CAN communication
const int CAN_RX_PIN = 5;  // Not used for sending, but required by library
const int CAN_TX_PIN = 4;

// Initialize our CAN library
ESP_CAN can(CAN_RX_PIN, CAN_TX_PIN);

void setup() {
  Serial.begin(115200);
  Serial.println("CAN Sender Initialized");
  
  // Start CAN communication at 125 kbps
  can.begin(125000);
  delay(1000);
}

void loop() {
  // 1. Create a CAN frame
  CAN_Frame txFrame;
  
  // 2. Set the frame properties
  txFrame.id = 0x123;
  txFrame.dlc = 4;
  txFrame.data[0] = 0xDE;
  txFrame.data[1] = 0xAD;
  txFrame.data[2] = 0xBE;
  txFrame.data[3] = 0xEF;

  // 3. Send the frame
  Serial.println("Sending CAN frame...");
  if (can.sendFrame(txFrame)) {
    Serial.println("Frame sent successfully!");
  } else {
    Serial.println("Failed to send frame.");
  }

  delay(2000); // Wait 2 seconds before sending the next frame
}
