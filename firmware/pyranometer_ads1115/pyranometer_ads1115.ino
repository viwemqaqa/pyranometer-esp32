/*
 * Pyranometer logger
 * ESP32-DEV (ESP-WROOM-32) + ADS1115 + Kipp & Zonen thermopile pyranometer
 *
 * Wiring
 *   ADS1115 VDD  -> 3V3
 *   ADS1115 GND  -> GND
 *   ADS1115 SDA  -> GPIO21
 *   ADS1115 SCL  -> GPIO22
 *   ADS1115 ADDR -> GND   (address 0x48)
 *   Pyranometer red  (+) -> A0
 *   Pyranometer blue (-) -> A1, and link A1 to GND
 *   Cable shield -> GND at this end only
 *
 * Library: "Adafruit ADS1X15" via Library Manager
 */

#include <Wire.h>
#include <Adafruit_ADS1X15.h>

#define SDA_PIN 21
#define SCL_PIN 22

#define ADS_ADDR 0x48

// ---- Calibration -----------------------------------------------------------
// Take this from YOUR instrument's calibration certificate, in uV per W/m2.
// The value below is a placeholder typical of a CMP-series sensor.
const float SENSITIVITY_UV_PER_WM2 = 8.55f;

// GAIN_SIXTEEN gives +/-0.256 V full scale
const float LSB_VOLTS = 7.8125e-6f;

// Number of conversions averaged per reported sample
const int   AVG_SAMPLES = 16;

// Reporting interval
const unsigned long SAMPLE_INTERVAL_MS = 2000;
// ----------------------------------------------------------------------------

Adafruit_ADS1115 ads;
bool adsReady = false;
unsigned long lastSample = 0;

// Scans the bus once and reports what it finds.
int scanI2C() {
  int nDevices = 0;

  Serial.println("Scanning for I2C devices ...");
  for (byte address = 0x01; address < 0x7f; address++) {
    Wire.beginTransmission(address);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.printf("  I2C device found at address 0x%02X\n", address);
      nDevices++;
    } else if (error != 2) {
      Serial.printf("  Error %d at address 0x%02X\n", error, address);
    }
  }

  if (nDevices == 0) {
    Serial.println("  No I2C devices found");
  }
  return nDevices;
}

// Returns the averaged differential reading across A0/A1 in volts.
float readDifferentialVolts() {
  long sum = 0;
  for (int i = 0; i < AVG_SAMPLES; i++) {
    sum += ads.readADC_Differential_0_1();
  }
  return (sum / (float)AVG_SAMPLES) * LSB_VOLTS;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000);

  scanI2C();

  if (!ads.begin(ADS_ADDR, &Wire)) {
    Serial.printf("ADS1115 not responding at 0x%02X. Check wiring and ADDR pin.\n",
                  ADS_ADDR);
    return;
  }

  ads.setGain(GAIN_SIXTEEN);            // +/-0.256 V, 7.8125 uV per LSB
  ads.setDataRate(RATE_ADS1115_8SPS);   // slowest rate, lowest noise

  adsReady = true;
  Serial.println("ADS1115 ready. Logging irradiance.");
  Serial.println("volts_mV\tirradiance_Wm2");
}

void loop() {
  if (!adsReady) {
    // Nothing to read. Re-scan slowly so the fault is visible on the monitor.
    delay(5000);
    scanI2C();
    return;
  }

  if (millis() - lastSample < SAMPLE_INTERVAL_MS) return;
  lastSample = millis();

  float volts = readDifferentialVolts();
  float irradiance = (volts * 1e6f) / SENSITIVITY_UV_PER_WM2;

  Serial.printf("%8.4f\t%8.2f\n", volts * 1000.0f, irradiance);
}
