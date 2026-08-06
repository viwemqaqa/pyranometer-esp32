# Pyranometer Logger (ESP32 + ADS1115)

Reading solar irradiance from a Kipp & Zonen CMP-series pyranometer using an
ESP32 and a 16-bit ADS1115 ADC.

Covers the CMP6, CMP10, CMP11, CMP21 and CMP22 pyranometers, and the CMA6 and
CMA11 albedometers.

---

## 1. What the sensor is and what it does

A pyranometer measures **global horizontal irradiance**: the total shortwave
solar power falling on a horizontal surface, in watts per square metre.

Inside the glass dome is a **thermopile**, a series chain of thermocouple
junctions. Sunlight is absorbed by a black coating on the hot junctions while
the cold junctions stay tied to the instrument body. The resulting temperature
difference produces a voltage by the Seebeck effect, and that voltage is very
nearly proportional to the absorbed radiant power.

Three consequences follow, and they shape the whole design of this project:

**The sensor is passive.** There is no supply pin, no excitation, no ground
reference of its own. It is a floating voltage source. Nothing on this board
powers the pyranometer.

**The output is tiny.** Full sunlight produces something in the region of ten
millivolts. Individual sensitivities vary by unit but sit broadly in the range
of 5 to 20 microvolts per W/m². This is why a dedicated 16-bit ADC with a
programmable gain amplifier is used instead of the ESP32's own 12-bit ADC,
which is both too coarse and notoriously non-linear near the rails.

**The output can go negative.** At night the dome radiates heat to the cold sky
and cools below the body, reversing the temperature gradient. A healthy
instrument reads a small negative value overnight, typically a few W/m². This
is a useful diagnostic, so the firmware reads bipolar rather than unipolar.

The glass dome shields the thermopile from wind and rain and sets the spectral
window to roughly the shortwave solar band. The dome also has to be kept clean:
a dirty dome reduces the reading, which is a silent failure mode with no
electrical symptom.

### Why measure it

Irradiance is the input variable for anything solar. Common uses:

- PV array performance ratio and yield analysis
- Solar resource assessment for a site before installation
- Reference measurement for comparing panel technologies
- Ground truth for satellite-derived or model-derived irradiance
- Meteorological and atmospheric research

---

## 2. Hardware

| Item | Notes |
|---|---|
| Kipp & Zonen CMP-series pyranometer | Passive thermopile, 2-wire plus shield |
| ADS1115 breakout | 16-bit, 4-channel, I2C, PGA to x16 |
| ESP32-DEV (ESP-WROOM-32) | Any ESP32 board works; pins below are the classic devkit |
| Jumper wire | For the A1 to GND link, see section 4 |
| 2x 470 Ω, 1x 100 nF | Optional input filter, see section 5 |

The full manufacturer instruction sheet is in
[`docs/`](docs/KippZonen_InstructionSheet_Pyranometers_Albedometers_CMP_CMA_series_V1603.pdf).

---

## 3. Wiring

### ADS1115 to ESP32

| ADS1115 | ESP32 | Note |
|---|---|---|
| VDD | 3V3 | Not 5V. The module's I2C pull-ups sit on VDD and the ESP32 is not 5V tolerant |
| GND | GND | |
| SDA | GPIO21 | Default hardware I2C |
| SCL | GPIO22 | Default hardware I2C |
| ADDR | GND | Sets address `0x48`. Do not leave floating |
| ALRT | not connected | |

Address options: ADDR to GND gives `0x48`, to VDD gives `0x49`, to SDA gives
`0x4A`, to SCL gives `0x4B`.

### Pyranometer to ADS1115

Colour code per the manufacturer instruction sheet:

| Wire | Function | Connect to |
|---|---|---|
| **Red** | + (Hi) | A0 |
| **Blue** | - (Lo) | A1, **and link A1 to GND** |
| **Shield** | Housing | GND, at the logger end only |

For a **CMA albedometer** the lower (downward-facing) sensor uses a second
pair: **green** is + (Hi) and **yellow** is - (Lo). Wire that pair to A2 and A3,
link A3 to GND the same way, and read it with
`readADC_Differential_2_3()`. Each of the two sensors has its own separate
sensitivity value on the certificate.

Some units carry an optional Pt-100 or thermistor for body temperature on
additional wires (brown, grey, green, yellow depending on option). Those are
resistance outputs, not voltage outputs, and need a different front end than
the ADS1115. The conversion formulas are on the instruction sheet if you decide
to add them later.

---

## 4. Why A1 is linked to ground

This is the single most common failure in a first build, so it gets its own
section.

The ADS1115 does not have isolated differential inputs. It has four
single-ended inputs, and "differential" mode simply amplifies the difference
between two of them. Each input must independently stay within the common-mode
range of GND to VDD.

The thermopile is a floating source. With nothing referencing it to the board's
ground, both inputs drift under the ADC's input bias current until they leave
the common-mode window. The symptom is nasty because it is not an immediate
failure: readings look plausible for a while, then wander, then rail.

Linking A1 to GND pins the low side at 0 V and gives the pair a defined DC
reference. You still read differentially, so you keep the bipolar range and the
negative night-time offset.

**Land the link at the ADS1115 terminal block itself**, not on the ESP32's
ground pin. At 7.8 µV per count, the millivolt-scale IR drop along a ground
wire carrying return current is a difference you can measure.

---

## 5. Input impedance and filtering

The instruction sheet specifies a readout impedance above 1 MΩ. The ADS1115 at
gain 16 presents roughly 710 kΩ differential, which is below that figure. It is
still fine in practice, and the arithmetic is worth understanding rather than
taking on faith.

The loading error is a simple divider between the thermopile's source impedance
and the ADC's input impedance:

```
error = Rsource / (Rsource + Rinput)
```

CMP-series thermopiles have a source impedance in the tens to low hundreds of
ohms. At 100 Ω into 710 kΩ the error is about 0.014 percent, roughly two orders
of magnitude below the calibration uncertainty of the instrument itself. The
1 MΩ figure is a conservative recommendation aimed at ruling out high-impedance
readouts, not a hard threshold.

**Optional input filter.** Put 470 Ω in series with each of A0 and A1, then
100 nF across the two inputs. This damps the switched-capacitor sampling
transients at the ADC input and rejects mains pickup on a long cable.

Keep the series resistors at or below 1 kΩ. They add directly to the source
impedance: two 470 Ω resistors plus a 100 Ω thermopile gives about 1.04 kΩ, so
the loading error rises to roughly 0.15 percent. Still acceptable, but larger
resistors would start eating into accuracy.

---

## 6. The formulas

### Irradiance

From the instruction sheet:

```
E = U / S
```

| Symbol | Meaning | Unit |
|---|---|---|
| `E` | Solar irradiance | W/m² |
| `U` | Thermopile output voltage | µV |
| `S` | Sensitivity | µV per W/m² |

`S` is **specific to your individual instrument** and is printed on its
calibration certificate. Do not substitute a datasheet typical value. That
number is the entire calibration, and using someone else's makes every reading
you take wrong by a fixed percentage.

### From ADC counts

At `GAIN_SIXTEEN` the full-scale range is ±0.256 V across 32768 counts:

```
LSB     = 0.256 / 32768 = 7.8125 µV
U [µV]  = counts × 7.8125
E [W/m²] = (counts × 7.8125) / S
```

With a sensitivity around 8.5 µV per W/m² that works out to roughly
**0.92 W/m² per count**, and 1500 W/m² of irradiance lands near 12.8 mV,
comfortably inside the ±256 mV range. Gain 16 is the highest the ADS1115
offers, so this is the best resolution available from this part.

### Resolution and noise

Resolution is not the same as accuracy. The 0.92 W/m² per count figure is the
quantisation step; the actual noise floor depends on data rate. The firmware
uses `RATE_ADS1115_8SPS`, the slowest setting and therefore the quietest, and
averages 16 conversions per reported sample. That takes about two seconds per
reading, which is why the reporting interval is set to 2000 ms.

---

## 7. Installation

Placement affects the measurement more than any electrical choice you make.
Summarising the instruction sheet:

- **Unobstructed horizon.** Any obstacle should be at a distance greater than
  ten times its height above the sensor.
- **Level the instrument** using the built-in bubble level and the adjustment
  screws. A tilted pyranometer has a systematic cosine error that varies
  through the day.
- **Point the cable towards the nearest pole**, so the body shades the cable
  entry rather than the dome.
- **Mount on a solid surface** with the supplied screws, washers and nylon
  rings, and fit the sun screen.
- **Cable length up to 100 m** is supported.
- For an **albedometer**, mount about 1.5 m above short cut grass.
- Minimise interference from the mounting structure itself.

Operating range is -40 to +80 °C, and the housing is IP67.

---

## 8. Maintenance

| Task | Interval |
|---|---|
| Clean the dome with water or alcohol | Regularly. A dirty dome reads low |
| Check the instrument is still level | Regularly |
| Replace the desiccant when it turns clear | As needed. CMP10 desiccant is replaced at factory recalibration |
| Recalibration | Every 2 years |

Keep the original packaging. You will need it to ship the unit for
recalibration.

---

## 9. Sanity check your readings

Typical daytime values from the instruction sheet:

| Sky condition | Irradiance |
|---|---|
| Fully clouded | 50 to 120 W/m² |
| Sunny, partly clouded | 120 to 500 W/m² |
| Clear and sunny | 500 to 1000 W/m² |

Clear-sky peak at solar noon should land near 1000 W/m² at sea level. Values
well above that in normal conditions point to a wrong sensitivity constant or a
gain setting mismatch. Values stuck near zero in daylight point to a swapped or
open signal pair.

---

## 10. Firmware

Two sketches:

- [`firmware/i2c_scan/`](firmware/i2c_scan) - bus scanner. Run this first and
  confirm `0x48` appears before going any further.
- [`firmware/pyranometer_ads1115/`](firmware/pyranometer_ads1115) - the logger.

### Setup

1. Arduino IDE, **Tools > Board > ESP32 Arduino > ESP32 Dev Module**
2. Flash Size 4MB, Partition Scheme "Default 4MB with spiffs", Upload Speed
   921600 (drop to 115200 if uploads fail)
3. Library Manager: install **Adafruit ADS1X15**
4. Open `firmware/pyranometer_ads1115/pyranometer_ads1115.ino` and set
   `SENSITIVITY_UV_PER_WM2` from your certificate
5. Upload, then open Serial Monitor at 115200

If upload stalls at "Connecting.....", hold the BOOT button until writing
begins. If no port appears, install the CH340 or CP2102 driver depending on
which USB chip your board carries.

### Output

Tab-separated, ready to paste into a spreadsheet:

```
volts_mV	irradiance_Wm2
  8.1250	  950.29
  8.1172	  949.38
```

[`tools/serial_to_csv.py`](tools/serial_to_csv.py) will capture the stream to a
timestamped CSV if you would rather log to a file.

---

## 11. Troubleshooting

| Symptom | Likely cause |
|---|---|
| Scanner finds nothing | Power, SDA/SCL swapped, or ADDR floating |
| Device found at 0x49/0x4A/0x4B | ADDR tied somewhere other than GND. Update `ADS_ADDR` or move the wire |
| Readings negative all day | Red and blue swapped |
| Readings drift then rail | The A1 to GND link is missing. See section 4 |
| Readings plausible but consistently off by a fixed percentage | Wrong sensitivity constant |
| Reading gradually declining over weeks | Dirty dome, or desiccant exhausted |
| Noisy readings, visible mains ripple | Shield grounded at both ends, or no input filter |
| Value pegged at ±32767 | Gain too high for the signal, or an open input |

---

## 12. Repository layout

```
.
├── README.md
├── docs/
│   ├── KippZonen_InstructionSheet_...pdf   Manufacturer instruction sheet
│   ├── WIRING.md                           Connection reference
│   └── CALIBRATION.md                      Sensitivity and gain working
├── firmware/
│   ├── i2c_scan/                           Bus scanner
│   └── pyranometer_ads1115/                Logger
└── tools/
    └── serial_to_csv.py                    Serial capture to CSV
```

---

## Licence

Code in this repository is MIT licensed, see [LICENSE](LICENSE).

The Kipp & Zonen instruction sheet in `docs/` is copyright Kipp & Zonen B.V.
and is included for reference only. It is not covered by the MIT licence.
