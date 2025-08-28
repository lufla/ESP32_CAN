# ESP32-CAN - A Software CAN Library for the ESP32 Family

A simple, lightweight, and educational CAN communication library for the **entire ESP32 family**. This library does **not** use the built-in hardware CAN (TWAI) controller. Instead, it manually implements the CAN protocol's logic on standard GPIO pins, a technique known as "bit-banging."

This approach makes it compatible with virtually any ESP32 board, as it only requires two available GPIO pins.

> **⚠️ Disclaimer: For Educational Use Only**
> This library was created to demonstrate the low-level workings of the CAN 2.0A protocol. It is **not** suitable for production, commercial, or mission-critical applications. It lacks crucial features like bus arbitration, robust error handling, and proper message validation, which are essential for reliable CAN communication. For any real-world application, please use a library that leverages the ESP32's built-in hardware CAN (TWAI) controller.

---

## Features
-   **Universal ESP32 Compatibility:** Works with any board in the ESP32 family (ESP32, ESP32-S2, ESP32-C3, etc.).
-   **Flexible Pin Assignment:** Use any two available GPIO pins for RX and TX.
-   **Hardware Independent:** Does not rely on the built-in TWAI peripheral.
-   **Lightweight:** Minimal footprint with no complex dependencies.
-   **Educational:** A great tool for learning the fundamentals of the CAN protocol.

---

## Hardware Requirements
To use this library, you need **two** of the following setups, one for sending and one for receiving:

1.  **Any ESP32 Board:** The library is compatible with the entire ESP32 family.
2.  **An External CAN Transceiver (Mandatory):** You **must** use a dedicated CAN transceiver module (e.g., **MCP2551**, **TJA1050**). You cannot connect the ESP32's GPIO pins directly to a CAN bus. The transceiver is responsible for converting the logic-level signals to the differential signals (CAN_H and CAN_L) required by the CAN standard.

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

### 2. Constructor
Create a CAN object by specifying your chosen RX and TX pins.
```cpp
ESP_CAN can(int rxPin, int txPin);

// Example:
ESP_CAN can(GPIO_NUM_5, GPIO_NUM_4); // RX on GPIO 5, TX on GPIO 4
```

### 3. begin()
Initialize the library and set the CAN bus speed. This should be called in your `setup()` function.
```cpp
void begin(long baudrate);
```
**Supported Baud Rates:**
The CAN protocol requires all nodes on the bus to operate at the same speed. Common baud rates include:
-   `1000000` (1 Mbps)
-   `500000`  (500 kbps)
-   `250000`  (250 kbps)
-   `125000`  (125 kbps)
-   `100000`  (100 kbps)
-   `50000`   (50 kbps)

**Example:**
```cpp
void setup() {
  // Start CAN communication at 125 kbps
  can.begin(125000);
}
```

### 4. The `CAN_Frame` Struct
This library uses a simple struct to hold message data for both sending and receiving.
```cpp
struct CAN_Frame {
  uint32_t id;      // 11-bit CAN Identifier (e.g., 0x123)
  uint8_t dlc;      // Data Length Code (0-8 bytes)
  uint8_t data[8];  // Data payload
};
```

### 5. Sending a Frame
```cpp
bool sendFrame(CAN_Frame &frame);
```
Returns `true` on successful transmission.

### 6. Receiving a Frame
```cpp
bool receiveFrame(CAN_Frame &frame);
```
This is a blocking function that waits for a new frame to arrive. It returns `true` when a valid frame has been received and its data has been placed in the provided `frame` struct.

---

## Full Examples

### Sender Sketch (`Sender.ino`)
```cpp
#include <ESP_CAN.h>

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
```

### Receiver Sketch (`Receiver.ino`)
```cpp
#include <ESP_CAN.h>

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
```

---

## Limitations
This library is a proof-of-concept and has several major limitations:
-   **No Collision Detection or Arbitration:** If two nodes transmit at the same time, the messages will be corrupted. The library does not monitor the bus while transmitting to detect collisions.
-   **No Error Frames:** It does not generate or handle CAN error frames.
-   **Dummy CRC:** The CRC is a fixed, dummy value. The receiver does not validate it.
-   **Basic ACK:** The sender simply releases the line during the ACK slot but does not check if a receiver actually acknowledged the message.
-   **Blocking Receiver:** The `receiveFrame()` function will halt your program until a message arrives.

---

## License
This project has no licensed, because it is a private development with no assurances.
