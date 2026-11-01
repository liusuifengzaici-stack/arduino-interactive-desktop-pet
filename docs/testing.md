# Testing and Evidence — V14.1

## Evidence boundaries

Repository preparation and compilation were performed on **2026-11-01**. No board upload, hardware operation, temperature calibration or new demonstration recording was performed during this preparation. The student identifies V14.1 as the functional version selected after physical testing; those historical observations are distinct from the new checks below.

Use only **Passed**, **Failed** or **Not tested** for test status. A checklist expectation describes what to verify, not a result that has already been observed.

## Build verification

**Passed** — the publication sketch compiled for **Arduino UNO**, FQBN `arduino:avr:uno`, without changing application logic.

| Item | Verified local version/result |
| --- | --- |
| Arduino CLI | 1.5.1 |
| Arduino AVR Boards | 1.8.8 |
| IRremote | 4.7.1 |
| U8g2 | 2.36.19 |
| Servo | 1.3.0 |
| LiquidCrystal | 1.0.7 |
| Program storage | 25,290 / 32,256 bytes (78%) |
| Global/static dynamic memory | 1,314 / 2,048 bytes (64%); 734 bytes left for stack/local allocation |

```sh
arduino-cli compile --fqbn arduino:avr:uno DesktopPet
```

These are the versions used for the new build; they are not asserted to be the original hardware-testing versions. The first sandboxed attempt failed before compilation because Windows could not launch `avr-g++` (illegal system DLL relocation). Repeating the same compilation outside that sandbox succeeded. No source workaround or library replacement was made. Raw machine-path-bearing logs are retained locally outside the repository.

Compilation establishes syntax, linking and reported storage usage for this environment. It does not establish electrical correctness, runtime stack margin, reliable infrared reception, actual head travel or sustained operation with all modules active.

## Verification checklist

| Test | Procedure and expected observation | Status | Evidence |
| --- | --- | --- | --- |
| Compile for Arduino UNO | Compile the publication sketch with the recorded dependencies | Passed | New CLI build; result above |
| OLED initialization | Power on; check normal face and `HELLO!` caption | Not tested | New hardware test pending |
| LCD1602 text | Adjust verified contrast circuit; check idle text on both rows | Not tested | New hardware test pending |
| Touch response | Touch/release for more than debounce time; one event per rising edge | Not tested | New hardware test pending |
| Three emotional levels | Repeat touches within timeout; observe levels 1, 2, 3 and cap at 3 | Not tested | New hardware test pending |
| Calm timeout | Release touch; observe calm after at least 5 seconds from last accepted touch | Not tested | New hardware test pending |
| Servo head movement | Check 65/90, 65/115/90 and 60/120/60/120/90 commands with safe mechanical clearance | Not tested | New hardware test pending |
| Buzzer effects | Check one, two and three tone sequences corresponding to anger | Not tested | New hardware test pending |
| Thermistor interaction | Observe estimate and hot/cool face changes under a safe temperature change; record ADC and comparison instrument if available | Not tested | No calibration evidence supplied |
| IR button 1 | From temperature, playing and paused music, return to idle | Not tested | New hardware test pending |
| IR button 2 | From each mode, enter estimated-temperature display | Not tested | New hardware test pending |
| IR button 3 | From each mode, start/restart melody from beginning | Not tested | New hardware test pending |
| Green-button pause | Pause mid-note, in a note gap and during a rest; sound stops and paused face/text appears | Not tested | New hardware test pending |
| Green-button resume | Resume each case; saved event position and remaining tone/gap are preserved | Not tested | New hardware test pending |
| Music completion | Let final event duration elapse; expect idle/calm and center movement | Not tested | New hardware test pending |
| Reset behavior | Verify sound stops, calm state returns and centering is scheduled without a new startup message | Not tested | New hardware test pending |
| Mode isolation | Touch in temperature/music should not raise anger; green outside music should do nothing | Not tested | New hardware test pending |
| Held touch | Hold HIGH; verify one event and no automatic calm until released | Not tested | New hardware test pending |
| IR filtering | Check repeat rejection, supported address and rapid duplicate suppression using Serial at 9600 baud | Not tested | New hardware test pending |
| Long-duration stability | Record actual duration, power arrangement, mode cycles, symptoms and any resets | Not tested | No duration or stability log supplied |

For each physical run, record date, board/module details, supply arrangement, dependency versions, test steps, actual result and media/log references. Replace `Not tested` only with observed `Passed` or `Failed`, retaining failures and explanations. Record estimate error separately if comparing against a thermometer; do not call the empirical output calibrated without calibration evidence.

## Student-reported historical observations

- The hardware was assembled and integrated by the student, who selected V14.1 after physical testing.
- Loose connections sometimes caused instability across the multi-module circuit.
- The servo sometimes moved unexpectedly or did not respond; wiring and logic were investigated.
- Some IR commands occasionally became unresponsive; Serial Monitor was used to inspect received commands.
- LCD visibility required potentiometer contrast adjustment. Occasional text-transition flicker remains.
- A vibration sensor caused the circuit to stop functioning properly and was removed. The precise electrical cause was not confirmed.
- V14.2 attempted to improve LCD refresh but introduced other issues; it is excluded from this repository.

No original measurement log, timing trace, dependency-version record or demonstration media was supplied. These observations do not mark the new hardware checklist as passed.

## Source-review observations

The following edge cases and limitations were identified by reading the unchanged sketch. They are **not confirmed hardware failures** and have not been silently fixed:

1. **Touch assumptions:** processing expects active-HIGH transitions. A touch held during startup is initialized as already stable and needs release before a new event. A latching TTP223 configuration would produce different interaction behavior from momentary output; verify the actual module setting.
2. **Display versus temperature state:** `displayedTemp` starts at 25 and changes only after candidate confirmation. `hotState` follows filtered ADC hysteresis independently, so hot text can change before the integer does. Reset retains the filter and candidate history.
3. **LCD transitions:** on music entry, the title and a valid lyric can be written in the same refresh. Rows are padded and rewritten on relevant state changes, so visible transitions can remain. This is the existing V14.1 design.
4. **Initialization handling:** IR readiness is printed, but initialization failure does not halt the application. The sketch does not add a dedicated OLED failure diagnostic. Observe peripherals directly before declaring initialization successful.
5. **Timing and memory:** display transfers and sampling consume loop time, and scheduled tasks do not catch up every missed interval. The compiled 734-byte remaining RAM figure is not a measured worst-case stack margin.
6. **Animation versus audio:** the singing mouth uses the current event's frequency, which is not cleared during that event's final silent gap. It is an animation cue, not audio sensing or exact sound synchronization.

No definite new software fault was established in this review. Any future logic change should be separately reviewed and physically retested rather than substituted for the selected V14.1.

## Publication review

- Original source and a byte-identical backup stay outside the repository.
- The publication sketch removes only the personal name/student-number comment line. Non-comment program content, strings, pins, thresholds, timings, song data and AI attribution remain unchanged.
- No V14.2 refresh changes, new libraries or new hardware requirements were introduced.
- Documentation separates source behavior, student reports and newly verified compilation.
- Authentic photo/video placeholders are explicit; no synthetic hardware evidence is included.
- The repository file set is reviewed for identifiers, local account paths, contact information, credentials and unrelated material before the initial commit.
- All seven Markdown documents were rendered with a local GFM-capable parser and inspected in a browser. Internal file targets and code-fence pairing passed local checks. The four Mermaid blocks received a source syntax review; the external renderer could not be loaded during local preview because of network access, so diagram rendering and native GitHub presentation remain to be inspected after publication.
