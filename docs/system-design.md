# System Design — V14.1

The sketch is the behavioral reference. Diagrams describe source logic; they are not measured circuit schematics or evidence of physical testing.

## 1. Overall system

```mermaid
flowchart LR
    touch["TTP223 / D3"] --> controller["Arduino UNO R3 / V14.1"]
    thermistor["Thermistor circuit / A0"] --> controller
    remote["IR remote"] --> receiver["NEC receiver / A2"]
    receiver --> controller
    controller --> servo["SG90 head / D9"]
    controller --> buzzer["Passive buzzer / D8"]
    controller --> oled["SSD1306 OLED / A4 SDA, A5 SCL"]
    controller --> lcd["LCD1602 / D5, D6, D7, D10, D11, D12"]
    controller --> serial["Serial diagnostics / 9600 baud"]
```

Power distribution is deliberately unspecified; see [Wiring](wiring.md).

## 2. Operating modes

```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> Idle: Button 1 / resetPet
    Temperature --> Idle: Button 1 / resetPet
    Music --> Idle: Button 1 / resetPet
    Idle --> Temperature: Button 2 / reset then enter
    Temperature --> Temperature: Button 2 / reset then enter
    Music --> Temperature: Button 2 / reset then enter
    Idle --> Music: Button 3 / start from beginning
    Temperature --> Music: Button 3 / start from beginning
    Music --> Music: Button 3 / restart from beginning
    Music --> Idle: Final event duration elapsed / resetPet
    state Music {
        [*] --> Playing
        Playing --> Paused: Green button / pauseMusic
        Paused --> Playing: Green button / resumeMusic
    }
```

`musicPaused` is a flag within `MODE_MUSIC`, not a fourth `PetMode`. The green button has no action in idle or temperature mode. Temperature sampling and debounced touch-state tracking continue across modes; only idle accepts touch-triggered emotion actions. Reset changes software state and schedules head centering without restarting the UNO. It does not clear the temperature filter/history or reinitialize the peripherals.

## 3. Touch-to-emotion processing

```mermaid
flowchart TD
    raw["Read D3 as active HIGH"] --> changed{"Raw input changed?"}
    changed -->|Yes| stamp["Save raw level and change time"]
    changed -->|No| debounce{"Stable for at least 40 ms?"}
    stamp --> debounce
    debounce -->|No| done["Return to main loop"]
    debounce -->|Yes| edge{"Different from debounced level?"}
    edge -->|No| done
    edge -->|Yes| stable["Update touchStable"]
    stable --> idle{"HIGH transition and idle mode?"}
    idle -->|No| done
    idle -->|Yes| timeout{"Previous anger and 5 seconds elapsed?"}
    timeout -->|Yes| clear["Set angerLevel to 0"]
    timeout -->|No| increment["Increment angerLevel up to 3; save lastTouch"]
    clear --> increment
    increment --> actions["Start movement and emotion sound; displays follow state"]
    actions --> done
```

An additional idle-loop condition clears anger when touch is released and five seconds have elapsed since the last accepted touch. A held touch does not repeatedly increase anger and prevents the idle-loop calm transition. A new touch after the timeout restarts escalation at level 1. A new movement/sound request replaces the earlier sequence rather than queuing it.

| Emotion | Head targets in degrees | Emotion tones |
| --- | --- | --- |
| Calm (0) | 90 | No new emotion sound |
| Slightly angry (1) | 65 → 90 | 350 Hz |
| Angry (2) | 65 → 115 → 90 | 500, 400 Hz |
| Very angry (3) | 60 → 120 → 60 → 120 → 90 | 900, 700, 500 Hz |

Each tone lasts 130 ms, with successive tone starts scheduled 200 ms apart. Servo software position changes by one degree per eligible 18 ms update, is constrained to 60°–120°, and waits 150 ms at each target. Anger increases sequence range and repetition; it does not change the degree-step interval. Actual shaft position is not sensed.

## 4. Music and pause/resume

```mermaid
flowchart TD
    start["Button 3: music mode; clear indices and timing state"] --> service["Main loop services music when not paused"]
    service --> due{"First event or nextNoteTime reached?"}
    due -->|No| other["Continue servicing other hardware"]
    due -->|Yes| endSong{"noteIndex at SONG_COUNT?"}
    endSong -->|Yes| reset["resetPet; return to idle"]
    endSong -->|No| note["Read flash event; set frequency and lyric index"]
    note --> sound["Rest: noTone; note: tone for 90 percent of duration"]
    sound --> advance["Set toneEndTime and nextNoteTime; increment noteIndex"]
    advance --> other
    playing["Green button while playing"] --> pause["Save remaining event and tone times; noTone; set paused"]
    pause --> waiting["Other hardware and displays continue"]
    waiting --> resume["Green button: clear paused; rebuild deadlines"]
    resume --> remaining{"Non-rest note with tone time remaining?"}
    remaining -->|Yes| replay["Play same frequency for remaining tone time"]
    remaining -->|No| service
    replay --> service
```

`SongNote` stores frequency, beat count and lyric phrase index in flash. One beat is 400 ms. Each non-rest event sounds for 90% of its event duration, leaving a gap. `noteIndex` points to the next event after the current event starts. Lyrics change at phrase indices, not by word recognition. Pausing preserves both the event deadline and its sounding portion; a pause in a gap or rest resumes silently until the next event. If playback has not yet started, resume leaves the first event ready to begin.

## Cooperative scheduling

Each loop captures one `now = millis()` and calls remote, touch and servo services in that order. It samples temperature when 250 ms have elapsed, advances unpaused music or idle emotion sounds, checks the idle calm timeout, refreshes LCD state and schedules OLED rendering every 100 ms.

These are minimum scheduling intervals, not guaranteed precise periods. The loop does not catch up all missed servo steps or notes after a slow iteration. Eight ADC conversions, LCD writes, I2C page transfers and Serial output still occupy processor time. IR decoding uses TinyIRReceiver's interrupt-based reception; no additional full IR receiver or timer-based scheduler was introduced.

The original LCD code already caches the last mode, anger, lyric, temperature, hot state and pause flag. It rewrites padded rows when those values change. No V14.2 changes or per-character caching are included.
