# Code Explanation — V14.1

All application logic remains in [DesktopPet.ino](../DesktopPet/DesktopPet.ino). Publication removes one personal-identifier comment only. Existing section headings, names, constants, melody and logic are preserved.

## Important state variables

| Variable | Meaning and hardware effect |
| --- | --- |
| `mode` | `MODE_IDLE`, `MODE_TEMP` or `MODE_MUSIC`; selects LCD/OLED content and active sound behavior |
| `angerLevel` | 0 is calm; 1–3 select dialogue, face, movement targets and emotion sound sequence |
| `musicPaused` | Freezes music event advancement and selects paused LCD/OLED content; other services keep running |
| `noteIndex` | Index of the next song event to load; incremented after an event starts |
| `lyricIndex` | Current phrase index 0–6; 255 means no valid phrase yet |
| `musicStarted` | Distinguishes the first event from timed continuation |
| `currentFrequency` | Current event frequency; 0 (`REST_NOTE`) is silence and selects the rest-mouth graphic |
| `nextNoteTime` | Deadline for starting the next event or completing the song |
| `toneEndTime` | End of the current event's sounding portion, used to preserve pause state |
| `remainingNoteTime` | Saved time to the next event when paused |
| `remainingToneTime` | Saved sounding time; resume replays the current frequency only if this is positive |
| `servoPosition` | Last commanded software angle, not measured shaft feedback |
| `servoTarget` | Current angle from the predefined movement sequence |
| `movement`, `movementCount` | Target list and number of valid entries |
| `movementIndex` | Current target within that list |
| `moving`, `waitingAtTarget` | Select incremental travel, target dwell or inactivity |
| `filteredADC` | Smoothed thermistor reading; controls estimated temperature and hot-state switching |
| `estimatedTemp` | Empirical Celsius estimate, constrained to 0–60 |
| `displayedTemp`, `candidateTemp`, `candidateSince` | Numeric display value and candidate that must remain unchanged for 1500 ms |
| `hotState` | Enters hot at filtered ADC ≤910 and exits at ≥920; selects temperature face/text |
| `lastOLED` | Last scheduled OLED render time; 100 ms interval |
| `lastTemp` | Last scheduled temperature sample time; 250 ms interval in every mode |
| `touchLastRaw`, `touchStable`, `touchChangedAt` | Raw level, debounced level and last raw-change time |
| `lastTouch` | Time of last accepted idle rising edge; used for the 5000 ms anger timeout |
| `soundLevel`, `soundStep`, `soundActive`, `nextSoundTime` | Emotion tone sequence and its next scheduled start |
| `lcdScreen`, `lcdAnger`, `lcdLyric`, `lcdTemperature`, `lcdHot`, `lcdPaused` | Original state/value cache that suppresses unnecessary LCD row updates |

## Initialization and main loop

`setup()` starts Serial at 9600 baud, configures touch/ADC/buzzer pins, initializes the page-buffer SSD1306 and 16×2 LCD, attaches the servo and commands 90°. It seeds `filteredADC` from A0, initializes touch state to the current raw input, starts TinyIRReceiver and prints readiness diagnostics. A HIGH touch already present at startup is not treated as a new rising edge.

`loop()` services remote, touch and servo on each pass, then checks scheduled temperature work. Unpaused music gets `updateMusic()`; idle gets emotion sounds and the calm-timeout check. `refreshLCD()` runs every pass but writes only for relevant state changes. `drawOLED()` runs on a 100 ms schedule. Temperature mode has display behavior but no dedicated servo or sound reaction beyond the centering performed on entry.

Elapsed checks such as `now - lastTemp >= TEMP_INTERVAL` let hardware services return quickly while awaiting their next interval. Deadline checks use signed differences. This supports usual timer wraparound behavior for these short intervals; long-duration operation is still a physical test item.

## Mode functions

| Function | Responsibility |
| --- | --- |
| `resetPet()` | Stops tone; sets idle/calm; clears music and emotion-sound state, indices and saved pause times; schedules center movement. It does not reboot, detach the servo, reset touch debounce, clear temperature history or rerun `setup()` |
| `enterTemperatureMode()` | Calls `resetPet()`, then sets `MODE_TEMP` |
| `startMusic()` | Stops existing sound, selects music, clears anger/playback state and deadlines, schedules center movement and prepares the first event |
| `updateRemote(now)` | Decodes a frame; copies address, command and flags with interrupts briefly disabled; rejects parity failures, repeats, nonzero addresses and rapid identical commands; dispatches four supported codes |

Accepted remote commands are printed as `IR: 0x...`. A reported `IR ready: NO` should be investigated rather than treated as successful initialization.

## Touch, movement and emotion sounds

`updateTouch(now)` reads D3 as HIGH/LOW. A raw change resets the 40 ms debounce timer. When the level has remained stable long enough, a new HIGH transition in idle raises anger up to 3. If five seconds have already elapsed with existing anger, it first resets anger to zero. Each accepted event updates `lastTouch` and calls `startMovement(angerLevel)` and `startEmotionSound(angerLevel)`.

`startMovement(type)` replaces the current movement, resets its index and chooses the unchanged target list:

| Type | Targets (degrees) |
| --- | --- |
| 0 | 90 |
| 1 | 65, 90 |
| 2 | 65, 115, 90 |
| 3 | 60, 120, 60, 120, 90 |

`updateServo(now)` changes the commanded angle one degree toward `servoTarget` when at least 18 ms have elapsed. It constrains the result to 60–120 and calls `headServo.write()`. At each target it waits 150 ms, advances `movementIndex` and eventually clears `moving`. A new touch can interrupt and replace an unfinished sequence, beginning from the existing commanded position.

`startEmotionSound(level)` cancels the current tone and resets a small sound sequencer. `updateEmotionSound(now)` schedules 130 ms tones, with 200 ms between starts: level 1 uses 350 Hz; level 2 uses 500/400 Hz; level 3 uses 900/700/500 Hz. The sequencer is serviced only in idle.

## Temperature estimate

`updateTemperature(now)` averages eight A0 readings and applies:

```cpp
filteredADC = filteredADC * 0.85 + adc * 0.15;
estimatedTemp = 25.0 + (932.0 - filteredADC) * 7.0 / 32.0;
estimatedTemp = constrain(estimatedTemp, 0.0, 60.0);
```

This is an empirical relationship, not a calibrated thermistor resistance equation. A lower ADC value increases the estimate. No thermistor model, divider resistance, reference measurements or uncertainty budget was supplied.

Hot-state hysteresis is independent of the numeric display confirmation: enter hot at ≤910, remain hot until ≥920. The rounded estimate must stay at the same candidate integer for 1500 ms before replacing `displayedTemp`. Thus the face/hot text can change before the displayed integer does. Sampling continues in idle and music; entering temperature mode retains the accumulated filtered reading.

## Music functions and flash data

`song[]`, `lyricTop` and `lyricBottom` use `PROGMEM`; `memcpy_P()` and `strcpy_P()` copy the needed item to RAM. The flash helpers come from the AVR toolchain. `SongNote` holds `frequency`, `beats` and `lyric`.

`updateMusic(now)` waits until the next event is due. It resets the pet when the event list is exhausted; otherwise it reads the next event, selects its phrase, computes `beats * 400 ms`, sounds non-rest events for 90% of that duration and advances `noteIndex`. Rests call `noTone()`. The mouth graphic follows the event's `currentFrequency`, rather than sensing actual sound or the note's final silent gap.

`pauseMusic(now)` sets the paused flag, saves positive time remaining until both deadlines and silences the buzzer. `resumeMusic(now)` reconstructs deadlines relative to resume time and sounds the current non-rest frequency for its remaining tone time. When paused in a rest or the end gap of a note, it resumes silently until the next event. The LCD shows the current phrase's top line and `PAUSED` below; resuming restores the phrase pair.

The melody is a simplified single-voice educational implementation labeled *You Are My Sunshine*. There are no vocals, polyphony or claims of an exact original arrangement. The existing song data and tempo are unchanged.

## LCD functions

`printLCDLine(row, str)` writes at most 16 characters, then pads remaining positions with spaces. `printFlashLyric()` copies a valid phrase row into a 17-byte buffer. `printTemperatureLCD()` prints `EST TEMP: ...C` and either `NICE WEATHER` or `TOO HOT!`.

`refreshLCD()` compares the active mode and associated state with the original cache variables. Idle updates on anger changes; temperature updates on displayed temperature or hot-state changes; music updates on phrase/pause changes. A music-mode transition can first write the title and then the valid phrase in the same call. Entire affected rows are written character by character, so visible text transitions can remain. No new character-level cache or V14.2 refresh change has been added.

| Idle level | LCD row 1 | LCD row 2 |
| --- | --- | --- |
| 0 | HELLO HUMAN | I AM YOUR PET |
| 1 | DON'T TOUCH ME! | GO AWAY! |
| 2 | I SAID NO! | STOP TOUCHING! |
| 3 | STOP IT!!! | I AM ANGRY!!! |

## OLED functions

`drawOLED(now)` uses `firstPage()` / `nextPage()` with the existing one-page-buffer SSD1306 constructor and dispatches by mode. Drawing operations run for each page.

| Function/state | Implemented graphics |
| --- | --- |
| `drawNormalFace()` | Disc eyes, smile and blink during the first 180 ms of each 3000 ms cycle |
| `drawAngryFace()`, level 0 | Normal face and `HELLO!` |
| Level 1 | Sloping brow lines, narrow eyes, straight mouth, `GO AWAY!` |
| Level 2 | Angled brows, smaller disc eyes, angular mouth, `NO!` |
| Level 3 | Face features shift ±2 pixels based on 120 ms phases; `STOP IT!` |
| `drawTemperatureFace()` | Normal face with `NICE!`, or hot face, circular mouth and animated drop with `TOO HOT!` |
| `drawMusicFace()`, playing | Moving pixels, happy eyes, changing heart radius and animated mouth with `SINGING...` |
| Music paused | Disc eyes and pause bars with `PAUSED` |
| `centeredText()` | Uses `u8g2_font_6x10_tr`, measures string width and centers the caption |

These are source-derived graphics descriptions. Physical display appearance and actual servo motion require the checklist in [Testing](testing.md).
