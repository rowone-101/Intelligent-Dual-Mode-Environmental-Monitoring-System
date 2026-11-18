# Intelligent Dual-Mode Environmental Monitoring System

![Platform](https://img.shields.io/badge/MCU-ESP8266%20NodeMCU-blue)
![ADC](https://img.shields.io/badge/ADC-ADS1115-informational)
![Firmware](https://img.shields.io/badge/Firmware-Arduino%20C%2B%2B-00979D)
![Status](https://img.shields.io/badge/Status-Prototype-green)

A low-power IoT node that monitors **air quality** (CO, gases, temperature, humidity) and **soil condition** (moisture, rain) on a single NodeMCU ESP8266. Two **AO3400A N-channel MOSFETs** switch the sensor blocks ON one at a time, so the power-hungry gas-sensor heaters run only while the air block is being measured.

---

## Key Features

- **Two sensor blocks, one controller:** air block (MQ-7, MQ-135, DHT11) and soil block (rain module, capacitive soil probe).
- **MOSFET block switching:** low-side AO3400A switches the ground return of each block; LEDs show the active block (green = air, blue = soil).
- **Single 16-bit ADC:** all four analog sensors are read through one ADS1115 over I²C.
- **Safe level shifting:** 12 kΩ / 20 kΩ dividers bring the 5 V MQ outputs to about 3.1 V for the 3.3 V ADC.
- **Power study:** continuous versus alternating operation compared with a duty-cycle model.
- **TerraSense dashboard** (built by teammates) for statistics, charts and CSV archive.

## System Overview


| Block | Sensors | Supply | Switch |
|---|---|---|---|
| Air | MQ-7, MQ-135, DHT11 | 5 V (MQs), 3.3 V (DHT11) | Q1 (AO3400A), gate on D5 |
| Soil | Rain module, capacitive soil probe v1.2 | 3.3 V | Q2 (AO3400A), gate on D6 |

## Hardware

| Component | Qty | Purpose |
|---|---|---|
| NodeMCU ESP8266 (ESP-12E) | 1 | Controller, block sequencing, Wi-Fi |
| ADS1115 16-bit ADC | 1 | Reads MQ-7, MQ-135, rain, soil (I²C, addr 0x48) |
| AO3400A N-MOSFET | 2 | Low-side switch for each block |
| MQ-7 / MQ-135 | 1 each | CO and air-quality sensing |
| DHT11 | 1 | Temperature and humidity |
| Rain sensor module | 1 | Rain and wetness |
| Capacitive soil moisture sensor v1.2 | 1 | Soil moisture |
| LEDs (green, blue) | 1 each | Active-block indicators |
| Resistors | - | 12 kΩ and 20 kΩ (dividers), 150 Ω (gates), 1 kΩ (gate pull-downs), 220 Ω (LEDs), 4.7 kΩ (DHT11 pull-up) |
| Regulated 5 V supply | 1 | MQ heaters and NodeMCU VIN |

### Pin Mapping

| NodeMCU pin | Function |
|---|---|
| D1 / GPIO5 | ADS1115 SCL |
| D2 / GPIO4 | ADS1115 SDA |
| D0 / GPIO16 | DHT11 data |
| D5 / GPIO14 | Air block control (Q1 gate) + green LED |
| D6 / GPIO12 | Soil block control (Q2 gate) + blue LED |

| ADS1115 input | Sensor |
|---|---|
| A0 | MQ-7 (through divider) |
| A1 | MQ-135 (through divider) |
| A2 | Rain module AO |
| A3 | Soil probe AOUT |

### How the switching works

Sensor VCC pins stay on their supply rail. Each block's **ground pin goes to the MOSFET drain**, and the MOSFET source goes to the common ground.

1. GPIO HIGH (3.3 V) turns the MOSFET ON, the current loop is complete and the block works.
2. GPIO LOW (0 V) turns it OFF, the loop is broken and the MQ heaters draw no current.
3. The gate gets a 150 Ω series resistor and a pull-down so the block stays OFF while the NodeMCU boots.

> **Wiring rule:** every sensor ground in a block connects only to that block's switched ground, never also to the main ground. Otherwise the switch is bypassed.

## Power Saving Analysis

Estimated ON-state power (typical datasheet currents):

| Block | Load | Power |
|---|---|---|
| Air | MQ-7 + MQ-135 heaters, 5 V x 0.30 A | 1.500 W |
| Soil | Rain + soil probe, 3.3 V x 0.02 A | 0.066 W |

`P_avg = P_air x D_air + P_soil x D_soil`  and  `Saving % = (P_cont - P_avg) / P_cont x 100`

| Mode | Air ON | Soil ON | P_avg | Saving |
|---|---|---|---|---|
| Continuous | 100% | 100% | 1.566 W | 0% |
| Alternating A | 30% | 10% | 0.457 W | 70.8% |
| Alternating B | 40% | 10% | 0.607 W | 61.3% |
| Alternating C | 50% | 10% | 0.757 W | 51.7% |

The air block (heaters) accounts for about 95% of the saving. These are **estimates**; measured values from a series current meter will replace them.

## Firmware

Written in Arduino C++ as a non-blocking `millis()` state machine:

1. Initialise D5, D6 (LOW), I²C, ADS1115 (0x48) and DHT11.
2. **Air block:** D5 HIGH, warm-up, average several ADC samples from A0, A1, read DHT11, D5 LOW.
3. **Soil block:** D6 HIGH, settle, read A2, A3, D6 LOW.
4. Convert voltages (undo the divider with x1.6), apply thresholds, print the result, wait, repeat.

Block times (warm-up, ON time, sampling interval) are constants at the top of the sketch.

Sample serial output:

```text
[09:00:02] AIR  ON | MQ7=1.24 ppm | MQ135=41.8 ppm | T=27.1 C | H=66.8 %
[09:00:05] SOIL ON | Soil=49.6 % | Rain=21.5 % (DRY)
[09:00:07] ALL BLOCKS OFF | waiting for next sampling interval
```

## TerraSense Dashboard

Developed by teammates. It loads the sensor log as a CSV (120 records, 9 columns) and shows statistics, a dataset archive with CSV download, histogram, rain-status pie chart and trend plot. The current version uses a static CSV; live Wi-Fi data is the next step.

## Repository Structure

```text
.
├── firmware/            # Arduino sketch
├── hardware/            # Schematic, wiring table, bill of materials
├── dashboard/           # TerraSense web dashboard
├── data/                # Sample CSV (terrasense.csv)
├── docs/
│   ├── images/          # Diagrams and screenshots
│   └── presentation.pdf # Project presentation
└── README.md
```

## Getting Started

1. Wire the circuit as per the diagram and pin mapping above.
2. Install the Arduino IDE with the **ESP8266** board package.
3. Install the libraries **Adafruit ADS1X15** and **DHT sensor library** (Library Manager).
4. Select **NodeMCU 1.0 (ESP-12E)**, open the sketch in `firmware/` and upload.
5. Open the Serial Monitor at **115200 baud**.

> Power the circuit from the external 5 V supply **or** USB, never both at once.

## Results and Limitations

**Results**
- Air and soil parameters read together by one NodeMCU and one ADS1115.
- Only one block powered at a time, shown by the green or blue LED.
- Estimated 52% to 71% lower average power than continuous operation.

**Limitations**
- Power savings are estimates; hardware measurement is pending.
- MQ-7 runs at a constant 5 V (no 1.4 V low-heat cycle), so CO values are indicative.
- MQ sensors need warm-up and calibration; DHT11 accuracy is about +/-2 °C and +/-5 % RH.
- About 0.1 mA leakage remains on an OFF block through the analog divider.
- Dashboard uses a static CSV for now.

## Future Work

- Measure real block currents and publish the power-saving results.
- Calibrate MQ and soil sensors against reference values.
- Add MQ-7 heater cycling with a dedicated switching channel.
- Stream live data over Wi-Fi (MQTT or HTTP) to the dashboard.

## References

- ESP8266EX, ADS111x and AO3400A datasheets
- Adafruit ADS1X15 and DHT sensor libraries
- Sedra and Smith, *Microelectronic Circuits*; Horowitz and Hill, *The Art of Electronics*

