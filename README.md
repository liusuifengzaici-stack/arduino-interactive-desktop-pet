# Arduino Interactive Desktop Pet

**DesktopPet V14.1 · Arduino UNO R3 · Functional educational prototype**

A physical desktop companion that reacts to touch with facial expressions, head movements, sound effects and dialogue. An infrared remote switches between emotional interaction, estimated-temperature display and a simplified buzzer melody.

Developed as an undergraduate Communication Engineering project, DesktopPet demonstrates how sensors, actuators and displays can cooperate in one embedded system. V14.1 is the final implementation selected by the student after physical testing. The application remains a single Arduino sketch.

> Evidence: hardware behavior and debugging history are student-reported. Repository checks and new build results are recorded separately in [Testing](docs/testing.md). This is an educational prototype, with known limitations.

## Features

- Active-HIGH capacitive touch input with 40 ms debounce and three increasing anger levels.
- Predefined SG90 head movements, incremental position control and emotion sound effects.
- SSD1306 animated normal, slightly angry, angry, very angry, temperature, singing and paused expressions.
- Parallel LCD1602 dialogue, estimated temperature and phrase-based lyric display.
- Filtered thermistor ADC input with hot-state hysteresis and delayed numeric updates.
- Four NEC remote commands, including music pause/resume that preserves remaining event and tone time.
- Cooperative `millis()` scheduling and flash-resident melody/lyric data.

## Hardware Components

| Component | Purpose |
| --- | --- |
| Arduino UNO R3 | Main controller |
| SSD1306 128×64 I2C OLED | Facial expressions and animations |
| LCD1602, parallel interface | Dialogue, temperature estimate and music information |
| TTP223 capacitive touch sensor | Head-touch input |
| Thermistor circuit | Analog temperature-related input |
| SG90 micro servo | Head motion |
| Passive buzzer | Single-voice melody and emotion tones |
| NEC infrared receiver | Remote input |
| Infrared remote control | Mode selection and playback control |

The vibration sensor and physical buttons were removed from the final design. Supply wiring, thermistor divider values and module electrical ratings require physical verification; the sketch establishes signal assignments only.

## Pin Connections

| UNO pin | Signal |
| --- | --- |
| D3 | TTP223 OUT |
| A0 | Thermistor analog input |
| A2 | IR receiver output |
| D8 | Passive buzzer signal |
| D9 | SG90 control signal |
| A4 | OLED SDA |
| A5 | OLED SCL |
| D5 | LCD RS |
| D6 | LCD E |
| D7 | LCD D4 |
| D10 | LCD D5 |
| D11 | LCD D6 |
| D12 | LCD D7 |

See [Wiring](docs/wiring.md) for shared-ground guidance, servo power considerations and unconfirmed electrical details.

## How It Works

**Touch sensor → Arduino emotion processing → Servo + OLED + Buzzer + LCD**

In idle mode, a debounced LOW-to-HIGH touch transition raises `angerLevel`, up to 3. Each new touch starts the matching head-motion and tone sequence. Holding a touch does not repeatedly increment the level. After release and at least five seconds since the last accepted touch, the pet calms down and returns its head toward 90°.

The remote selects idle, temperature or music mode. Touch state is still tracked in every mode, but emotional actions occur only in idle. Temperature sampling continues in every mode. The green button controls playback only in music mode.

## Software Architecture

| Part | Responsibility |
| --- | --- |
| `setup()` | Starts Serial at 9600 baud, configures pins and displays, attaches/centers the servo, seeds ADC/touch state and initializes TinyIRReceiver |
| `loop()` | Services remote, touch and servo; samples temperature; advances music or emotion sounds; refreshes LCD and schedules OLED |
| State management | `mode`, `angerLevel` and `musicPaused` select behavior without a complex class hierarchy |
| Touch processing | Debounces input and starts emotion-linked movement and sound |
| Servo movement | Moves one degree per eligible 18 ms update; waits 150 ms at each target |
| Music playback | Reads `SongNote` events from `PROGMEM`; schedules notes, rests and phrase indices |
| LCD1602 updates | Existing mode/value caches trigger full padded 16-character row writes when content changes |
| OLED rendering | Page-buffer graphics, scheduled every 100 ms |
| Temperature sampling | Eight ADC readings every 250 ms, smoothing, empirical estimate and hysteresis |

`millis()` deadlines let the loop revisit other hardware while waiting for a note, movement or animation interval. Display transfers, ADC reads and Serial output still take finite execution time; this is cooperative scheduling, not a real-time guarantee. No long `delay()` calls are used.

Read the [system diagrams](docs/system-design.md) and [variables/function guide](docs/code-explanation.md) for an explanation suitable for a project discussion.

## Remote Control Instructions

| Button | NEC command | Action |
| --- | --- | --- |
| 1 | `0x0C` | Software-state reset to idle and center movement |
| 2 | `0x18` | Enter temperature mode |
| 3 | `0x5E` | Start/restart music from the beginning |
| Green play/pause | `0x43` | Pause/resume music; ignored outside music mode |

The receiver accepts address `0x00`, discards parity failures and repeat frames, and suppresses identical commands arriving less than 160 ms apart. Buttons 4 and 5 have no assigned function. Button 1 calls `resetPet()`; it does not restart the microcontroller or rerun `setup()`.

## Getting Started

1. Verify actual module ratings, power wiring, shared ground and head clearance using [Wiring](docs/wiring.md), with power disconnected while changing connections.
2. Install Arduino IDE and the **Arduino AVR Boards** package. Open `DesktopPet/DesktopPet.ino`; the folder and sketch names must remain `DesktopPet`.
3. Use Library Manager for the dependencies below. Instructions: [Arduino library installation](https://docs.arduino.cc/software/ide-v2/tutorials/ide-v2-installing-a-library/).
4. Select **Arduino Uno** and the correct connected port. Click **Verify**, then **Upload**. Uploading replaces the sketch currently on the board.
5. Open Serial Monitor at **9600 baud**. Check `IR ready: YES` and `DesktopPet V14.1 READY`, then exercise the demonstration sequence.

| Dependency/header | Source |
| --- | --- |
| `U8g2lib.h` | [U8g2](https://github.com/olikraus/u8g2) |
| `TinyIRReceiver.hpp` | [IRremote](https://github.com/Arduino-IRremote/Arduino-IRremote); the sketch uses its Tiny receiver |
| `Servo.h` | Servo library |
| `LiquidCrystal.h` | LiquidCrystal library, parallel LCD version |
| `Arduino.h`, `Wire.h`, `avr/pgmspace.h`, `string.h` | Arduino AVR core/toolchain headers; no separate library installation |

The sketch uses hardware I2C for the OLED and `LiquidCrystal(5, 6, 7, 10, 11, 12)` for the LCD; an I2C LCD library is not a dependency. The locally checked dependency versions and build result are in [Testing](docs/testing.md). The original hardware-testing library versions were not supplied.

Optional command-line verification, after installing the same core and libraries:

```sh
arduino-cli compile --fqbn arduino:avr:uno DesktopPet
```

## Demonstration

**Photo placeholder:** authentic whole-prototype photograph pending.

**Video placeholder:** authentic demonstration recording pending; see [asset guidance](assets/README.md).

1. Show the normal face and idle greeting.
2. Touch and release repeatedly, keeping accepted touches within five seconds, to show levels 1, 2 and 3.
3. Show the corresponding servo head movements, dialogue and buzzer tones; then release and observe calming.
4. Press button 2 to show estimated temperature and the temperature-related face.
5. Press button 3 to start the simplified music and singing animation.
6. Pause and resume with the green button, showing the paused display and preserved playback position.
7. Press button 1 to return to idle using the software-state reset.

## Challenges and Debugging

These are **student-reported observations**, not independently repeated measurements:

| Challenge | Observation and response |
| --- | --- |
| Complex wiring | Many modules shared one UNO; loose connections sometimes caused unstable behavior |
| Servo issues | Unexpected motion or missed response led to investigation of wiring and control logic |
| Infrared remote problems | Some commands occasionally became unresponsive; Serial Monitor was used to inspect received commands |
| LCD1602 issues | A potentiometer adjustment was needed for contrast; minor visible text transitions remain |
| Vibration sensor issue | Adding this sensor caused the circuit to stop functioning properly; it was removed for stability. The precise electrical cause was not confirmed |

V14.2 attempted to reduce LCD flicker but introduced other problems during the student's physical testing. V14.1 was retained. This repository preserves its existing LCD implementation, servo timing, input logic, temperature calculations and music data. See [review observations](docs/testing.md#source-review-observations) for unmodified edge cases.

## Technical Skills Demonstrated

- Arduino programming and state-based embedded control.
- Sensor and actuator interfacing and multi-peripheral system integration.
- Analog sampling/filtering and digital input debounce.
- Non-blocking timing, event sequencing and flash-memory data storage.
- Hardware debugging, Serial diagnostics and peripheral communication.

## Limitations

- Thermistor Celsius values are empirical estimates, without independent calibration or laboratory-grade accuracy.
- Music is a simplified educational single-voice arrangement labeled *You Are My Sunshine*. The passive buzzer produces tones, not vocals, and does not reproduce an exact original arrangement.
- LCD1602 text transitions may briefly flicker. State-based update suppression already exists in V14.1; no new character caching has been added.
- This is an educational prototype, not a commercial or custom PCB product. Electrical details and sustained stability need documented physical verification.
- Original library versions and authentic demonstration media remain to be recorded. The program is not claimed to be bug-free.

## Future Improvements

Future plans, not implemented features: a compact circuit layout, more reliable physical connections, a protective enclosure, improved power distribution and more advanced mechanical design.

## AI Assistance and Attribution

This project was developed with AI-assisted programming. The hardware assembly, system integration, physical testing, debugging observations, and design decisions were carried out by the student.

Repository organization and explanatory documentation were also prepared with AI assistance and checked against the supplied V14.1 sketch. Newly verified results are limited to the checks explicitly recorded in the testing document.

## Repository Guide

```text
DesktopPet/DesktopPet.ino   Final V14.1 application (personal comment removed)
docs/wiring.md             Signal connections and electrical unknowns
docs/system-design.md      Architecture and four Mermaid diagrams
docs/code-explanation.md   Variables, functions and hardware behavior
docs/testing.md            Evidence, build result and test checklist
assets/README.md           Authentic-media placeholders
.gitignore                Local/private and generated-file exclusions
```

No license has been selected or added. The project owner has approved public publication. See [Publishing](docs/publishing.md) if the remote repository still needs to be created.
