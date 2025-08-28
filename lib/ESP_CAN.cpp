/*
 * ESP_CAN.cpp - Implementation of the feature-complete bit-banging CAN library.
 */

#include "ESP_CAN.h"

ESP_CAN::ESP_CAN(int rxPin, int txPin) {
  _rxPin = rxPin;
  _txPin = txPin;
  tec = 0;
  rec = 0;
  state = CAN_STATE_ERROR_ACTIVE;
  _rxState = RX_STATE_IDLE;
}

void ESP_CAN::begin(long baudrate) {
  pinMode(_txPin, OUTPUT);
  pinMode(_rxPin, INPUT_PULLUP);
  digitalWrite(_txPin, HIGH);
  _bitTimeUs = 1000000 / baudrate;
  _lastSampleTime = micros();
}

// --- ERROR HANDLING ---

void ESP_CAN::updateState() {
  if (tec > 255 || rec > 255) {
    state = CAN_STATE_BUS_OFF;
  } else if (tec > 127 || rec > 127) {
    state = CAN_STATE_ERROR_PASSIVE;
  } else {
    state = CAN_STATE_ERROR_ACTIVE;
  }
}

void ESP_CAN::handleError(bool isTxError, bool isRxError) {
  if (isTxError && state != CAN_STATE_BUS_OFF) {
    tec += 8;
  }
  if (isRxError && state != CAN_STATE_BUS_OFF) {
    rec++;
  }
  updateState();
}

void ESP_CAN::handleSuccess(bool isTxSuccess, bool isRxSuccess) {
  if (isTxSuccess && tec > 0) {
    tec--;
  }
  if (isRxSuccess && rec > 0) {
    rec--;
  }
  updateState();
}

// --- SENDER LOGIC ---

bool ESP_CAN::sendBit(bool bit, bool checkArbitration) {
  if (state == CAN_STATE_BUS_OFF) return false;

  digitalWrite(_txPin, bit ? HIGH : LOW);
  delayMicroseconds(_bitTimeUs);

  // Arbitration check: if we send recessive (1) but read dominant (0), we lost.
  if (checkArbitration && bit == HIGH && digitalRead(_rxPin) == LOW) {
    return false; // Arbitration lost
  }
  return true; // Bit sent successfully
}

bool ESP_CAN::sendFrame(CAN_Frame &frame) {
  if (state == CAN_STATE_BUS_OFF) return false;

  bool bitSequence[128];
  int bitIndex = 0;

  // Construct bit sequence for CRC
  for (int i = 10; i >= 0; i--) bitSequence[bitIndex++] = (frame.id >> i) & 0x01;
  bitSequence[bitIndex++] = 0; // RTR
  bitSequence[bitIndex++] = 0; // IDE
  bitSequence[bitIndex++] = 0; // r0
  uint8_t dlc = frame.dlc > 8 ? 8 : frame.dlc;
  for (int i = 3; i >= 0; i--) bitSequence[bitIndex++] = (dlc >> i) & 0x01;
  for (int i = 0; i < dlc; i++) {
    for (int j = 7; j >= 0; j--) bitSequence[bitIndex++] = (frame.data[i] >> j) & 0x01;
  }
  uint16_t crc = calculateCRC(bitSequence, bitIndex);

  // --- Transmit Frame ---
  if (!sendBit(LOW, false)) { handleError(true, false); return false; } // SOF

  int consecutiveBits = 0;
  bool lastBit = LOW;

  // Send main frame with bit stuffing and arbitration
  for (int i = 0; i < bitIndex; i++) {
    if (bitSequence[i] == lastBit) consecutiveBits++; else consecutiveBits = 1;
    lastBit = bitSequence[i];
    if (!sendBit(bitSequence[i], true)) { handleError(true, false); return false; } // ARBITRATION LOST
    if (consecutiveBits == 5) {
      if (!sendBit(!lastBit, true)) { handleError(true, false); return false; }
      consecutiveBits = 0;
      lastBit = !lastBit;
    }
  }

  // Send CRC with bit stuffing
  for (int i = 14; i >= 0; i--) {
    bool bit = (crc >> i) & 0x01;
    if (bit == lastBit) consecutiveBits++; else consecutiveBits = 1;
    lastBit = bit;
    if (!sendBit(bit, false)) { handleError(true, false); return false; }
    if (consecutiveBits == 5) {
      if (!sendBit(!lastBit, false)) { handleError(true, false); return false; }
      consecutiveBits = 0;
      lastBit = !lastBit;
    }
  }

  // CRC Delimiter
  if (!sendBit(HIGH, false)) { handleError(true, false); return false; }

  // ACK Slot
  pinMode(_txPin, INPUT);
  delayMicroseconds(_bitTimeUs);
  bool ackReceived = (digitalRead(_rxPin) == LOW);
  pinMode(_txPin, OUTPUT);
  digitalWrite(_txPin, HIGH);

  if (!ackReceived) { handleError(true, false); return false; }

  // ACK Delimiter & EOF
  sendBit(HIGH, false);
  for (int i = 0; i < 7; i++) sendBit(HIGH, false);

  handleSuccess(true, false);
  return true;
}

// --- RECEIVER LOGIC (Non-Blocking) ---

CAN_Read_Status ESP_CAN::readFrame(CAN_Frame &frame) {
  if (state == CAN_STATE_BUS_OFF) return CAN_READ_NO_MSG;

  // Non-blocking bit sampling
  if (micros() - _lastSampleTime < _bitTimeUs) {
    return CAN_READ_NO_MSG;
  }
  _lastSampleTime += _bitTimeUs;
  bool currentBit = digitalRead(_rxPin);

  // State Machine for receiving a frame
  switch (_rxState) {
    case RX_STATE_IDLE:
      if (currentBit == LOW) { // Potential SOF
        _rxState = RX_STATE_SOF;
        _rxBitCount = 0;
        _consecutiveBits = 1;
        _lastBit = LOW;
      }
      break;

    case RX_STATE_SOF:
      // We are now in the frame, after SOF
      _rxState = RX_STATE_FRAME;
      // Fall through to process the first bit of the ID

    case RX_STATE_FRAME:
      if (currentBit == _lastBit) _consecutiveBits++; else _consecutiveBits = 1;

      if (_consecutiveBits == 5) {
        // Next bit is a stuff bit, ignore it by not storing it
        _consecutiveBits = 0; // Reset counter for the stuff bit itself
        _lastBit = !currentBit; // Assume stuff bit is opposite
        return CAN_READ_NO_MSG;
      }

      _lastBit = currentBit;
      _rxBuffer[_rxBitCount++] = currentBit;

      // Check for EOF (7 consecutive recessive bits)
      if (currentBit == HIGH && _consecutiveBits >= 7) {
        _rxState = RX_STATE_IDLE; // Frame finished

        // --- DECODE FRAME ---
        int bitIndex = 0;
        frame.id = 0;
        for (int i = 0; i < 11; i++) frame.id = (frame.id << 1) | _rxBuffer[bitIndex++];
        bitIndex += 3; // Skip RTR, IDE, r0
        frame.dlc = 0;
        for (int i = 0; i < 4; i++) frame.dlc = (frame.dlc << 1) | _rxBuffer[bitIndex++];
        if (frame.dlc > 8) frame.dlc = 8;
        for (int i = 0; i < frame.dlc; i++) {
          frame.data[i] = 0;
          for (int j = 0; j < 8; j++) frame.data[i] = (frame.data[i] << 1) | _rxBuffer[bitIndex++];
        }
        uint16_t received_crc = 0;
        for (int i = 0; i < 15; i++) received_crc = (received_crc << 1) | _rxBuffer[bitIndex++];

        // --- VALIDATE CRC and SEND ACK ---
        uint16_t calculated_crc = calculateCRC(_rxBuffer, bitIndex - 15);
        if (calculated_crc == received_crc) {
          // Send ACK
          delayMicroseconds(_bitTimeUs); // Wait for ACK slot
          digitalWrite(_txPin, LOW);
          delayMicroseconds(_bitTimeUs);
          digitalWrite(_txPin, HIGH);
          handleSuccess(false, true);
          return CAN_READ_MSG_OK;
        } else {
          handleError(false, true);
          return CAN_READ_ERROR;
        }
      }
      break;
  }
  return CAN_READ_NO_MSG;
}

// --- UTILITY ---
uint16_t ESP_CAN::calculateCRC(const bool bits[], int len) {
    uint16_t crc_reg = 0x0000;
    for (int i = 0; i < len; i++) {
        bool do_xor = ( (crc_reg & 0x4000) >> 14 ) ^ bits[i];
        crc_reg <<= 1;
        if (do_xor) {
            crc_reg ^= 0x4599;
        }
    }
    return crc_reg & 0x7FFF;
}
