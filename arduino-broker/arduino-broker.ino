#include <Arduino.h>
#include <SoftwareSerial.h>
#include "ssd1306.h"

const char DATA_PINS[] = {12, 11, 10, 9, 8, 7, 6, 5};
#define PACKET_FULL_SIZE              130
#define PACKET_DATA_SIZE              128
#define PACKET_DATA_START_INDEX       0
#define PACKET_CHECKSUM_LO_INDEX      128
#define PACKET_CHECKSUM_HI_INDEX      129
#define PACKET_STATUS_SENT            0
#define PACKET_STATUS_RECEIVING       1
#define PACKET_STATUS_SENDING         2
#define PACKET_STATUS_CSUM_ERR        3
#define PACKET_STATUS_AWAITING_RESULT 4
#define FLAG2                   4
#define PC2                     2
#define PA2                     3
#define SERIAL_RX               A2
#define SERIAL_TX               A3
#define OLED_SDA                A4
#define OLED_SCL                A5
#define INACTIVE_TIMEOUT_MS     10000

void setup();
void loop();
void readSerial();
void nextPacket();
void sendByte();
void onPC2();
void setStatus(uint8_t);
void resetCounters();

// packet structure:
// [0] - [127] packet data
// [128] - checksum lo byte
// [129] - checksum hi byte
byte fullPacket[PACKET_FULL_SIZE];
uint8_t bytesReceived = 0;
uint8_t sendIndex = PACKET_DATA_START_INDEX;
uint8_t bytesSent = 0;
uint8_t bytesAcked = 0;
uint16_t totalReceivedSuccess = 0;
uint16_t totalReceivedError = 0;
uint16_t totalSentSuccess = 0;
uint16_t totalSentError = 0;


uint8_t packetStatus = 0;
volatile bool byteAcked = false;
unsigned long lastActivityMs = 0;

SoftwareSerial swSerial(SERIAL_RX, SERIAL_TX);

void setup() {
  ssd1306_128x64_i2c_init();
  ssd1306_fillScreen(0x00);
  ssd1306_setFixedFont(ssd1306xled_font6x8);
  setStatus(packetStatus);
  updateCounterDisplay();

  Serial.begin(19200);

  for (int i = 0; i < 8; ++i) {
      pinMode(DATA_PINS[i], OUTPUT);
  }

  digitalWrite(FLAG2, HIGH);
  pinMode(FLAG2, OUTPUT);

  pinMode(PA2, INPUT);
  pinMode(PC2, INPUT);
  attachInterrupt(digitalPinToInterrupt(PC2), onPC2, FALLING);

  pinMode(SERIAL_RX, INPUT);
  pinMode(SERIAL_TX, OUTPUT);

  swSerial.begin(19200);

  nextPacket();
}

// To validate the checksum
//   Add the checksum we receive, which is actually a 1's complement
//   of the checksum, to the sum of bytes received. Then make sure
//   all bits in our summed value are 1's.
bool validateChecksum() {
  uint8_t checksumLo = fullPacket[PACKET_CHECKSUM_LO_INDEX];
  uint8_t checksumHi = fullPacket[PACKET_CHECKSUM_HI_INDEX];

  unsigned int checksumReceived = (checksumHi << 8) + checksumLo;

  unsigned int byteSum = 0;
  for (unsigned int i = 0; i < PACKET_CHECKSUM_LO_INDEX; ++i) {
    byteSum += fullPacket[i];
  }

  // Serial.print("byte sum: ");
  // Serial.print(byteSum);
  // Serial.print(", csum lo: ");
  // Serial.print(checksumLo);
  // Serial.print(", csum hi: ");
  // Serial.println(checksumHi);

  unsigned int summed = byteSum + checksumReceived;

  if(summed != 65535) {
    return false;
  }

  return true;
}

void readSerial() {
  if (swSerial.available() == 0) return;

  lastActivityMs = millis();
  // if (bytesReceived == 0) {
  //   Serial.print(F("Packet: ["));
  // }
  // else {
  //   Serial.print(F(","));
  // }

  byte byteRead = swSerial.read();
  fullPacket[bytesReceived] = byteRead;
  ++bytesReceived;
  //Serial.print(byteRead, HEX);

  if (bytesReceived == PACKET_FULL_SIZE) {
    //Serial.println(F("]"));
    if(validateChecksum()) {
      Serial.println("packet received, checksum passed");
      totalReceivedSuccess++;
      setStatus(PACKET_STATUS_SENDING);
    }
    else {
      Serial.println("packet received, checksum error");
      totalReceivedError++;
      setStatus(PACKET_STATUS_CSUM_ERR);
    }
    updateCounterDisplay();
  }
}

void loop() {
  if((millis() - lastActivityMs) > INACTIVE_TIMEOUT_MS) {
    Serial.println(F("Timeout. Starting over."));
    resetCounters();
    nextPacket();
  }

  switch (packetStatus) {
    case PACKET_STATUS_SENT:
      Serial.println(F("Packet sent and acked by receiver."));
      nextPacket();
      swSerial.write('A');
      break;
    case PACKET_STATUS_RECEIVING:
      // when the receiver starts up, it might trigger the ack.
      // so ignore it if so.
      if (byteAcked) byteAcked = false;
      readSerial();
      break;
    case PACKET_STATUS_SENDING:
      if (byteAcked) {
        byteAcked = false;
        lastActivityMs = millis();
        ++bytesAcked;
      }

      if(bytesAcked == PACKET_FULL_SIZE) {
        setStatus(PACKET_STATUS_AWAITING_RESULT);
        break;
      }
      if (bytesAcked >= bytesSent) {
        sendByte();
        break;
      }
      break;
    case PACKET_STATUS_CSUM_ERR:
      nextPacket();
      swSerial.write('R');
      break;
    case PACKET_STATUS_AWAITING_RESULT:
      if (byteAcked) {
        byteAcked = false;
        lastActivityMs = millis();
        setStatus(digitalRead(PA2) ? PACKET_STATUS_CSUM_ERR : PACKET_STATUS_SENT);
        if(packetStatus == PACKET_STATUS_CSUM_ERR) {
          Serial.println("checksum error on c64");
          totalSentError++;
        }
        else {
          totalSentSuccess++;
        }
        updateCounterDisplay();
      }
      break;
    default:
      break;
  }

}

void nextPacket() {
  lastActivityMs = millis();
  sendIndex = PACKET_DATA_START_INDEX;
  bytesReceived = 0;
  bytesSent = 0;
  bytesAcked = 0;
  setStatus(PACKET_STATUS_RECEIVING);
}

void sendByte() {
  digitalWrite(FLAG2, HIGH);

  byte data = fullPacket[sendIndex];
  for (int i = 7; i >= 0; --i) {
    digitalWrite(DATA_PINS[i], data & 1);
    data = data >> 1;
  }

  digitalWrite(FLAG2, LOW);

  ++sendIndex;
  ++bytesSent;
}

void onPC2() {
  byteAcked = true;
}

void setStatus(uint8_t status) {
  packetStatus = status;
  char statusStr[10];
  sprintf(statusStr, "Status: %d", status);
  ssd1306_printFixed(0, 8, statusStr, STYLE_NORMAL);
  //oled.println(packetStatus);
  //oled.setCursor(1, 1);
  //oled.write((char)packetStatus);
}

void updateCounterDisplay() {
  char rxSuccess[20];
  sprintf(rxSuccess, "RX S:   %5d", totalReceivedSuccess);
  ssd1306_printFixed(0, 16, rxSuccess, STYLE_NORMAL);

  char rxError[20];
  sprintf(rxError, "RX ERR: %5d", totalReceivedError);
  ssd1306_printFixed(0, 24, rxError, STYLE_NORMAL);

  char txSuccess[20];
  sprintf(txSuccess, "TX S:   %5d", totalSentSuccess);
  ssd1306_printFixed(0, 32, txSuccess, STYLE_NORMAL);

  char txError[20];
  sprintf(txError, "TX ERR: %5d", totalSentError);
  ssd1306_printFixed(0, 40, txError, STYLE_NORMAL);
}

void resetCounters() {
  totalReceivedSuccess = 0;
  totalReceivedError = 0;
  totalSentSuccess = 0;
  totalSentError = 0;
  updateCounterDisplay();
}