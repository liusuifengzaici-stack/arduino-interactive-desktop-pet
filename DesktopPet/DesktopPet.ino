
#include <Arduino.h>
#include <Wire.h>
#include <Servo.h>
#include <U8g2lib.h>
#include <LiquidCrystal.h>
#include <avr/pgmspace.h>
#include <string.h>

// ==================================================
// AI-assisted generation
// ==================================================
// DESKTOP PET V14.1
// Arduino UNO R3
// Sunshine Music + Touch + Servo + Temperature
// ==================================================

// ---------------- IR RECEIVER ----------------

#define IR_RECEIVE_PIN A2
#define NO_LED_FEEDBACK_CODE
#include <TinyIRReceiver.hpp>

// ---------------- HARDWARE ----------------

U8G2_SSD1306_128X64_NONAME_1_HW_I2C oled(
  U8G2_R0, U8X8_PIN_NONE
);

LiquidCrystal lcd(5, 6, 7, 10, 11, 12);
Servo headServo;

const byte PIN_TOUCH = 3;
const byte PIN_BUZZER = 8;
const byte PIN_SERVO = 9;
const byte PIN_TEMP = A0;

// ---------------- REMOTE CODES ----------------

const byte KEY_RESET = 0x0C;
const byte KEY_TEMP  = 0x18;
const byte KEY_SONG  = 0x5E;
const byte KEY_PAUSE = 0x43;

// ---------------- SYSTEM MODES ----------------

enum PetMode {
  MODE_IDLE,
  MODE_TEMP,
  MODE_MUSIC
};

PetMode mode = MODE_IDLE;

byte angerLevel = 0;
bool musicPaused = false;

// ---------------- TIMING ----------------

const unsigned long OLED_INTERVAL = 100;
const unsigned long TEMP_INTERVAL = 250;
const unsigned long TOUCH_DEBOUNCE = 40;
const unsigned long ANGER_TIMEOUT = 5000;

unsigned long lastOLED = 0;
unsigned long lastTemp = 0;
unsigned long lastTouch = 0;

// ==================================================
// SONG: YOU ARE MY SUNSHINE
// Simplified melody based on supplied score
// Key: C major, 4/4
// ==================================================

// NOTE_ prefix prevents conflicts with Arduino A5

const uint16_t REST_NOTE = 0;
const uint16_t NOTE_G4  = 392;
const uint16_t NOTE_C5  = 523;
const uint16_t NOTE_D5  = 587;
const uint16_t NOTE_DS5 = 622;
const uint16_t NOTE_E5  = 659;
const uint16_t NOTE_F5  = 698;
const uint16_t NOTE_G5  = 784;
const uint16_t NOTE_A5  = 880;

// One beat = 400 ms
const unsigned int BEAT_MS = 400;

// Lyrics displayed on LCD1602

const char lyricTop[][17] PROGMEM = {
  "YOU ARE MY",
  "MY ONLY",
  "YOU MAKE ME",
  "WHEN SKIES ARE",
  "YOU'LL NEVER",
  "HOW MUCH I",
  "PLEASE DON'T TAKE"
};

const char lyricBottom[][17] PROGMEM = {
  "SUNSHINE",
  "SUNSHINE",
  "HAPPY",
  "GRAY",
  "KNOW DEAR",
  "LOVE YOU",
  "MY SUNSHINE AWAY"
};

const byte LYRIC_COUNT = 7;

// Musical event
struct SongNote {
  uint16_t frequency;
  byte beats;
  byte lyric;
};

// Main melody
const SongNote song[] PROGMEM = {

  // Phrase 1
  {REST_NOTE,1,0},
  {NOTE_G4,1,0},
  {NOTE_C5,1,0},
  {NOTE_D5,1,0},
  {NOTE_E5,2,0},
  {NOTE_E5,2,0},

  // Phrase 2
  {REST_NOTE,1,1},
  {NOTE_E5,1,1},
  {NOTE_DS5,1,1},
  {NOTE_E5,1,1},
  {NOTE_C5,2,1},
  {NOTE_C5,2,1},

  // Phrase 3
  {REST_NOTE,1,2},
  {NOTE_C5,1,2},
  {NOTE_D5,1,2},
  {NOTE_E5,1,2},
  {NOTE_F5,2,2},
  {NOTE_A5,2,2},

  // Phrase 4
  {REST_NOTE,1,3},
  {NOTE_A5,1,3},
  {NOTE_G5,1,3},
  {NOTE_F5,1,3},
  {NOTE_E5,2,3},
  {NOTE_C5,2,3},

  // Phrase 5
  {REST_NOTE,1,4},
  {NOTE_C5,1,4},
  {NOTE_D5,1,4},
  {NOTE_E5,1,4},

  // Phrase 6
  {NOTE_E5,3,5},
  {NOTE_F5,1,5},
  {NOTE_D5,2,5},
  {NOTE_D5,1,5},
  {NOTE_E5,1,5},

  // Phrase 7
  {NOTE_C5,3,6},
  {REST_NOTE,1,6}
};

const byte SONG_COUNT =
  sizeof(song) / sizeof(song[0]);

// ---------------- MUSIC STATE ----------------

byte noteIndex = 0;
byte lyricIndex = 255;

bool musicStarted = false;

uint16_t currentFrequency = 0;

unsigned long nextNoteTime = 0;
unsigned long toneEndTime = 0;

unsigned long remainingNoteTime = 0;
unsigned long remainingToneTime = 0;

// ==================================================
// TOUCH AND EMOTION
// ==================================================

bool touchLastRaw = false;
bool touchStable = false;

unsigned long touchChangedAt = 0;

byte soundLevel = 0;
byte soundStep = 0;

bool soundActive = false;
unsigned long nextSoundTime = 0;

// ==================================================
// SERVO CONTROL
// ==================================================

const byte SERVO_CENTER = 90;
const byte SERVO_MIN = 60;
const byte SERVO_MAX = 120;

const unsigned long SERVO_STEP_MS = 18;
const unsigned long SERVO_WAIT_MS = 150;

int servoPosition = 90;
int servoTarget = 90;

byte movement[6];
byte movementCount = 0;
byte movementIndex = 0;

bool moving = false;
bool waitingAtTarget = false;

unsigned long lastServoUpdate = 0;
unsigned long servoWaitUntil = 0;

// ==================================================
// TEMPERATURE SENSOR
// Demonstration estimate, not calibrated
// ==================================================

float filteredADC = 932.0;
float estimatedTemp = 25.0;

int displayedTemp = 25;
int candidateTemp = 25;

bool hotState = false;

unsigned long candidateSince = 0;

const unsigned long TEMP_CONFIRM_MS = 1500;

// ==================================================
// LCD DISPLAY CACHE
// ==================================================

byte lcdScreen = 255;
byte lcdAnger = 255;
byte lcdLyric = 255;

int lcdTemperature = -100;

bool lcdHot = false;
bool lcdPaused = false;

// ==================================================
// SETUP
// ==================================================

void setup() {

  Serial.begin(9600);

  pinMode(PIN_TOUCH, INPUT);
  pinMode(PIN_TEMP, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  noTone(PIN_BUZZER);

  // Initialize OLED and LCD
  oled.begin();
  lcd.begin(16, 2);

  // Initialize servo
  headServo.attach(PIN_SERVO);
  headServo.write(SERVO_CENTER);

  servoPosition = SERVO_CENTER;
  servoTarget = SERVO_CENTER;

  // Initialize temperature
  filteredADC = analogRead(PIN_TEMP);
  candidateSince = millis();

  // Initialize touch
  touchLastRaw =
    digitalRead(PIN_TOUCH) == HIGH;

  touchStable = touchLastRaw;
  touchChangedAt = millis();

  // Initialize IR receiver
  bool irReady =
    initPCIInterruptForTinyIRReceiver();

  Serial.print(F("IR ready: "));
  Serial.println(
    irReady ? F("YES") : F("NO")
  );

  refreshLCD();

  Serial.println(F("DesktopPet V14.1 READY"));
}

// ==================================================
// MAIN LOOP
// Non-blocking system scheduling
// ==================================================

void loop() {

  unsigned long now = millis();

  updateRemote(now);
  updateTouch(now);
  updateServo(now);

  // Temperature sampling
  if (now - lastTemp >= TEMP_INTERVAL) {
    lastTemp = now;
    updateTemperature(now);
  }

  // Mode management
  if (mode == MODE_MUSIC) {

    if (!musicPaused) {
      updateMusic(now);
    }

  } else if (mode == MODE_IDLE) {

    updateEmotionSound(now);

    // Automatically return to calm state
    if (angerLevel > 0 &&
        !touchStable &&
        now - lastTouch >= ANGER_TIMEOUT) {

      angerLevel = 0;
      soundActive = false;
      startMovement(0);
    }
  }

  refreshLCD();

  // OLED animation
  if (now - lastOLED >= OLED_INTERVAL) {

    lastOLED = now;
    drawOLED(now);
  }
}

// ==================================================
// SYSTEM MODE MANAGEMENT
// ==================================================

void resetPet() {

  noTone(PIN_BUZZER);

  mode = MODE_IDLE;
  angerLevel = 0;

  musicPaused = false;
  musicStarted = false;
  soundActive = false;

  currentFrequency = 0;

  noteIndex = 0;
  lyricIndex = 255;

  remainingNoteTime = 0;
  remainingToneTime = 0;

  startMovement(0);

  Serial.println(F("Reset to IDLE"));
}

void enterTemperatureMode() {

  resetPet();

  mode = MODE_TEMP;

  Serial.println(F("Temperature mode"));
}

void startMusic() {

  noTone(PIN_BUZZER);

  mode = MODE_MUSIC;

  angerLevel = 0;
  soundActive = false;

  musicPaused = false;
  musicStarted = false;

  noteIndex = 0;
  lyricIndex = 255;

  currentFrequency = 0;

  nextNoteTime = 0;
  toneEndTime = 0;

  remainingNoteTime = 0;
  remainingToneTime = 0;

  startMovement(0);

  Serial.println(F("Playing Sunshine"));
}

// ==================================================
// INFRARED REMOTE CONTROL
// ==================================================

void updateRemote(unsigned long now) {

  if (!TinyIRReceiverDecode()) {
    return;
  }

  noInterrupts();

  uint16_t address =
    TinyIRReceiverData.Address;

  byte command =
    TinyIRReceiverData.Command;

  byte flags =
    TinyIRReceiverData.Flags;

  interrupts();

  if (flags & IRDATA_FLAGS_PARITY_FAILED)
    return;

  if (flags & IRDATA_FLAGS_IS_REPEAT)
    return;

  if (address != 0x00)
    return;

  // Ignore rapid duplicate commands
  static byte lastCommand = 0;
  static unsigned long lastCommandTime = 0;
  static bool receivedBefore = false;

  if (receivedBefore &&
      command == lastCommand &&
      now - lastCommandTime < 160) {
    return;
  }

  receivedBefore = true;
  lastCommand = command;
  lastCommandTime = now;

  Serial.print(F("IR: 0x"));
  Serial.println(command, HEX);

  switch (command) {

    case KEY_RESET:
      resetPet();
      break;

    case KEY_TEMP:
      enterTemperatureMode();
      break;

    case KEY_SONG:
      startMusic();
      break;

    case KEY_PAUSE:

      if (mode == MODE_MUSIC) {

        if (musicPaused) {
          resumeMusic(now);
        } else {
          pauseMusic(now);
        }
      }

      break;

    default:
      break;
  }
}

// ==================================================
// MUSIC PLAYBACK
// ==================================================

void pauseMusic(unsigned long now) {

  if (mode != MODE_MUSIC || musicPaused)
    return;

  musicPaused = true;

  if (musicStarted) {

    if ((long)(nextNoteTime - now) > 0) {
      remainingNoteTime = nextNoteTime - now;
    } else {
      remainingNoteTime = 0;
    }

    if ((long)(toneEndTime - now) > 0) {
      remainingToneTime = toneEndTime - now;
    } else {
      remainingToneTime = 0;
    }
  }

  noTone(PIN_BUZZER);

  Serial.println(F("Music paused"));
}

void resumeMusic(unsigned long now) {

  if (mode != MODE_MUSIC || !musicPaused)
    return;

  musicPaused = false;

  if (musicStarted) {

    nextNoteTime =
      now + remainingNoteTime;

    toneEndTime =
      now + remainingToneTime;

    if (currentFrequency != REST_NOTE &&
        remainingToneTime > 0) {

      tone(
        PIN_BUZZER,
        currentFrequency,
        remainingToneTime
      );
    }

  } else {

    nextNoteTime = 0;
  }

  Serial.println(F("Music resumed"));
}

void updateMusic(unsigned long now) {

  // Wait until the next musical event
  if (musicStarted &&
      (long)(now - nextNoteTime) < 0) {
    return;
  }

  // End of music
  if (noteIndex >= SONG_COUNT) {

    resetPet();

    Serial.println(F("Song finished"));
    return;
  }

  // Read the next note from flash memory
  SongNote note;

  memcpy_P(
    &note,
    &song[noteIndex],
    sizeof(SongNote)
  );

  currentFrequency = note.frequency;
  lyricIndex = note.lyric;

  unsigned long duration =
    (unsigned long)note.beats * BEAT_MS;

  unsigned long soundingTime =
    duration * 90UL / 100;

  if (currentFrequency == REST_NOTE) {

    noTone(PIN_BUZZER);
    toneEndTime = now;

  } else {

    tone(
      PIN_BUZZER,
      currentFrequency,
      soundingTime
    );

    toneEndTime =
      now + soundingTime;
  }

  nextNoteTime = now + duration;

  musicStarted = true;
  noteIndex++;
}

// ==================================================
// TOUCH SENSOR
// ==================================================

void updateTouch(unsigned long now) {

  bool raw =
    digitalRead(PIN_TOUCH) == HIGH;

  if (raw != touchLastRaw) {

    touchLastRaw = raw;
    touchChangedAt = now;
  }

  if (now - touchChangedAt <
      TOUCH_DEBOUNCE) {
    return;
  }

  if (raw != touchStable) {

    touchStable = raw;

    if (touchStable && mode == MODE_IDLE) {

      if (angerLevel > 0 &&
          now - lastTouch >= ANGER_TIMEOUT) {
        angerLevel = 0;
      }

      if (angerLevel < 3) {
        angerLevel++;
      }

      lastTouch = now;

      startMovement(angerLevel);
      startEmotionSound(angerLevel);

      Serial.print(F("Anger level: "));
      Serial.println(angerLevel);
    }
  }
}

// ==================================================
// SERVO MOVEMENT
// 0 = Center
// 1 = Slight turn
// 2 = Single shake
// 3 = Strong shake
// ==================================================

void startMovement(byte type) {

  moving = false;
  waitingAtTarget = false;

  movementIndex = 0;
  movementCount = 0;

  switch (type) {

    case 0:
      movement[0] = 90;
      movementCount = 1;
      break;

    case 1:
      movement[0] = 65;
      movement[1] = 90;
      movementCount = 2;
      break;

    case 2:
      movement[0] = 65;
      movement[1] = 115;
      movement[2] = 90;
      movementCount = 3;
      break;

    case 3:
      movement[0] = 60;
      movement[1] = 120;
      movement[2] = 60;
      movement[3] = 120;
      movement[4] = 90;
      movementCount = 5;
      break;
  }

  if (movementCount > 0) {

    servoTarget = movement[0];
    moving = true;
  }
}

void updateServo(unsigned long now) {

  if (!moving) return;

  // Wait between positions
  if (waitingAtTarget) {

    if ((long)(now - servoWaitUntil) < 0)
      return;

    waitingAtTarget = false;
    movementIndex++;

    if (movementIndex >= movementCount) {

      moving = false;
      return;
    }

    servoTarget =
      movement[movementIndex];
  }

  // Gradually move to target
  if (now - lastServoUpdate >= SERVO_STEP_MS) {

    lastServoUpdate = now;

    if (servoPosition < servoTarget) {
      servoPosition++;
    }

    else if (servoPosition > servoTarget) {
      servoPosition--;
    }

    servoPosition = constrain(
      servoPosition,
      SERVO_MIN,
      SERVO_MAX
    );

    headServo.write(servoPosition);
  }

  if (servoPosition == servoTarget &&
      !waitingAtTarget) {

    waitingAtTarget = true;

    servoWaitUntil =
      now + SERVO_WAIT_MS;
  }
}

// ==================================================
// EMOTION SOUND EFFECTS
// ==================================================

void startEmotionSound(byte level) {

  noTone(PIN_BUZZER);

  soundLevel = level;
  soundStep = 0;

  nextSoundTime = 0;
  soundActive = true;
}

void updateEmotionSound(unsigned long now) {

  if (!soundActive) return;

  if (nextSoundTime != 0 &&
      (long)(now - nextSoundTime) < 0) {
    return;
  }

  if (soundStep >= soundLevel) {

    noTone(PIN_BUZZER);
    soundActive = false;
    return;
  }

  uint16_t frequency = 350;

  if (soundLevel == 2) {

    frequency =
      soundStep == 0 ? 500 : 400;

  } else if (soundLevel == 3) {

    frequency =
      900 - 200 * soundStep;
  }

  tone(
    PIN_BUZZER,
    frequency,
    130
  );

  soundStep++;

  nextSoundTime =
    now + 200;
}

// ==================================================
// TEMPERATURE SENSOR
// Original V12 estimation
// ==================================================

void updateTemperature(unsigned long now) {

  long total = 0;

  for (byte i = 0; i < 8; i++) {
    total += analogRead(PIN_TEMP);
  }

  float adc = total / 8.0;

  filteredADC =
    filteredADC * 0.85 +
    adc * 0.15;

  estimatedTemp =
    25.0 +
    (932.0 - filteredADC) * 7.0 / 32.0;

  estimatedTemp =
    constrain(estimatedTemp, 0.0, 60.0);

  if (!hotState && filteredADC <= 910) {

    hotState = true;

  } else if (hotState && filteredADC >= 920) {

    hotState = false;
  }

  int rounded =
    (int)(estimatedTemp + 0.5);

  if (rounded != candidateTemp) {

    candidateTemp = rounded;
    candidateSince = now;
  }

  if (candidateTemp != displayedTemp &&
      now - candidateSince >= TEMP_CONFIRM_MS) {

    displayedTemp = candidateTemp;
  }
}

// ==================================================
// LCD1602 DISPLAY
// ==================================================

void printLCDLine(byte row, const char* str) {

  lcd.setCursor(0, row);

  byte i = 0;

  while (str[i] != '\0' && i < 16) {

    lcd.print(str[i]);
    i++;
  }

  while (i < 16) {

    lcd.print(' ');
    i++;
  }
}

// Read lyrics from flash
void printFlashLyric(
  byte row,
  byte index,
  bool topLine
) {

  char buffer[17];

  if (index >= LYRIC_COUNT)
    return;

  if (topLine) {

    strcpy_P(
      buffer,
      lyricTop[index]
    );

  } else {

    strcpy_P(
      buffer,
      lyricBottom[index]
    );
  }

  printLCDLine(row, buffer);
}

void printTemperatureLCD() {

  char buffer[17];

  snprintf(
    buffer,
    sizeof(buffer),
    "EST TEMP: %dC",
    displayedTemp
  );

  printLCDLine(0, buffer);

  if (hotState) {
    printLCDLine(1, "TOO HOT!");
  } else {
    printLCDLine(1, "NICE WEATHER");
  }
}

// Update only when needed
void refreshLCD() {

  byte screen = (byte)mode;

  bool screenChanged =
    lcdScreen != screen;

  // ---------------- MUSIC ----------------

  if (mode == MODE_MUSIC) {

    if (screenChanged) {

      printLCDLine(0, "YOU ARE MY");
      printLCDLine(1, "SUNSHINE");

      lcdLyric = 255;
      lcdPaused = false;
    }

    if (musicPaused) {

      if (screenChanged || !lcdPaused) {

        if (lyricIndex < LYRIC_COUNT) {
          printFlashLyric(
            0, lyricIndex, true
          );
        } else {
          printLCDLine(0, "YOU ARE MY");
        }

        printLCDLine(1, "PAUSED");
      }

    } else if (
      lyricIndex < LYRIC_COUNT &&
      (screenChanged ||
       lcdLyric != lyricIndex ||
       lcdPaused)
    ) {

      printFlashLyric(
        0, lyricIndex, true
      );

      printFlashLyric(
        1, lyricIndex, false
      );
    }

    lcdLyric = lyricIndex;
    lcdPaused = musicPaused;
  }

  // ---------------- TEMPERATURE ----------------

  else if (mode == MODE_TEMP) {

    if (screenChanged ||
        lcdTemperature != displayedTemp ||
        lcdHot != hotState) {

      printTemperatureLCD();

      lcdTemperature = displayedTemp;
      lcdHot = hotState;
    }
  }

  // ---------------- IDLE / EMOTION ----------------

  else {

    if (screenChanged ||
        lcdAnger != angerLevel) {

      switch (angerLevel) {

        case 0:

          printLCDLine(
            0, "HELLO HUMAN"
          );

          printLCDLine(
            1, "I AM YOUR PET"
          );
          break;

        case 1:

          printLCDLine(
            0, "DON'T TOUCH ME!"
          );

          printLCDLine(
            1, "GO AWAY!"
          );
          break;

        case 2:

          printLCDLine(
            0, "I SAID NO!"
          );

          printLCDLine(
            1, "STOP TOUCHING!"
          );
          break;

        default:

          printLCDLine(
            0, "STOP IT!!!"
          );

          printLCDLine(
            1, "I AM ANGRY!!!"
          );
          break;
      }

      lcdAnger = angerLevel;
    }
  }

  lcdScreen = screen;
}

// ==================================================
// OLED GRAPHICS
// ==================================================

void centeredText(const char* str) {

  oled.setFont(u8g2_font_6x10_tr);

  int width =
    oled.getStrWidth(str);

  oled.drawStr(
    (128 - width) / 2,
    62,
    str
  );
}

// Normal expression
void drawNormalFace(unsigned long now) {

  if (now % 3000 < 180) {

    oled.drawHLine(29, 28, 22);
    oled.drawHLine(77, 28, 22);

  } else {

    oled.drawDisc(40, 28, 8);
    oled.drawDisc(88, 28, 8);
  }

  oled.drawLine(49,43,55,48);
  oled.drawLine(55,48,64,51);
  oled.drawLine(64,51,73,48);
  oled.drawLine(73,48,79,43);
}

// Angry expressions
void drawAngryFace(unsigned long now) {

  if (angerLevel == 0) {

    drawNormalFace(now);
    centeredText("HELLO!");
    return;
  }

  if (angerLevel == 1) {

    oled.drawLine(22,23,52,27);
    oled.drawLine(76,27,106,23);

    oled.drawHLine(29,35,23);
    oled.drawHLine(76,35,23);

    oled.drawLine(54,48,73,48);

    centeredText("GO AWAY!");
  }

  else if (angerLevel == 2) {

    oled.drawLine(24,18,52,30);
    oled.drawLine(76,30,104,18);

    oled.drawDisc(41,35,5);
    oled.drawDisc(87,35,5);

    oled.drawLine(53,51,64,45);
    oled.drawLine(64,45,75,51);

    centeredText("NO!");
  }

  else {

    int shake =
      (now / 120 % 2) ? 2 : -2;

    oled.drawLine(
      23+shake,17,
      53+shake,31
    );

    oled.drawLine(
      75+shake,31,
      105+shake,17
    );

    oled.drawDisc(42+shake,36,6);
    oled.drawDisc(86+shake,36,6);

    oled.drawLine(52,49,76,49);

    centeredText("STOP IT!");
  }
}

// Temperature expression
void drawTemperatureFace(unsigned long now) {

  if (!hotState) {

    drawNormalFace(now);
    centeredText("NICE!");
    return;
  }

  oled.drawLine(25,26,52,29);
  oled.drawLine(76,29,103,26);

  oled.drawDisc(40,35,4);
  oled.drawDisc(88,35,4);

  oled.drawCircle(64,46,7);

  int y =
    12 + (now / 220) % 15;

  oled.drawDisc(109,y+5,3);

  oled.drawTriangle(
    109,y-2,
    106,y+5,
    112,y+5
  );

  centeredText("TOO HOT!");
}

// Music animation
void drawMusicFace(unsigned long now) {

  if (musicPaused) {

    oled.drawDisc(40,28,6);
    oled.drawDisc(88,28,6);

    oled.drawBox(54,40,6,13);
    oled.drawBox(68,40,6,13);

    centeredText("PAUSED");
    return;
  }

  // Stars animation
  for (byte i = 0; i < 18; i++) {

    int x =
      (i * 29 + 11) % 128;

    int y =
      (now / (100 + i * 9) +
       i * 17) % 64;

    oled.drawPixel(x,y);
  }

  // Happy eyes
  oled.drawLine(25,30,38,23);
  oled.drawLine(38,23,51,30);

  oled.drawLine(77,30,90,23);
  oled.drawLine(90,23,103,30);

  // Animated heart
  byte radius =
    (now / 350 % 2) ? 5 : 4;

  oled.drawDisc(59,12,radius);
  oled.drawDisc(69,12,radius);

  oled.drawTriangle(
    55,14,
    73,14,
    64,26
  );

  // Mouth animation
  if (currentFrequency == REST_NOTE) {

    oled.drawHLine(58,43,12);

  } else if ((now / 280) % 2) {

    oled.drawCircle(64,43,5);

  } else {

    oled.drawHLine(57,43,14);
  }

  centeredText("SINGING...");
}

// OLED rendering
void drawOLED(unsigned long now) {

  oled.firstPage();

  do {

    if (mode == MODE_MUSIC) {

      drawMusicFace(now);

    } else if (mode == MODE_TEMP) {

      drawTemperatureFace(now);

    } else {

      drawAngryFace(now);
    }

  } while (oled.nextPage());
}
