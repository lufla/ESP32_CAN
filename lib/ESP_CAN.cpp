/*
 * ESP_CAN.cpp - Implementation of the simple bit-banging CAN library.
 *
 * Author: Gemini
 * Date: 2024
 */

#include "ESP_CAN.h"

// Constructor: Initializes the CAN object with specified RX and TX pins.
ESP_CAN::ESP_CAN(int rxPin, int txPin) {
  _rxPin = rxPin;
  _txPin = txPin;
}

// begin: Sets up the pins and calculates the bit timing based on baudrate.
void ESP_CAN::begin(long baudrate) {
  pinMode(_txPin, OUTPUT);
  pinMode(_rxPin, INPUT_PULLUP);
  digitalWrite(_txPin, HIGH); // Set bus to idle (recessive)

  // Calculate bit time in microseconds from baudrate
  _bitTimeUs = 1000000 / baudrate;
}

// --- SENDER LOGIC ---

void ESP_CAN::sendRecessive() {
  digitalWrite(_txPin, HIGH);
  delayMicroseconds(_bitTimeUs);
}

void ESP_CAN::sendDominant() {
  digitalWrite(_txPin, LOW);
  delayMicroseconds(_bitTimeUs);
}

void ESP_CAN::sendBitStream(bool bits[], int len) {
  int consecutiveBits = 0;
  bool lastBit = true; // Start with recessive state

  for (int i = 0; i < len; i++) {
    bool currentBit = bits[i];
    if (currentBit == lastBit) {
      consecutiveBits++;
    } else {
      consecutiveBits = 1;
      lastBit = currentBit;
    }

    if (currentBit) sendRecessive();
    else sendDominant();

    if (consecutiveBits == 5) {
      if (lastBit) sendDominant();
      else sendRecessive();
      consecutiveBits = 0;
      lastBit = !lastBit;
    }
  }
}

bool ESP_CAN::sendFrame(CAN_Frame &frame) {
  bool frameBits[128];
  int bitIndex = 0;

  // 1. Identifier (11 bits)
  for (int i = 10; i >= 0; i--) {
    frameBits[bitIndex++] = (frame.id >> i) & 0x01;
  }

  // 2. RTR bit (0 for data frame)
  frameBits[bitIndex++] = 0;

  // 3. IDE bit (0 for standard frame) & reserved bit
  frameBits[bitIndex++] = 0;
  frameBits[bitIndex++] = 0;

  // 4. DLC (4 bits)
  uint8_t dlc = frame.dlc > 8 ? 8 : frame.dlc;
  for (int i = 3; i >= 0; i--) {
    frameBits[bitIndex++] = (dlc >> i) & 0x01;
  }

  // 5. Data Field
  for (int i = 0; i < dlc; i++) {
    for (int j = 7; j >= 0; j--) {
      frameBits[bitIndex++] = (frame.data[i] >> j) & 0x01;
    }
  }

  // 6. CRC (Dummy value for this example)
  uint16_t crc = 0x5D3F;
  for (int i = 14; i >= 0; i--) {
    frameBits[bitIndex++] = (crc >> i) & 0x01;
  }

  // --- Transmit the Frame ---
  sendDominant(); // SOF
  sendBitStream(frameBits, bitIndex);
  sendRecessive(); // CRC Delimiter

  // ACK Slot
  pinMode(_txPin, INPUT);
  delayMicroseconds(_bitTimeUs);
  pinMode(_txPin, OUTPUT);

  sendRecessive(); // ACK Delimiter

  // EOF
  for (int i = 0; i < 7; i++) {
    sendRecessive();
  }
  return true;
}


// --- RECEIVER LOGIC ---

bool ESP_CAN::readBit() {
  delayMicroseconds(_bitTimeUs / 2);
  bool bit = digitalRead(_rxPin);
  delayMicroseconds(_bitTimeUs - (_bitTimeUs / 2));
  return bit;
}

bool ESP_CAN::receiveFrame(CAN_Frame &frame) {
  // Wait for SOF
  while (digitalRead(_rxPin) == HIGH) {
      // Return false if there's no activity to avoid blocking forever
      // A more robust solution would use interrupts or a timeout.
  }

  // Sync to the middle of the SOF bit
  delayMicroseconds(_bitTimeUs / 2);

  bool rawBits[128];
  int bitCount = 0;
  int consecutiveBits = 0;
  bool lastBit = 0; // SOF was dominant

  // Read the frame, accounting for bit stuffing
  while (bitCount < 120) {
    delayMicroseconds(_bitTimeUs);
    bool currentBit = digitalRead(_rxPin);

    if (currentBit == lastBit) {
      consecutiveBits++;
    } else {
      consecutiveBits = 1;
      lastBit = currentBit;
    }

    if (consecutiveBits == 5) {
      delayMicroseconds(_bitTimeUs); // Skip the stuffed bit
      lastBit = digitalRead(_rxPin);
      consecutiveBits = 1;
      continue; // Don't store the stuffed bit
    }
    
    // A sequence of 7 recessive bits indicates EOF
    if (currentBit == 1 && consecutiveBits >= 7) {
        break; // End of Frame detected
    }
    
    rawBits[bitCount++] = currentBit;
  }

  if (bitCount < 20) return false; // Frame too short, likely an error

  // --- Decode the Frame ---
  int bitIndex = 0;
  
  // 1. Identifier
  frame.id = 0;
  for (int i = 0; i < 11; i++) {
    frame.id = (frame.id << 1) | rawBits[bitIndex++];
  }

  // 2. RTR, IDE, reserved
  bitIndex += 3;

  // 3. DLC
  frame.dlc = 0;
  for (int i = 0; i < 4; i++) {
    frame.dlc = (frame.dlc << 1) | rawBits[bitIndex++];
  }
  if (frame.dlc > 8) frame.dlc = 8;

  // 4. Data
  for (int i = 0; i < frame.dlc; i++) {
    frame.data[i] = 0;
    for (int j = 0; j < 8; j++) {
      frame.data[i] = (frame.data[i] << 1) | rawBits[bitIndex++];
    }
  }
  
  // A real library would check CRC here and send an ACK bit.

  return true;
}

