/*
* ESP_CAN.h - An enhanced bit-banging CAN library for the ESP32 family.
 *
 * This library is for educational and experimental purposes. It implements
 * CRC calculation and ACK checking but lacks full bus arbitration and
 * error state handling.
 *
 * Author: Lukas Flad
 * Date: 2025
 */

#ifndef ESP_CAN_H
#define ESP_CAN_H

#include <Arduino.h>

// Represents the operational state of the CAN node
enum CAN_State {
  CAN_STATE_ERROR_ACTIVE,
  CAN_STATE_ERROR_PASSIVE,
  CAN_STATE_BUS_OFF
};

// Represents the result of a read operation
enum CAN_Read_Status {
  CAN_READ_NO_MSG,
  CAN_READ_MSG_OK,
  CAN_READ_ERROR
};

// Struct to hold CAN frame data
struct CAN_Frame {
  uint32_t id;      // 11-bit CAN Identifier
  uint8_t dlc;      // Data Length Code (0-8)
  uint8_t data[8];  // Data payload
};

class ESP_CAN {
public:
  // Publicly accessible error counters and state
  uint8_t tec;      // Transmit Error Counter
  uint8_t rec;      // Receive Error Counter
  CAN_State state;

  // Constructor
  ESP_CAN(int rxPin, int txPin);

  // Initialization
  void begin(long baudrate);

  // Sending and Receiving (now non-blocking)
  bool sendFrame(CAN_Frame &frame);
  CAN_Read_Status readFrame(CAN_Frame &frame);

private:
  int _rxPin;
  int _txPin;
  int _bitTimeUs;
  unsigned long _lastSampleTime;

  // Non-blocking read state machine variables
  enum RxState { RX_STATE_IDLE, RX_STATE_SOF, RX_STATE_FRAME };
  RxState _rxState;
  bool _rxBuffer[128];
  int _rxBitCount;
  int _consecutiveBits;
  bool _lastBit;

  // Low-level bit functions
  bool sendBit(bool bit, bool checkArbitration);
  bool readBit();

  // CRC-15-CAN Calculation
  uint16_t calculateCRC(const bool bits[], int len);

  // Error handling
  void handleError(bool isTxError, bool isRxError);
  void handleSuccess(bool isTxSuccess, bool isRxSuccess);
  void updateState();
};

#endif // ESP_CAN_H
