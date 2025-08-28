/*
 * ESP_CAN.h - A simple bit-banging CAN library for ESP32.
 *
 * This library is for educational purposes to demonstrate the CAN protocol
 * at a low level. It is not a robust, production-ready implementation.
 *
 * Author: Gemini
 * Date: 2024
 */

#ifndef ESP_CAN_H
#define ESP_CAN_H

#include <Arduino.h>

// A simple struct to hold CAN frame data
struct CAN_Frame {
  uint32_t id;      // 11-bit CAN Identifier
  uint8_t dlc;      // Data Length Code (0-8)
  uint8_t data[8];  // Data payload
};

class ESP_CAN {
public:
  // Constructor
  ESP_CAN(int rxPin, int txPin);

  // Initialization
  void begin(long baudrate);

  // Sending and Receiving
  bool sendFrame(CAN_Frame &frame);
  bool receiveFrame(CAN_Frame &frame);

private:
  int _rxPin;
  int _txPin;
  int _bitTimeUs;

  // Low-level bit functions
  void sendRecessive();
  void sendDominant();
  void sendBitStream(bool bits[], int len);
  bool readBit();
};

#endif // ESP_CAN_H

