// Prototypes which might have been auto-generated but are not.
void updateShiftRegister();
void MYshiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint32_t val);

int32_t shiftRegBits = 0;  // Size for up to 32 relays, though at first we use no more than 16.

void RelaysInit() {
  //-------( Initialize Pins so all relays are inactive at reset)----
  // Shift Register
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  digitalWrite(LATCH_PIN, HIGH);
  shiftRegBits = 0;
  updateShiftRegister();
}

/**
 * Relay Tests.  Writes 8 lines, numbered 1-8.
 * Each heater and chiller relay is turned on, then off.
 */
void relayTest() {
  tft.fillScreen(BLACK);
  int lineY = 0;
  int shiftUp = 0;  // Start writing at the top if we'd otherwise be off the bottom.
  for (i = 0; i < NT; i++) {
    // If > 4 tanks, write LCD output over the earlier ones
    if (i >= 4) shiftUp = 2 * 4 * LINEHEIGHT3;
    lineY = 2 * i * LINEHEIGHT3 - shiftUp;
    if (shiftUp > 0) {
      tft.fillRect(0, lineY, TFT_WIDTH, LINEHEIGHT3, BLACK);
    }
    tft.setCursor(0, lineY);

    // Test one heater relay
    tft.printf("%d. T%dHeatr", 2*i+1, i+1);
    Serial.printf("%d. T%dHeatr\n", 2*i+1, i+1);
    setHeatRelay(i, RELAY_ON);
    delay(RELAY_PAUSE);
    setHeatRelay(i, RELAY_OFF);

    delay(100);  // A brief delay so you can hear the "off" relay action separately from the following "on"
    // Test one chiller relay
    lineY = (2 * i + 1) * LINEHEIGHT3 - shiftUp;
    if (shiftUp > 0) {
      tft.fillRect(0, lineY, TFT_WIDTH, LINEHEIGHT3, BLACK);
    }
    tft.setCursor(0, lineY);

    tft.printf("%d. T%dChillr", 2*i+2, i+1);
    Serial.printf("%d. T%dChillr\n", 2*i+2, i+1);
    setChillRelay(i, RELAY_ON);

    delay(RELAY_PAUSE);
    setChillRelay(i, RELAY_OFF);

    delay(100);  // A brief delay so you can hear the "off" relay action separately from the following "on"
    esp_task_wdt_reset();
  }

  if (switchLights) {
    delay(RELAY_PAUSE);
    tft.fillScreen(BLACK);
    for (i = 0; i < NT; i++) {
      tft.setCursor(0, i * LINEHEIGHT3);
      // Test one light relay
      tft.printf("%d. T%dLight", i+1, i+1);
      Serial.printf("%d. T%dLight\n", i+1, i+1);
      setLightRelay(i, RELAY_ON);
      delay(RELAY_PAUSE);
      esp_task_wdt_reset();
      setLightRelay(i, RELAY_OFF);
    }
  }
  esp_task_wdt_reset();
}

/**
 * Relay show.  Just for fun.  Attach lights or water pumps for best effect.
 */
void relayShow() {
  esp_task_wdt_reset();
  tft.fillScreen(BLACK);
  tft.setCursor(0, 0);
  tft.print("Watch the relays!");
  Serial.println("relayShow() is starting.  Totally optional.");
  int on = 200;
  int off = 20;
  int i;
  // All relays, pretty fast.
  for (i = 0; i < NT; i++) {
    // Heat
    setHeatRelay(i, RELAY_ON);
    delay(on);
    setHeatRelay(i, RELAY_OFF);

    delay(off);  
    // Chill
    setChillRelay(i, RELAY_ON);
    delay(on);
    setChillRelay(i, RELAY_OFF);
    delay(off);  
  }
  // Very fast
  on = 20;
  off = 1;
  for (i = 0; i < NT; i++) {
    // Heat
    setHeatRelay(i, RELAY_ON);
    delay(on);
    setHeatRelay(i, RELAY_OFF);

    delay(off);  
    // Chill
    setChillRelay(i, RELAY_ON);
    delay(on);
    setChillRelay(i, RELAY_OFF);
    delay(off);  
  }

  // Racetrack
  on = 200;
  off = 1;
  for (int j = 0; on >= 16 && j < 20; j++) {
    Serial.printf("j = %d  NT = %d  on = %d\n", j, NT, on);
    for (i = 0; i < NT; i++) {
      // Heat
      setHeatRelay(i, RELAY_ON);
      delay(on);
      setHeatRelay(i, RELAY_OFF);
      delay(off);
      on -= 2;
    }
    for (i = NT-1; i > -1; i--) {
      // Chill
      setChillRelay(i, RELAY_ON);
      delay(on);
      setChillRelay(i, RELAY_OFF);
      delay(off);  
      on -= 2;
    }
    // on = (int)(on * 0.7);
    esp_task_wdt_reset();
  }


  // All heat, all chill
  on = 2000;
  off = 500;
  for (i = 0; i < NT; i++) setHeatRelay(i, RELAY_ON);
  delay(on);
  for (i = 0; i < NT; i++) setHeatRelay(i, RELAY_OFF);
  delay(off);
  for (i = 0; i < NT; i++) setChillRelay(i, RELAY_ON);
  delay(on);
  for (i = 0; i < NT; i++) setChillRelay(i, RELAY_OFF);
  delay(off);
  // All together, now!
  for (i = 0; i < NT; i++) setHeatRelay(i, RELAY_ON);
  for (i = 0; i < NT; i++) setChillRelay(i, RELAY_ON);
  delay(on);
  delay(on);
  delay(on);
  for (i = 0; i < NT; i++) setHeatRelay(i, RELAY_OFF);
  for (i = 0; i < NT; i++) setChillRelay(i, RELAY_OFF);
  delay(off);

  tft.setCursor((int)(TFT_HEIGHT/2), 0);
  tft.print("The show is over.");
  Serial.println("relayShow() is done.");
  delay(1000);
  esp_task_wdt_reset();
}

/**
 * Originally some tanks were controlled directly by Nano pins and
 * others by a shift register.  Now two shift registers are used,
 * so the logic here is simpler.
 * The "tank" input is zero-based.
 */
void setHeatRelay(int tank, boolean state) {
 if (state) {
    bitSet(shiftRegBits, HeaterRelay[tank]);
    updateShiftRegister();
  } else {
    bitClear(shiftRegBits, HeaterRelay[tank]);
    updateShiftRegister();
  }
  strcpy(RelayStateStr[i], state ? "HTR" : "OFF");
}
void setChillRelay(int tank, boolean state) {
if (state) {
    bitSet(shiftRegBits, ChillRelay[tank]);
    updateShiftRegister();
  } else {
    bitClear(shiftRegBits, ChillRelay[tank]);
    updateShiftRegister();
  }
  strcpy(RelayStateStr[i], state ? "CHL" : "OFF");
}

/**
 * Set the light relay on or off.
 */
void setLightRelay(int tank, boolean state) {
  if (state) {
    bitSet(shiftRegBits, LightRelay[tank]);
    updateShiftRegister();
  } else {
    bitClear(shiftRegBits, LightRelay[tank]);
    updateShiftRegister();
  }
  strcpy(LightStateStr[i], state ? "LGT" : "DRK");
}

/**
 * Update the relay states to match the most recent calculations based on temperature.
 * Also check the lights if specified.
 */
void updateRelays() {
  bool lights = getLightState();
  for (i = 0; i < NT; i++) {
    // Note that controlOutput is on a scale of +/- 10000, where 10000 indicates maximum heating.
    // tempInput is in degrees C.  It is a possibly adjusted version of the sensor temperature output.
    // Below, always have "OFF" lines before "ON" since the set*Relay calls set the state string.
    if (controlOutput[i] < 0) {  // Chilling
      setHeatRelay(i, RELAY_OFF);
      if (tempInput[i] > setPoint[i] - chillOffset) {
        setChillRelay(i, RELAY_ON);
      } else {
        setChillRelay(i, RELAY_OFF);
      }
    } else {  //Heating
      setChillRelay(i, RELAY_OFF);
      if (controlOutput[i] > 0.0) {
        if (tempInput[i] > setPoint[i] - chillOffset) {
          setHeatRelay(i, RELAY_OFF);
        } else {
          setHeatRelay(i, RELAY_ON);
        }
      } else {  // controlOutput == 0
        if (tempInput[i] > setPoint[i] - chillOffset) {
          setHeatRelay(i, RELAY_OFF);
        } else {
          setHeatRelay(i, RELAY_ON);
        }
      }
    }
    if (switchLights) {
      if (lights) setLightRelay(i, RELAY_ON);
      else setLightRelay(i, RELAY_OFF);
    }
  }
}

/**
 * Update the relays be sending the current value of shiftRegBits to 
 * the shift register.
 * This function sets the latchPin to low, then calls a function
 * to shift the contents of variable shiftRegBits into the registers
 * before setting the 'latchPin' high again.
 *  Note that SHIFT_PINS specifies how many of the bits in the shiftRegBits value are actually used.
 */
void updateShiftRegister() {
  //Serial.printf("Setting relays with byte %d \n",shiftRegBits);

  digitalWrite(LATCH_PIN, LOW);
  // shiftOut on the ESP32 is too fast for the chip!  This
  // has enough delays to make it work.
  MYshiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, shiftRegBits);
  digitalWrite(LATCH_PIN, HIGH);
}

/*
 * This replacement for shiftOut is thanks to jim-p on the Arduino forums.  The first
 * digitalWrite() is new, as well as the two delayMicroseconds() calls.  Those account
 * for the fact that the ESP32 clock is too fast for the 74HC595.
 */
void MYshiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, int32_t val) {
  uint8_t i;
  //Serial.printf("In MyShiftOut shiftRegBits (val) = %d\n", val, val);
  // or this
  //Serial.print("In binary: ");
  //printBits(sizeof(val), &val);
  //Serial.println();
  digitalWrite(clockPin, LOW);

  for (i = 0; i < SHIFT_PINS; i++) {
    if (bitOrder == LSBFIRST) {
      digitalWrite(dataPin, val & 1);
      val >>= 1;
    } else {
      // This was 128 for an 8-bit input.  Now we will typically have 16, but make it variable.
      // Note that 1 << N is 2 to the power of that number.  We compute the value of
      // the highest bit we are using to set relays.
      digitalWrite(dataPin, (val & (1 << (SHIFT_PINS-1))) != 0);
      val <<= 1;
    }

    delayMicroseconds(10);
    digitalWrite(clockPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(clockPin, LOW);
  }
}

// Diagnostic print helpers from https://stackoverflow.com/questions/111928/is-there-a-printf-converter-to-print-in-binary-format
// Assumes little endian
void printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    
    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            Serial.printf("%u", byte);
        }
    }
    puts("");
}
