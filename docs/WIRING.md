# Wiring Reference

Quick reference. The reasoning behind these choices is in the main
[README](../README.md).

## Signal chain

```
  Pyranometer                 ADS1115                    ESP32-DEV
  (passive thermopile)        (16-bit ADC)               (ESP-WROOM-32)

   Red  (+ Hi) ----[470R]----> A0  ]
                              |    ] differential pair
                       100nF ===    ] read as A0 - A1
                              |    ]
   Blue (- Lo) ----[470R]----> A1  ]
                                +---> GND     <-- the critical link
                                                   (land it at the module)

   Shield ----------------------> GND
   (logger end only)

                              VDD ----------------------> 3V3
                              GND ----------------------> GND
                              SDA ----------------------> GPIO21
                              SCL ----------------------> GPIO22
                             ADDR ----> GND  (0x48)
                             ALRT      n/c
```

The 470 Ω resistors and 100 nF capacitor are optional. Fit them for long cable
runs or noisy environments.

## Pyranometer colour code (CMP series)

| Wire | Function | Connect to |
|---|---|---|
| Red | + (Hi) | A0 |
| Blue | - (Lo) | A1 + GND |
| Shield | Housing | GND, logger end only |

## Albedometer colour code (CMA series)

The upper sensor uses the red/blue pair above. The lower sensor adds:

| Wire | Function | Connect to |
|---|---|---|
| Green | + (Hi) | A2 |
| Yellow | - (Lo) | A3 + GND |

Read the lower sensor with `readADC_Differential_2_3()` and apply its own
sensitivity value.

Albedo is then:

```
albedo = E_lower / E_upper
```

## Optional temperature sensor

Some units carry a Pt-100 or a 10 kΩ thermistor for body temperature. These are
resistance outputs and cannot be read by the ADS1115 directly. They need a
ratiometric divider or a dedicated RTD front end. Wire colours and the
resistance-to-temperature conversion polynomials are on the manufacturer
instruction sheet in this folder.

## Rules that matter

1. **ADS1115 VDD goes to 3V3, never 5V.** The module's I2C pull-ups reference
   VDD, and the ESP32 is not 5V tolerant.
2. **ADDR must be tied, not floating.** GND gives 0x48.
3. **A1 must be linked to GND.** A floating thermopile has no common-mode
   reference and the inputs will drift out of range.
4. **Ground the shield at one end only.** Grounding both ends creates a loop
   that appears as ripple on the irradiance curve.
5. **Do not use GPIO12 on the ESP32 for anything else.** It is a strapping pin
   and a pull-up at reset can stop the board booting.
