# Calibration and Gain Selection

## The sensitivity constant

Every pyranometer ships with a calibration certificate carrying a measured
sensitivity `S`, in microvolts per W/m². This value is unique to the individual
instrument, and it is the whole calibration.

Set it in the firmware:

```cpp
const float SENSITIVITY_UV_PER_WM2 = 8.55f;   // <-- from YOUR certificate
```

Do not use a typical value from a datasheet. Units of the same model differ
from one another, and substituting a generic figure introduces a fixed
percentage error into every reading you will ever take with the instrument.

Recalibration is recommended every two years. When the unit comes back, update
this constant.

## Irradiance calculation

```
E = U / S
```

- `E` irradiance, W/m²
- `U` thermopile output, µV
- `S` sensitivity, µV per W/m²

## Working from ADC counts

The ADS1115 is a 16-bit bipolar converter, so a full-scale range of ±FSR maps
to ±32768 counts.

| Gain setting | FSR | µV per count |
|---|---|---|
| GAIN_TWOTHIRDS | ±6.144 V | 187.5 |
| GAIN_ONE | ±4.096 V | 125 |
| GAIN_TWO | ±2.048 V | 62.5 |
| GAIN_FOUR | ±1.024 V | 31.25 |
| GAIN_EIGHT | ±0.512 V | 15.625 |
| **GAIN_SIXTEEN** | **±0.256 V** | **7.8125** |

Use `GAIN_SIXTEEN`. It is the highest gain the part offers and the signal never
comes close to filling even that range.

```
U [µV]   = counts × 7.8125
E [W/m²] = (counts × 7.8125) / S
```

## Headroom check

Worked for a sensitivity of 8.55 µV per W/m²:

| Irradiance | Output | Counts at gain 16 | Fraction of range |
|---|---|---|---|
| 1 W/m² | 8.55 µV | 1.1 | 0.003 % |
| 100 W/m² | 0.86 mV | 109 | 0.3 % |
| 1000 W/m² | 8.55 mV | 1095 | 3.3 % |
| 2000 W/m² | 17.1 mV | 2189 | 6.7 % |
| 4000 W/m² | 34.2 mV | 4377 | 13.4 % |

Resolution works out to `7.8125 / 8.55` which is about **0.91 W/m² per count**.

Note how little of the range is used even at extreme irradiance. That is
unavoidable with a 16-bit converter capped at gain 16, and it is the main
argument for a dedicated instrumentation amplifier ahead of the ADC if you
later need finer resolution. Adding a gain of 10 in front would bring
1000 W/m² up to a third of full scale and take resolution below 0.1 W/m², at
the cost of having to characterise the amplifier's own offset and drift.

## Resolution is not accuracy

The 0.91 W/m² figure is the quantisation step only. Real uncertainty is
dominated by other terms:

| Source | Rough scale |
|---|---|
| Instrument calibration uncertainty | typically low single-digit percent |
| Cosine and directional response error | varies with solar elevation |
| Temperature response of the thermopile | percent-level over the full range |
| Dome soiling | unbounded, and always in the direction of reading low |
| Levelling error | systematic, varies through the day |
| ADC noise and quantisation | well under 1 W/m² with averaging |

The electrical side of this project is not the limiting factor. Keeping the
dome clean and the instrument level will do more for your data quality than any
change to the ADC configuration.

## Noise reduction in firmware

Two settings, both already in the sketch:

```cpp
ads.setDataRate(RATE_ADS1115_8SPS);   // slowest rate, lowest noise
const int AVG_SAMPLES = 16;           // average per reported reading
```

The ADS1115's noise falls substantially at lower data rates. At 8 SPS, 16
conversions take about two seconds, which sets the reporting interval. Faster
sampling is available but buys nothing here: irradiance does not change
meaningfully on sub-second timescales except during cloud edges.

## Verifying a new build

1. Cover the dome completely. The reading should fall to near zero, and may go
   slightly negative.
2. Uncover in daylight. The reading should rise immediately and settle.
3. Compare a clear-sky solar-noon peak against roughly 1000 W/m² at sea level.
4. Leave it running overnight. A small negative offset of a few W/m² is normal
   and indicates a healthy instrument. A hard zero overnight suggests you are
   reading single-ended instead of differential.
