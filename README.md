# ESP32- A Software CAN Library for the ESP32 Family

A feature-complete, lightweight, and educational CAN communication library for the **entire ESP32 family**. This library does **not** use the built-in hardware CAN (TWAI) controller. Instead, it manually implements the CAN protocol's logic on standard GPIO pins, a technique known as "bit-banging."

This approach makes it compatible with virtually any ESP32 board, as it only requires two available GPIO pins. This version is a robust implementation that includes **arbitration, a full error state machine, CRC validation, and active acknowledgement**.

> **⚠️ Disclaimer: For Experimental and Low-Speed Use**
> This library was created to provide a fully-featured CAN implementation for educational and experimental purposes. While it implements the core logic of the CAN standard, software-based timing (bit-banging) is inherently less precise than a dedicated hardware controller. It is **not recommended for high-speed or mission-critical applications**. For reliable, high-speed communication, please use a library that leverages the ESP32's built-in hardware CAN (TWAI) controller.

---

## Features
-   **Universal ESP32 Compatibility:** Works with any board in the ESP32 family.
-   **Flexible Pin Assignment:** Use any two available GPIO pins for RX and TX.
-   **Collision Detection & Arbitration:** Sender detects higher-priority messages and will safely abort transmission if it loses arbitration.
-   **Full Error State Machine:** Tracks Transmit/Receive Error Counters (TEC/REC) and transitions between **Error-Active**, **Error-Passive**, and **Bus-Off** states.
-   **CRC Validation & Active ACK:** The receiver validates the CRC of incoming messages and actively sends an Acknowledge (ACK) bit for valid frames.
-   **Non-Blocking Read:** The `readFrame()` function is non-blocking, allowing your main loop to run freely without getting stuck.
-   **Hardware Independent:** Does not rely on the built-in TWAI peripheral.

---

## Hardware Requirements
To use this library, you need **two** of the following setups, one for sending and one for receiving:

1.  **Any ESP32 Board:** The library is compatible with the entire ESP32 family.
2.  **An External CAN Transceiver (Mandatory):** You **must** use a dedicated CAN transceiver module (e.g., **MCP2551**, **TJA1050**).

### Wiring Diagram
```
              +--------------+        +----------------+
Any ESP32     |              |        |                |
GPIO Pin (TX) |--------------|-- TXD --| CAN Transceiver|-- CAN_H ---< CAN Bus >
              |              |        |                |
GPIO Pin (RX) |--------------|-- RXD --|                |-- CAN_L ---< CAN Bus >
              +--------------+        +----------------+
```

---

## Installation

1.  Click the "Code" button on this repository and select "Download ZIP".
2.  Open your Arduino IDE.
3.  Go to `Sketch` -> `Include Library` -> `Add .ZIP Library...`.
4.  Select the ZIP file you just downloaded.
5.  The library is now installed and ready to use.

---

## API and Usage

### 1. Include the Library
```cpp
#include <ESP_CAN.h>
```

### 2. Constructor & Public State
You can monitor the node's health via public variables.
```cpp
ESP_CAN can(int rxPin, int txPin);

// Monitor these in your code:
uint8_t tec = can.tec; // Transmit Error Counter
uint8_t rec = can.rec; // Receive Error Counter
CAN_State state = can.state; // ERROR_ACTIVE, ERROR_PASSIVE, or BUS_OFF
```

### 3. begin()
```cpp
void begin(long baudrate);
```

### 4. Sending a Frame
```cpp
bool sendFrame(CAN_Frame &frame);
```
Returns `true` if the frame was successfully transmitted and acknowledged. Returns `false` if arbitration was lost, no acknowledgement was received, or the node is in a Bus-Off state.

### 5. Reading a Frame (Non-Blocking)
```cpp
CAN_Read_Status readFrame(CAN_Frame &frame);
```
This function must be called frequently in your `loop()`. It returns:
- `CAN_READ_NO_MSG`: No new message has been received.
- `CAN_READ_MSG_OK`: A valid message was received and its contents are in the `frame` struct.
- `CAN_READ_ERROR`: A frame was detected but contained an error (e.g., bad CRC).

---

## Full Examples (Non-Blocking)

### Sender Sketch (`Sender.ino`)
```cpp
#include <ESP_CAN.h>

const int CAN_RX_PIN = 5;
const int CAN_TX_PIN = 4;
ESP_CAN can(CAN_RX_PIN, CAN_TX_PIN);

void setup() {
  Serial.begin(115200);
  Serial.println("CAN Sender Initialized");
  can.begin(125000);
  delay(1000);
}

void loop() {
  CAN_Frame txFrame;
  txFrame.id = 0x123;
  txFrame.dlc = 4;
  txFrame.data[0] = 0xDE;
  txFrame.data[1] = 0xAD;
  txFrame.data[2] = 0xBE;
  txFrame.data[3] = 0xEF;

  Serial.println("Attempting to send CAN frame...");
  if (can.sendFrame(txFrame)) {
    Serial.println("Frame sent and ACKNOWLEDGED!");
  } else {
    Serial.printf("Failed to send. Node state: %d, TEC: %d\n", can.state, can.tec);
  }

  delay(2000);
}
```

### Receiver Sketch (`Receiver.ino`)
```cpp
#include <ESP_CAN.h>

const int CAN_RX_PIN = 5;
const int CAN_TX_PIN = 4;
ESP_CAN can(CAN_RX_PIN, CAN_TX_PIN);

void setup() {
  Serial.begin(115200);
  Serial.println("CAN Receiver Initialized");
  can.begin(125000);
  Serial.println("Waiting for CAN frames...");
}

void loop() {
  CAN_Frame rxFrame;
  CAN_Read_Status status = can.readFrame(rxFrame);

  if (status == CAN_READ_MSG_OK) {
    Serial.println("--- Frame Received! ---");
    Serial.printf("ID: 0x%X\n", rxFrame.id);
    Serial.printf("DLC: %d\n", rxFrame.dlc);
    Serial.print("Data: ");
    for (int i = 0; i < rxFrame.dlc; i++) {
      Serial.printf("0x%02X ", rxFrame.data[i]);
    }
    Serial.println("\n-----------------------\n");
  } else if (status == CAN_READ_ERROR) {
    Serial.printf("Frame error detected! REC: %d\n", can.rec);
  }
}
```

---

## Advanced Limitations
While feature-complete in software, this library's reliance on bit-banging has inherent limitations compared to a hardware controller:
-   **Timing Precision:** The timing is dependent on `micros()` and `delayMicroseconds()`, which can be affected by other code, interrupts, or high CPU load. This can lead to instability, especially at higher baud rates (>125kbps).
-   **CPU Intensive:** The non-blocking `readFrame()` function must be polled constantly, consuming CPU cycles that could be used for other tasks.
-   **Limited Arbitration Reliability:** While arbitration logic is implemented, its reliability depends heavily on the timing precision. In a high-traffic scenario, it may not perform as robustly as a hardware-based solution.

---


## License
This project has no licensed, because it is a private development with no assurances.
