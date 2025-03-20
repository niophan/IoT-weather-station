#include <SPI.h>
#include <Ethernet.h>

// #define MAC_6    0x73
// static uint8_t mymac[6] = { 0x44, 0x76, 0x58, 0x10, 0x00, MAC_6 };

// HARDWARE MAC
// #define MAC_6    0x30
// static uint8_t mymac[6] = { 0xA8, 0x61, 0x0A, 0xAE, 0x46, MAC_6 };

void fetchIP();

void setup() {
  Serial.begin(9600);
  byte mymac[6];

  Ethernet.macAddress(mymac)
  Serial.print("MAC Address: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(mac[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();

  fetchIP();
}

void loop() {}

void fetchIP() {
  byte connection = 1;
  connection = Ethernet.begin(mac);
  
  Serial.print(F("\nW5100 Revision "));
  if (connection == 0) {
    Serial.println(F("Failed to access Ethernet controller"));
  }
  
  Serial.println(F("Setting up DHCP"));
  Serial.print("Connected with IP: ");
  Serial.println(Ethernet.localIP());
  delay(1500);
}
