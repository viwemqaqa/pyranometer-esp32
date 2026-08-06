/*
 * I2C bus scanner
 *
 * Run this first. You are looking for a device at 0x48, which is the ADS1115
 * with its ADDR pin tied to GND.
 *
 * If you see 0x49, 0x4A or 0x4B instead, ADDR is tied to VDD, SDA or SCL
 * respectively. Either move the wire or change ADS_ADDR in the logger sketch.
 *
 * If you see nothing at all, check power first, then that SDA and SCL are not
 * swapped, then that ADDR is actually connected to something.
 */

#include <Wire.h>

#define SDA_PIN 21
#define SCL_PIN 22

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  Serial.printf("I2C scanner on SDA=%d SCL=%d\n", SDA_PIN, SCL_PIN);
}

void loop() {
  int nDevices = 0;

  Serial.println("Scanning for I2C devices ...");
  for (byte address = 0x01; address < 0x7f; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();

    if (error == 0) {
      Serial.printf("  I2C device found at address 0x%02X", address);
      if (address >= 0x48 && address <= 0x4B) {
        Serial.print("   <- looks like an ADS1115");
      }
      Serial.println();
      nDevices++;
    } else if (error != 2) {
      // error 2 is a normal address NACK, meaning nothing is there.
      Serial.printf("  Error %d at address 0x%02X\n", error, address);
    }
  }

  if (nDevices == 0) {
    Serial.println("  No I2C devices found");
  }

  delay(5000);
}
