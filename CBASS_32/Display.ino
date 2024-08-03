void box(char* s, int line, int lineSize);

void startDisplay() {
    //canvas = GFXcanvas16(TFT_WIDTH, TFT_HEIGHT);
    pinMode(TFT_CS, OUTPUT);
    tft.begin();
    tft.setRotation(TFT_LAND); 
    tft.fillScreen(BLACK);
    // A start point for text size and color.  If changed elsewhere, change it back.
    tft.setTextSize(2);  // A reasonable size. 1-3 (maybe more) are available.
    tft.setTextColor(WHITE);
}

/**
 * The display can go all white and stay that way until a hard reboot.  Forums
 * suggest:
 * - Loose ribbon cable on the back of the display board.
 * - Other poor connections, possibly with low voltage.
 * - A bad transistor (images show it and suggest a replacement)
 * - incorrect pin mode.
 *
 * But note that analog/digital is not explicity set.  An analogRead(), 
 * digitalRead(), etc. sets it to match.
 * 
 * The display connects to DC, CS, MOSI, SCK, 3V3, and GND.
 *
 * As a test, see if calling this occasionally, or perhaps as a web action,
 * clears the condition.
 *
 * It could be interesting to power cycle the display, but that would mean hardware
 * changes to make the 3.3V supply switchable.
 */
void resetDisplayPins() {
  SPI.end();
  //SPI.begin();

  startDisplay();  // KEY!

  // SD card too?
  SDinit();

  // These four lines essentially repeat the beginning of Adafruit_SPITFT::initSPI

  pinMode(TFT_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  pinMode(TFT_DC, OUTPUT);
  digitalWrite(TFT_DC, HIGH);
    // Also be sure SD pin wasn't left in a bad state
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);
  // We are trying to overcome a white screen. Can it be set black?
  tft.fillScreen(BLACK);

}

/**
 * This is meant for status messages during startup which will
 * stay on the screen until the next step or error.
 * The "pass" static variable allows subsequent lines to stack.
 */
void tftMessage(const char* msg, bool toSerial) {
  static int pass = 0;
  if (pass == 0) {
    tft.fillScreen(tft.color565(0, 128, 0));
    tft.setCursor(0, 0);
  }

  tft.setTextColor(WHITE);
  //tft.setCursor(0, LINEHEIGHT3*pass);
  pass = (pass > 5) ? 0 : pass + 1;
  // Add code to break the string if it is longer than one line?
  // For now:
  if (strlen(msg) > 16) {
    tft.setTextSize(2);
  } else {
    tft.setTextSize(3);
  }
  tft.println(msg);
  if (toSerial) Serial.println(msg);
}

/**
 * Put a message on the display and Serial monitor, and then
 * wait long enough to trigger a reboot.  Note that the argument
 * must be something like F("My message").
 */
void fatalError(const __FlashStringHelper *msg) {
  Serial.print(F("Fatal error: "));
  Serial.println(msg);
  tft.fillScreen(BLACK);
  tft.setTextSize(3);
  tft.setTextColor(RED);
  tft.setCursor(0, 0);

  tft.println(F("Fatal error: "));
  tft.print(msg);
  // This is long enough to trigger the watchdog timer, causing a reboot.
  // It prevents the run from starting and allows for a retry in case (for
  // example) an SD card is inserted late.
  delay(WDT_TIMEOUT*1000 + 1000);
}

void nonfatalError(const __FlashStringHelper *msg) {
  Serial.print(F("Nonfatal error: "));
  Serial.println(msg);
  tft.fillScreen(BLACK);
  tft.setTextSize(3);
  tft.setTextColor(RED);
  tft.setCursor(0, 0);

  tft.println(F("Nonfatal error: "));
  tft.println("Run will resume without this capability.");
  tft.print(msg);
  // In contrast to fatalError, wait 10 seconds without triggering
  // the watchdog timer.
  for (i=0; i<5; i++) {
    delay(2000);
    esp_task_wdt_reset();
  }
  // Revert to default color.
  tft.setTextColor(WHITE);
}

void tftPauseWarning(boolean on) {
  if (!on) {
      tft.fillRect(0, 0, TFT_WIDTH, LINEHEIGHT3, BLACK);
      return;
  }
  tft.fillRect(0, 0, TFT_WIDTH, LINEHEIGHT3, WHITE);
  tft.setCursor(0, 0);
  tft.setTextSize(2);
  int duration = (int)((millis() - startPause) / 1000);
  if (duration > 20) {
    tft.setTextColor(RED);
    tft.printf("No Log for %d seconds. Reboot?", duration);
  } else if (duration > 10) {
    tft.setTextColor(BLUE);
    tft.print("Log paused >10 seconds.");
  } else {
    tft.setTextColor(BLACK);
    tft.print("Log paused by web action.");
  }
  tft.setTextColor(WHITE);
}

/*  Unbuffered version.  Faster and saves memory, but only significant on AVR-based Arduinos.
void displayTemperatureStatusBold() {
    tft.setTextSize(2);
    // Only clear below the heading for less flashing.  Also don't clear the boxes, which are refreshed anyway.
    //tft.fillRect(LINEHEIGHT*2, LINEHEIGHT3, TFT_WIDTH-LINEHEIGHT3*4, TFT_HEIGHT-LINEHEIGHT3, BLACK);
    //Header
    if (!logPaused) {
      // Leave a different message in place
      tft.setCursor(0,0);
      tft.print("     SETPT  INTEMP   RELAY");
    }
    tft.setTextSize(3);
    int shiftUp = (NT >= 8) ? 5 : 0;
    int shrinkBox = (NT >= 8) ? 2 : 1;
    int lineTop;
    for (i=0; i<NT; i++) {
      lineTop = LINEHEIGHT3*(i+1) - shiftUp;
      if (NT >= 8) shiftUp = shiftUp + 2;
      tft.fillRect(LINEHEIGHT*2, lineTop, TFT_WIDTH-LINEHEIGHT3*4, LINEHEIGHT3, BLACK);
      tft.setCursor(0, lineTop);
      tft.print("T"); tft.print(i+1); tft.print(" ");
      dtostrf(setPoint[i], 4, 1, setPointStr);
      tft.print(setPointStr);
      tft.print(" ");
      dtostrf(tempInput[i], 4, 1, tempInputStr);
      tft.print(tempInputStr);
      box(RelayStateStr[i], lineTop, LINEHEIGHT3-2 - shrinkBox);
    }

    // Add time and IP address in the smallest font at the bottom of the screen.

    tft.setCursor(1, TFT_HEIGHT-8);
    tft.setTextSize(0);
    tft.setTextColor(GREEN);
    tft.fillRect(0, TFT_HEIGHT-8, TFT_WIDTH, 8, BLACK);

    tft.print(gettime());  tft.print("   IP: ");
    tft.print(myIP.toString().c_str());

    // Temporarily add boot time as a diagnostic
    tft.print("  Started: ");
    tft.print(bootTime.c_str());
    tft.setTextColor(WHITE);
 
    // To add diagnostics after the temperature lines:
    // tft.fillRect(LINEHEIGHT*2, LINEHEIGHT3*5, TFT_WIDTH-LINEHEIGHT3*4, LINEHEIGHT3, BLACK);
    // tft.setCursor(0, LINEHEIGHT3*5);
    // tft.print("Loop ms "); tft.print(someGlobalVariable);
}
 */


// This version eliminates screen flicker by drawing to a buffer (canvas) and drawing that
// to the screen.  Unfortunately, it increased update time from 183 ms to 1566 ms!  That's 
// too slow, but I'm keeping the code temporarily, hoping to come up with a faster buffer.
// If enabled, a canvas variable will be needed globally or as a static here:
GFXcanvas1 canvas(TFT_WIDTH-LINEHEIGHT3*2, LINEHEIGHT3); // For blink-free line updates on the screen.
GFXcanvas1 canvasNarrow(TFT_WIDTH, 8); // For blink-free line updates on the screen.
void displayTemperatureStatusBold() {
    tft.setTextSize(2);
    // Only clear below the heading for less flashing.  Also don't clear the boxes, which are refreshed anyway.
    //tft.fillRect(LINEHEIGHT*2, LINEHEIGHT3, TFT_WIDTH-LINEHEIGHT3*4, TFT_HEIGHT-LINEHEIGHT3, BLACK);
    //Header
    if (!logPaused) {
      tft.setCursor(0,0);
      tft.print("     SETPT  INTEMP   RELAY");
    }
    tft.setTextSize(3);
    int shiftUp = (NT >= 8) ? 5 : 0;
    int shrinkBox = (NT >= 8) ? 2 : 1;
    int lineTop;
    canvas.setTextSize(3); // Drawing to a canvas first prevents display flashing.
    for (i=0; i<NT; i++) {
      lineTop = LINEHEIGHT3*(i+1) - shiftUp;
      if (NT >= 8) shiftUp = shiftUp + 2;
      canvas.fillScreen(BLACK);
      canvas.setCursor(0, 0);
      canvas.print("T"); canvas.print(i+1); canvas.print(" ");
      dtostrf(setPoint[i], 4, 1, setPointStr);
      canvas.print(setPointStr);
      canvas.print(" ");
      dtostrf(tempInput[i], 4, 1, tempInputStr);
      canvas.print(tempInputStr);
      // The canvas is sized to overwrite the text in the old line, but not the relay box at the end.
      //tft.drawBitmap(0, LINEHEIGHT3 * (i+1), canvas.getBuffer(), TFT_WIDTH-LINEHEIGHT3*2, LINEHEIGHT3, WHITE, BLACK);
      tft.drawBitmap(0, lineTop, canvas.getBuffer(), TFT_WIDTH-LINEHEIGHT3*2, LINEHEIGHT3, WHITE, BLACK);
      box(RelayStateStr[i], lineTop, LINEHEIGHT3-2 - shrinkBox);
    }

        // Add time and IP address in the smallest font at the bottom of the screen.
    canvasNarrow.setCursor(1, 0);
    canvasNarrow.setTextSize(0);
    canvasNarrow.setTextColor(GREEN);
    canvasNarrow.fillRect(0, 0, TFT_WIDTH, 8, BLACK);

    canvasNarrow.print(gettime());  canvasNarrow.print("   IP: ");
    canvasNarrow.print(myIP.toString().c_str());

    // Temporarily add boot time as a diagnostic
    canvasNarrow.print("  Started: ");
    canvasNarrow.print(bootTime.c_str());
    tft.drawBitmap(0, TFT_HEIGHT-8, canvasNarrow.getBuffer(), TFT_WIDTH, 8, GREEN, BLACK);

    // To add diagnostics after the temperature lines:
    // tft.fillRect(LINEHEIGHT*2, LINEHEIGHT3*5, TFT_WIDTH-LINEHEIGHT3*4, LINEHEIGHT3, BLACK);
    // tft.setCursor(0, LINEHEIGHT3*5);
    // tft.print("Loop ms "); tft.print(someGlobalVariable);
}


// Draw a box with color based on relay state.
// Place on the zero-based line specified.
void box(char* s, int lineTop, int lineSize) {
  word color;
  if (strcmp(s, "OFF") == 0) {
    color = BLACK;
  } else if (strcmp(s, "HTR") == 0) {
    color = RED;
  } else if (strcmp(s, "CHL") == 0) {
    color = BLUE;
  } else {
    color = YELLOW; // Yellow = caution.  This should not happen.
  }
  // x, y, width, height, color
  tft.fillRect(TFT_WIDTH-1.5*lineSize, lineTop, lineSize-1, lineSize-1, color);
}

// Draw a box with color based on relay state.
// Place on the zero-based line specified.
void boxOld(char* s, int line, int lineSize) {
  word color;
  if (strcmp(s, "OFF") == 0) {
    color = BLACK;
  } else if (strcmp(s, "HTR") == 0) {
    color = RED;
  } else if (strcmp(s, "CHL") == 0) {
    color = BLUE;
  } else {
    color = YELLOW; // Yellow = caution.  This should not happen.
  }
  // x, y, width, height, color
  tft.fillRect(TFT_WIDTH-1.5*lineSize, lineSize*line, lineSize-1, lineSize-1, color);
}
