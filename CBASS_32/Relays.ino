// Prototypes which might have been auto-generated but are not.
void updateShiftRegister();
void MYshiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);

byte shiftRegByte = 0;

void RelaysInit() {
  //-------( Initialize Pins so relays are inactive at reset)----
  // No more than 4 pins, since any beyond that are handled by the shift register.
  for (i = 0; i < min(NT, 4); i++) {
    // Set relay signal to off
    digitalWrite(HeaterRelay[i], RELAY_OFF);
    digitalWrite(ChillRelay[i], RELAY_OFF);
    //---( THEN set pins as Outputs )----
    pinMode(HeaterRelay[i], OUTPUT);
    pinMode(ChillRelay[i], OUTPUT);
  }
  // Shift Register
  // Note that it doesn't matter here whether the shift register will control heaters, lights, or chillers.
  pinMode(LATCH_PIN, OUTPUT);
  pinMode(DATA_PIN, OUTPUT);
  pinMode(CLOCK_PIN, OUTPUT);
  digitalWrite(LATCH_PIN, HIGH);
  shiftRegByte = 0;
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
 * Relays for tanks 1-4 are directly controlled by Nano pins, but any higher numbers are 
 * controlled by a shift register.  Make that choice here so other functions don't need
 * to know about it.
 * The "tank" input is zero-based.
 */
void setHeatRelay(int tank, boolean state) {
  if (tank < 4) {
    digitalWrite(HeaterRelay[tank], state);
  } else if (state) {
    bitSet(shiftRegByte, HeaterRelayShift[tank - 4]);
    updateShiftRegister();
  } else {
    bitClear(shiftRegByte, HeaterRelayShift[tank - 4]);
    updateShiftRegister();
  }
  strcpy(RelayStateStr[i], state ? "HTR" : "OFF");

}
void setChillRelay(int tank, boolean state) {
  if (tank < 4) {
    digitalWrite(ChillRelay[tank], state);
  } else if (state) {
    bitSet(shiftRegByte, ChillRelayShift[tank - 4]);
    updateShiftRegister();
  } else {
    bitClear(shiftRegByte, ChillRelayShift[tank - 4]);
    updateShiftRegister();
  }
  strcpy(RelayStateStr[i], state ? "CHL" : "OFF");
}

/**
 * Set the light relay on or off.  Note that lights are
 * always on shift registers, so we don't need a digitalWrite
 * option.
 * The shift bits are numbered backward from the end of the byte.
 */
void setLightRelay(int tank, boolean state) {
  if (state) {
    bitSet(shiftRegByte, LightRelayShift[5 - tank]);
    updateShiftRegister();
  } else {
    bitClear(shiftRegByte, LightRelayShift[5 - tank]);
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
 * Update the relays be sending the current value of shiftRegByte to 
 * the shift register.
 * This function sets the latchPin to low, then calls the Arduino function
 * 'shiftOut' to shift out contents of variable shiftRegByte
 *  before putting the 'latchPin' high again.
 */
void updateShiftRegister() {
  //Serial.printf("Setting relays with byte %d \n",shiftRegByte);

  digitalWrite(LATCH_PIN, LOW);
  // shiftOut on the ESP32 is too fast for the chip!  This
  // has enough delays to make it work.
  //MYshiftOut(DATA_PIN, CLOCK_PIN, LSBFIRST, shiftRegByte);
  MYshiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, shiftRegByte);
  digitalWrite(LATCH_PIN, HIGH);
}

/*
 * This replacement for shiftOut is thanks to jim-p on the Arduino forums.  The first
 * digitalWrite() is new, as well as the two delayMicroseconds() calls.  Those account
 * for the fact that the ESP32 clock is too fast for the 74HC595.
 */
void MYshiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val) {
  uint8_t i;

  digitalWrite(clockPin, LOW);

  for (i = 0; i < 8; i++) {
    if (bitOrder == LSBFIRST) {
      digitalWrite(dataPin, val & 1);
      val >>= 1;
    } else {
      digitalWrite(dataPin, (val & 128) != 0);
      val <<= 1;
    }

    delayMicroseconds(10);
    digitalWrite(clockPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(clockPin, LOW);
  }
}
