# Wiring and Hardware

## Confirmed signal mapping

These assignments are confirmed by the V14.1 sketch and the student's component list. They do not establish the actual supply wiring or component ratings.

| UNO pin | Component terminal/signal | Source evidence |
| --- | --- | --- |
| D3 | TTP223 OUT | `PIN_TOUCH = 3`, `INPUT`, active HIGH |
| A0 | Thermistor circuit analog output | `PIN_TEMP = A0`, `analogRead()` |
| A2 | NEC IR receiver output | `IR_RECEIVE_PIN A2` |
| D8 | Passive buzzer signal | `PIN_BUZZER = 8`, `tone()` / `noTone()` |
| D9 | SG90 servo signal | `PIN_SERVO = 9`, `headServo.attach()` |
| A4 | SSD1306 OLED SDA | UNO hardware I2C and `_HW_I2C` constructor |
| A5 | SSD1306 OLED SCL | UNO hardware I2C and `_HW_I2C` constructor |
| D5 | LCD RS | First `LiquidCrystal` constructor argument |
| D6 | LCD E | Second argument |
| D7 | LCD D4 | Third argument |
| D10 | LCD D5 | Fourth argument |
| D11 | LCD D6 | Fifth argument |
| D12 | LCD D7 | Sixth argument |

## Component roles

The UNO runs the state machine. The TTP223 reports touch as a digital level. The thermistor circuit produces an analog signal; its topology has not been supplied. The IR receiver receives the remote's NEC frames. The SG90 moves the head through predefined software targets. The passive buzzer plays tones. The OLED draws 128×64 graphics over I2C; the LCD1602 displays two rows of text over a four-bit parallel interface. The remote supplies the four documented commands.

Vibration sensing and physical buttons are absent from the final configuration.

## Electrical details requiring physical verification

| Detail | What must be checked on the actual prototype |
| --- | --- |
| Module VCC/GND | Supply voltage, polarity, connector order and logic-level compatibility for each exact module |
| Thermistor | Divider topology, fixed resistance, thermistor specification and resistance curve; no resistor value is inferred from the empirical formula |
| OLED | Actual address, pull-ups and supply/logic requirements; no explicit address override exists in the sketch |
| Servo supply | Actual supply source and ability to handle movement/stall current; no voltage/current rating is established here |
| LCD | VSS/VDD, R/W connection, contrast potentiometer wiring/value, backlight current limiting and module rating |
| Buzzer | Actual drive circuit and permissible pin load |

## Shared ground and servo power

The controller and connected signal devices need a common signal reference. If the servo uses a separate suitable supply, connect its ground to the UNO signal ground. Verify the existing physical circuit before applying power; the table above does not prove how grounds are currently connected.

Do not power a servo through a GPIO pin. Select and verify its supply using the exact hardware specifications and expected load, and investigate voltage drop, connection quality and ground routing if motion is erratic. Avoid joining positive supply outputs without understanding the power arrangement. No specific capacitor, resistor or supply rating is prescribed because these details were not confirmed.

Keep the head mechanically clear throughout the existing 60°–120° software range. Software limits do not establish the actual safe mechanical travel of the assembled head. Stop power if it binds or overheats.

## LCD1602 contrast

The student reported that a potentiometer adjustment was needed before text became visible. Verify its actual connection to the LCD contrast input and the module's required wiring. Adjust contrast gently after checking power and signal connections. Blank text may result from contrast, power, wiring or initialization; the source alone cannot identify the cause.

## Connection practice

Disconnect power before rewiring. Verify pin labels on the exact modules, secure loose breadboard leads and inspect for short circuits. Test subsystems systematically before exercising all modules together. Record a real wiring photograph and measured supply observations in the testing record when available. The prior vibration-sensor failure is a reported symptom, not proof of a particular electrical fault.
