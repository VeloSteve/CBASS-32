// Prototypes which might have been auto-generated but are not.
void updateShiftRegister();
void MYshiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);


byte shiftRegByte = 0;

void RelaysInit()
{
  //-------( Initialize Pins so relays are inactive at reset)----
  for (i = 0; i < NT; i++) {
    // Set relay signal to off
    digitalWrite(HeaterRelay[i], RELAY_OFF);
    digitalWrite(ChillRelay[i], RELAY_OFF);
    //---( THEN set pins as Outputs )----
    pinMode(HeaterRelay[i], OUTPUT);
    pinMode(ChillRelay[i], OUTPUT);
#ifdef COLDWATER
    digitalWrite(LightRelay[i], RELAY_OFF);
    pinMode(LightRelay[i], OUTPUT);
#endif
  }
 
  // Shift Register
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
 * In COLDWATER test the light relays separately.
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
    tft.print(2 * i + 1); tft.print(". T"); tft.print(i + 1); tft.print("Heatr");
    Serial.print(2 * i + 1); Serial.print(". T"); Serial.print(i + 1); Serial.println("Heatr");
    digitalWrite(HeaterRelay[i], RELAY_ON);
    bitSet(shiftRegByte, HeaterRelayShift[i]);  updateShiftRegister();
    delay(RELAY_PAUSE);
    esp_task_wdt_reset();
    digitalWrite(HeaterRelay[i], RELAY_OFF);
    bitClear(shiftRegByte, HeaterRelayShift[i]);  updateShiftRegister();

    delay(100); // A brief delay so you can hear the "off" relay action separately from the following "on"    
    // Test one chiller relay
    lineY = (2 * i + 1) * LINEHEIGHT3 - shiftUp;
    if (shiftUp > 0) {
        tft.fillRect(0, lineY, TFT_WIDTH, LINEHEIGHT3, BLACK);
    }
    tft.setCursor(0, lineY);

    tft.print(2 * i + 2); tft.print(". T"); tft.print(i + 1); tft.print("Chillr");
    Serial.print(2 * i + 2); Serial.print(". T"); Serial.print(i + 1); Serial.println("Chillr");
    digitalWrite(ChillRelay[i], RELAY_ON);
    bitSet(shiftRegByte, ChillRelayShift[i]);  updateShiftRegister();

    delay(RELAY_PAUSE);
    esp_task_wdt_reset();
    digitalWrite(ChillRelay[i], RELAY_OFF);
    bitClear(shiftRegByte, ChillRelayShift[i]);  updateShiftRegister();

    delay(100); // A brief delay so you can hear the "off" relay action separately from the following "on"    
  }

#ifdef COLDWATER
  delay(RELAY_PAUSE);
  esp_task_wdt_reset();
  tft.fillScreen(BLACK);
  for (i = 0; i < NT; i++) {
    Serial.print("a "); Serial.println(millis());
    tft.setCursor(0, i * LINEHEIGHT3);
    // Test one light relay
    tft.print(i + 1); tft.print(". T"); tft.print(i + 1); tft.print("Light");
    Serial.print(i + 1); Serial.print(". T"); Serial.print(i + 1); Serial.println("Light");
        Serial.print("b "); Serial.println(millis());

    digitalWrite(LightRelay[i], RELAY_ON);
        Serial.print("c "); Serial.println(millis());

    delay(RELAY_PAUSE);
    esp_task_wdt_reset();
        Serial.print("d "); Serial.println(millis());

    digitalWrite(LightRelay[i], RELAY_OFF);
        Serial.print("e "); Serial.println(millis());

  }
#endif // COLDWATER
}

// This was originally just for turning off heating and cooling, but in COLDWATER we also
// check lights here.
void updateRelays() {
  for (i=0; i<NT; i++) {
    // We originally checked for tempOutput < 0, but that is never the case
    // now that SetOutputLimits is never called.  tempOutput ranges from 0.0 to 255.0
    if (tempOutput[i] < 0) { //Chilling
      if (tempInput[i] > setPoint[i] - chillOffset) {
        digitalWrite(ChillRelay[i], RELAY_ON);
        digitalWrite(HeaterRelay[i], RELAY_OFF);
        strcpy(RelayStateStr[i], "CHL");
      }
      else {
        digitalWrite(ChillRelay[i], RELAY_OFF);
        digitalWrite(HeaterRelay[i], RELAY_OFF);
        strcpy(RelayStateStr[i], "OFF");
      }
    } else { //Heating
      if (tempOutput[i] > 0.0) {  
        if (tempInput[i] > setPoint[i] - chillOffset) {
          digitalWrite(HeaterRelay[i], RELAY_ON);
          digitalWrite(ChillRelay[i], RELAY_ON);
          strcpy(RelayStateStr[i], "HTR");
        }
        else {
          digitalWrite(HeaterRelay[i], RELAY_ON);
          digitalWrite(ChillRelay[i], RELAY_OFF);
          strcpy(RelayStateStr[i], "HTR");
        }
      }
      else {
        if (tempInput[i] > setPoint[i] - chillOffset) {
          digitalWrite(HeaterRelay[i], RELAY_ON);
          digitalWrite(ChillRelay[i], RELAY_ON);
          strcpy(RelayStateStr[i], "HTR");
        }
        else {
          digitalWrite(HeaterRelay[i], RELAY_OFF);
          digitalWrite(ChillRelay[i], RELAY_ON);
          strcpy(RelayStateStr[i], "OFF");
        }
      }
    }
#ifdef COLDWATER
    // All will be on or all off.
    if (lightStatus[lightPos]) {
      digitalWrite(LightRelay[i], RELAY_ON);
    } else {
      digitalWrite(LightRelay[i], RELAY_OFF);
    }
#endif // COLDWATER
  }
}

/**
 * Update the relays be sending the current value of shiftRegByte to 
 * the shift register.
 * This function sets the latchPin to low, then calls the Arduino function
 * 'shiftOut' to shift out contents of variable shiftRegByte
 *  before putting the 'latchPin' high again.
 */
void updateShiftRegister()
{
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
void MYshiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val)
{
  uint8_t i;
  
  digitalWrite(clockPin, LOW);

  for (i = 0; i < 8; i++)  {
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
