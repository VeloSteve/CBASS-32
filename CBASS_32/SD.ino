
// Function prototypes
void fillBuffer(byte c);
void fillBuffer(byte c, boolean pin, boolean spaceOK);
unsigned long timeInMinutes(byte c);
int tempInHundredths(byte c);
void printRampPlan();
void printAsHM(unsigned int t);
bool myFileCopy(const char* ooo, const char* nnn);
bool resetSettings();
bool createBackupFileName(char* name);
String gettime();

// Global settings file, since position isn't preserved unless the file is returned from functions (in SdFat).
File32 settingsFile;

// Length of one log line GRAPHPTS.LOG
const byte bytesPerTempLine = 52;  // 5 digits for day, 4 for minute, 8 4-char floats, 9 commas, 2 for CRLF (0x0d 0x0a)
const byte graphBufLen = bytesPerTempLine + 5;
char logBuf[graphBufLen];
// Buffer for general reading of SD as fast as possible.  The absolute optimum may be higher than this,
// but somewhere above 6000 we get crashes.
const int sdBufLen = 4096;
char sdBuffer[sdBufLen];


void SDinit() {
  pinMode(SD_CS, OUTPUT);  // probably redundant

  // Initialize the SD card.
  bool init = true;
  if (!SDF.begin(SD_CONFIG)) {
    Serial.println("Failed to initialize microSD card.  calling initErrorHalt.");

    SDF.initErrorHalt(&Serial);
    init = false;
  }
  if (!init) fatalError(F("SD initialization failed!"));

  Serial.println("SD initialization done.");
  if (!SDF.exists("/")) {
    Serial.println("but root directory does not exist!");
    fatalError(F("SD has no root directory!"));
    while (1)
      ;
  }
}

// For debugging, this can be called between steps which might affect the SD.
void checkSD(const char* txt) {
  if (!SDF.exists("/")) {
    Serial.printf("SD has no root directory!\n");
    fatalError(F("SD has no root directory!"));
  } else {
    Serial.printf("SD ok at %s\n", txt);
  }
}

/**
 * Read the ramp plan one line at a time.  This should be easier than the old
 * method.  It will take more memory, but we can afford that now.
 * Be sure to account for cases where NT has changed since Settings.ini was
 * written.
 * Lines can be comments, ramp points, or key-value pairs as follows:

  // A comment - must start at the beginning of a line
START 14:30
INTERP LINEAR|STEP
// START, if provided, causes ramp times to be interpreted as relative to that time.  For example
START 15:00
0:00 30 30 30 30
3:00 30 30 33 33

  * The lines above mean to have all tanks at 30 degrees at 3 PM and ramp tanks 3 and 4 to 33 degrees by 6 PM.
  * Times not falling within the range specified in ramp steps will have a target temperature equal to the last step.
  * If INTERP is LINEAR temperatures will be interpolated between the two times.  If STEP, temperatures
  * will stay at 30 until 6 PM and then attempt to jump to 33 degrees.  STEP is only for backward
  * compatibility.  LINEAR is the default as of 25 Apr 2022.
  * 
  * These are the only two interpolation options, but something like a cubic spline could be added
  * for smooth simulation of diurnal variations.
  */
void readRampPlan() {
  int maxLine = 128;
  char lineBuffer[maxLine+1];
  char *lb = lineBuffer;
  int nRead, nParse, pos, hh, mm;
  double tempRead;
  int tank = 0;
  int extra = 0;
  boolean ntFail = false;
  int upTo8[8];  // Read this many temperatures if in the file, then check against NT.
  if (SDF.exists("/Settings.ini")) {
    Serial.println("The ramp plan exists.");
  } else {
    if (!SDF.exists("/")) {
      Serial.println("and root directory does not exist!!");
      fatalError(F("SD has no root directory!!"));
    }
    fatalError(F("---ERROR--- No ramp plan file (/Settings.ini)!"));
  }
  settingsFile = SDF.open("/Settings.ini", O_RDONLY);
  while (settingsFile.available()) {
    nRead = settingsFile.readBytesUntil('\n', lineBuffer, maxLine);  // One line is now in the buffer.
    lineBuffer[max(0, nRead)] = 0;  // null terminate the line!
    // There MAY be a problem since lines edited in different ways could end with \n, \r, or both.
    // To be safe, see if one of those remains in the buffer or the file and get rid of it.
    // Note that some of these four cases may never occur, but until proven otherwise, they should be kept.
    if (lineBuffer[nRead-1] == '\n' || lineBuffer[nRead-1] == '\r') {
        lineBuffer[max(0, nRead-1)] = 0;  // null terminate on top of the extra character!
    } else {
        lineBuffer[max(0, nRead)] = 0;  // null terminate the line normally
    }
    // Now discard any leftover end of line in the file.  This also removes any empty next line, but that's fine.
    while (settingsFile.peek() == '\n') settingsFile.read();
    while (settingsFile.peek() == '\r') settingsFile.read();

    // Serial.printf("Read >%s<\n", lineBuffer);
    if (nRead == 0 || lineBuffer[0] == '/') {
      ;  // an empty line or comment, move on.
    } else if (!strncmp(lineBuffer, "START", 5)) {
      // The line should contain a start time after START
      nParse = sscanf(lineBuffer + 5, "%d:%d", &hh, &mm);
      if (nParse != 2) {
        settingsFile.close();
        fatalError(F("Invalid START time in Settings.ini"));
      } else if (hh < 0 || hh > 23) {
        settingsFile.close();
        fatalError(F("START time hour must be from 0 to 23."));
      } else if (mm < 0 || hh > 59) {
        settingsFile.close();
        fatalError(F("START time minutes must be from 0 to 59."));
      }
      relativeStartTime = hh * 60 + mm;
      relativeStart = true;
    } else if (!strncmp(lineBuffer, "INTERP", 6)) {
      pos = 6;
      while (isSpace(lineBuffer[pos])) pos++;
      if (!strncmp(lineBuffer + pos, "LINEAR", 6)) {
        interpolateT = true;
      } else if (!strncmp(lineBuffer + pos, "STEP", 4)) {
        interpolateT = true;
      } else {
        settingsFile.close();
        fatalError(F("Unsupported interpolation option.  Must be LINEAR or STEP"));
      }
    } else if (isDigit(lineBuffer[0])) {
      // 07:00       26.00  26.00  26.00  26.00	24.0
      // Time and temperature lines (and only such lines) should start with a digit.
      // A time is required, ideally followed by NT temperatures, but handle the case
      // where the number doesn't match!
      nParse = sscanf(lineBuffer, "%d:%d", &hh, &mm);
      //Serial.printf("On temp line got %d values, %d and %d\n", nParse, hh, mm);

      rampMinutes[rampSteps] = hh * 60 + mm;
      // There are ways to use sscanf in a loop, but strtok seems nicer here.
      char* token;
      token = strtok(lineBuffer, " \t");  // the time - already handled
      token = strtok(NULL, " \t");        // first temperature in same line (NULL)
      tank = 0;
      extra = 0;
      while (token != NULL) {
        if (tank < NT) {
          sscanf(token, "%lf", &tempRead);
          // Serial.printf("Scanned temp as %lf for tank %d line %d\n", tempRead, tank, rampSteps);
          rampHundredths[tank++][rampSteps] = round(tempRead * 100);
          //Serial.printf("%d hundredths.\n", rampHundredths[tank-1][rampSteps]);
        } else {
          extra++;
        }
        token = strtok(NULL, " \t");
      }
      if (tank < NT) {
        // Settings don't match the number of tanks, but don't stop until the available
        // data is read so it can be edited rather than starting from scratch.
        ntFail = true;
      }
      rampSteps++;
    } else {
      settingsFile.close();
      Serial.printf("Bad line: >%s<\n", lineBuffer);
      fatalError(F("Settings.ini has an illegal line.  Edit (/RampPlan) or reset (/ResetRampPlan)."));
    }
  }
  if (extra > 0) {
    Serial.printf("WARNING: Ignoring %d extra tanks in Settings.ini with NT = %d\n", extra, NT);
  }
  settingsFile.close();
  if (ntFail) {
    // If the user specified NT tanks and the are less than NT in the plan, an experiment
    // could be ruined.  Consider this a fatal error.  Try to keep the web server
    // running so this can be addressed without pulling the card.
    Serial.printf("Settings.ini supports %d tanks, but CBASS is configured for %d\n", tank, NT);
    Serial.printf("Use the controls at http://%s/RampPlan or http://%s/ResetRampPlan\n", myIP.toString().c_str(), myIP.toString().c_str());
    fatalError(F("Settings.ini has fewer temperatures than you have tanks.  Edit (/RampPlan), reset (/ResetRampPlan), or adust NT."));
  } else {
    printRampPlan();
  }
}

// If no second argument, expect a keyword, not a pin.
void fillBuffer(byte c) {
  fillBuffer(c, false, false);
}
/**
   We are expecting a keyword, PIN, or BLE name
   If pin, accept anything except whitespace.
   Spaces are allowed in BLE names, so in that case read to end of line.
   If !pin and !spaceOK it's a keyword so only accept letters.
   Fill the global buffer with any acceptable characters.
   The buffer starts with c, which has already been read.
   Leave the file ready to read from the first unwanted character read, so use "peek".
   isWhitespace: space or tab
   isSpace: also true for things like linefeeds
*/
void fillBuffer(byte c, boolean pin, boolean spaceOK) {
  byte pos;
  iniBuffer[0] = c;
  pos = 1;
  while (settingsFile.available()) {
    c = settingsFile.peek();
    if ((!pin && !spaceOK && isAlpha(c)) || (pin && !isSpace(c)) || (spaceOK && (!isSpace(c) || isWhitespace(c)))) {
      iniBuffer[pos++] = c;
      c = settingsFile.read();  // consume the character
      if (pos >= BUFMAX) fatalError(F("Settings.ini keyword too long for BUFMAX."));
    } else {
      iniBuffer[pos] = 0;  // null terminate
      return;
    }
  }
}

/**
   We expect a time like 1:23 in hours and minutes, with optional leading zeros and
   possibly on the hour.
   Return it in minutes, which may be interpreted either relative to a base time
   or relative to midnight, depending on context.
   Leave the file ready to read from the first unwanted character read, so use "peek".
*/
unsigned long timeInMinutes(byte c) {
  byte num;
  unsigned int mins = 0;
  // Read hours, which are mandatory.
  num = c - '0';
  while (settingsFile.available()) {
    c = settingsFile.peek();
    if (isDigit(c)) {
      num = num * 10 + c - '0';
      c = settingsFile.read();
    } else {
      // Must be at end of hour.
      mins = num * 60;
      break;
    }
  }
  // See if there are minutes
  if (settingsFile.peek() != ':') return mins;
  settingsFile.read();  // consume the colon.
  num = 0;
  while (settingsFile.available()) {
    c = settingsFile.peek();
    if (isDigit(c)) {
      num = num * 10 + c - '0';
      c = settingsFile.read();
    } else {
      // Must be at end of minute.
      mins = mins + num;
      return mins;
    }
  }
  return mins;  // We should get here only if there was a colon at the end of the file.
}


/**
   We expect a temperature like 1.23 in degrees C, either an integer or with decimal places.
   Leave the file ready to read from the first unwanted character read, so use "peek".

   We return hundredths of a degree, so +-327 degrees is supported by a 2-byte int.
*/
int tempInHundredths(byte c) {
  float divisor;
  byte num;
  float tValue = 0;
  // Read integer degrees, which are mandatory.
  num = c - '0';
  while (settingsFile.available()) {
    c = settingsFile.peek();
    if (isDigit(c)) {
      num = num * 10 + c - '0';
      c = settingsFile.read();
    } else {
      // Must be at end of integer part.
      tValue = (float)num;
      break;
    }
  }
  // See if there are decimals
  if (settingsFile.peek() != '.') return round(tValue * 100);
  settingsFile.read();  // consume the period.
  num = 0;
  divisor = 10;
  while (settingsFile.available()) {
    c = settingsFile.peek();
    if (isDigit(c)) {
      num = +c - '0';
      tValue = tValue + (double)(c - '0') / divisor;
      divisor = divisor * 10;
      c = settingsFile.read();
    } else {
      // Must be at end of decimal places.
      return round(tValue * 100);
    }
  }
  return round(tValue * 100);  // We should get here only if there was a period at the end of the file.
}

void printRampPlan() {
  Serial.printf("%d tanks and %d ramp lines.\n", NT, rampSteps);
  printBoth("Temperature Ramp Plan");
  printlnBoth();
  if (relativeStart) {
    printBoth("Settings will be applied relative to start time ");
    printAsHM(relativeStartTime);
    printlnBoth();
  }
  printBoth("Time  ");
  for (int j = 0; j < NT; j++) {
    printBoth("  Tank ");
    printBoth(j + 1);
  }
  printlnBoth();
  for (short k = 0; k < rampSteps; k++) {
    printAsHM(rampMinutes[k]);
    for (int j = 0; j < NT; j++) {
      printBoth("   ");
      printBoth(((double)rampHundredths[j][k]) / 100.0, 2);
    }
    printlnBoth();
  }
}

// Print minutes as hh:mm, e.g. 605 becomes 10:05
void printAsHM(unsigned int t) {
  if (t < 10 * 60) printBoth("0");
  printBoth((int)floor(t / 60));
  printBoth(":");
  if (t % 60 < 10) printBoth("0");
  printBoth(t % 60);
}


#ifdef COLDWATER
//This block of code is only for systems with lights controlled by relays as specified in LightRelay[]
/**
   Read the content of LightPln.ini  Lines can be comments or an on or off time.
   All tanks switch together, so there is only one on or off per line.
   It is assumed that we want the last state specified, wrapping to the previous day if needed.  In the
   example below, if the system starts at 6 AM, lights should be off.

  // A comment - must start at the beginning of a line
  7:00 1
  19:00 0
  // The lines above mean on at 7 AM, off at 7 PM.
*/
void readLightPlan() {
  Serial.println("Reading light plan");
  lightSteps = 0;
  // State machine:
  const byte ID = 0;      // Identifying what type of line this is.
  const byte SWITCH = 2;  // Read on or off as 1 or 0.
  const byte NEXT = 3;    // Going to the start of the next line, ignoring anything before.
  byte state = 0;
  if (SDF.exists("/LightPln.ini")) {
    Serial.println("The light plan exists.");
    settingsFile = SDF.open("/LightPln.ini", O_RDONLY);
  } else {
    fatalError(F("---ERROR--- No light plan file!"));
  }
  char c;
  while (settingsFile.available()) {
    c = settingsFile.read();
    switch (state) {
      case ID:
        // Valid characters are a digit of a number, slash, or whitespace
        if (c == '/') {
          state = NEXT;
        } else if (isDigit(c)) {
          // Time at the start of a line must be the beginning of a switch point
          // Check that there is space to store a new step.
          if (lightSteps >= MAX_LIGHT_STEPS) {
            fatalError(F("Not enough light steps allowed!  Increase MAX_LIGHT_STEPS in Settings.ini"));
            return;
          }
          lightMinutes[lightSteps] = timeInMinutes(c);
          state = SWITCH;
        } else if (isSpace(c)) {
          state = ID;  // ignore space or tab, keep going
        } else {
          Serial.print("---ERROR in settings.txt.  Invalid line with ");
          Serial.print(c);
          Serial.println("---");
        }
        break;
      case SWITCH:
        // Get an on off state or fail.
        if (isSpace(c) || c == ',') break;  // keep looking
        if (isDigit(c) && (c == '0' || c == '1')) {
          lightStatus[lightSteps] = (c == '0') ? 0 : 1;
          esp_task_wdt_reset();
        } else {
          Serial.print("---ERROR in settings.txt--- Light switch state needed here. Got c = ");
          Serial.println(c);
          fatalError(F("Incomplete light line."));
        }
        lightSteps++;
        state = NEXT;
        break;
      case NEXT:
        // Consume everything until any linefeed or characters have been removed, ready for  new line.
        if (c == 10 || c == 13) {  // LF or CR
          // In case we have CR and LF consume the second one.
          c = settingsFile.peek();
          if (c == 10 || c == 13) {
            c = settingsFile.read();
          }
          state = ID;
        }
        break;
      default:
        Serial.println("---ERROR IN LIGHT SETTINGS LOGIC---");
        fatalError(F("Bad light settings logic"));
    }
  }
  // It is okay to end in the NEXT or ID state, but otherwise we are mid-line and it's an error.
  if (state != NEXT && state != ID) {
    Serial.print("---ERROR out of light data with step incomplete.  state = ");
    Serial.println(state);
    fatalError(F("Bad light settings line"));
  }

  settingsFile.close();
  printLightPlan();
}

void printLightPlan() {
  printBoth("Lighting Plan");
  printlnBoth();
  printBoth("Settings are based on time of day.");
  printlnBoth();
  printBoth("Time    Light State");
  printlnBoth();
  for (short k = 0; k < lightSteps; k++) {
    printAsHM(lightMinutes[k]);
    printBoth("   ");
    printBoth(lightStatus[k]);
    printlnBoth();
  }
}

#endif  // COLDWATER

/**
 * Make a new copy of Settings.ini on the SD card from the current settings.
 * Add a comment that this has been done.  Ideally, prior comments will be retained.
 *
 * XXX care take to keep a backup and check before deleting but there is NO
 * XXX lock to prevent both cores from trying to access a file at once.  Fix
 * XXX this if multi-core methods are used as planned for the web server.
 **/
bool rewriteSettingsINI() {
  // For safety, copy the original (short term backup)
  Serial.println("rewrite 0");
  if (SDF.exists("/SetCopy.ini")) {
    Serial.println("Found old SetCopy.");
    if (SDF.remove("/SetCopy.ini")) {
      Serial.println("Removed old SetCopy.");
    } else {
      Serial.println("==============  FAILED to remove old SetCopy.");
      return false;
    }
  }
  if (!myFileCopy("/Settings.ini", "/SetCopy.ini")) return false;
  Serial.println("rewrite 0.1");

  // We must back up the previous file long term, so make a name.
  if (!SDF.exists("/WebBack")) {
    if (!SDF.mkdir("/WebBack")) {
      Serial.println("ERROR: Failed to make a backup directory.  Abandoning INI rewrite.");
      return false;
    }
  }
  Serial.println("rewrite 1");

  char newName[32];
  bool goodName = createBackupFileName(newName);

  Serial.println("rewrite 2");

  if (!goodName) {
    Serial.println("Could not make a valid INI backup name.  Too many files?");
    return false;
  }
  Serial.println("rewrite 3");


  // Create new file, but still don't touch the old one.  First get rid of old copies if they exist.
  if (SDF.exists("/SetMods.xxx")) {
    Serial.println("Found old SetMods.");
    if (SDF.remove("/SetMods.xxx")) {
      Serial.println("Removed old SetMods.");
    } else {
      Serial.println("==============  FAILED to remove old SetMods.");
    }
  }
  File32 mod = SDF.open("/SetMods.xxx", O_WRONLY | O_CREAT);
  // Calling size() just after open seems to give a bad value - it may even affect the file!
  //Serial.printf("Empty (?) modified file size is %d\n", mod.size());

  Serial.println("\n== Starting to write modified settings.==\n");

  // WARNING: for some reason mod.println() writes a "carriage return" instead
  // of the usual "newline".  This can be avoided by using mod.print("\n") or a printf()

  if (relativeStart) {
    mod.print("// This file was modified via the web interface at ");
    // DDDDD
    mod.flush();
    Serial.printf("In-progress file size is %d\n", mod.size());
    Serial.printf(" gettime gives >%s<", gettime().c_str());
    mod.print(gettime().c_str());
    mod.print(" on ");
    Serial.printf(" getdate gives >%s<", getdate().c_str());
    mod.print(getdate().c_str());
    mod.print("\n// Prior comments have been lost.  The previous file will be saved as ");
    mod.print(newName);
    mod.print("\n");
    // DDDDD
    mod.flush();
    Serial.printf("In-progress file size is %d\n", mod.size());
    // Start time
    if (relativeStart) {
      mod.print("START ");
      mod.print((int)(relativeStartTime / 60));
      mod.print(":");
      int min = relativeStartTime % 60;
      if (min == 0) {
        mod.print("00");
      } else if (min < 10) {
        mod.print("0");
        mod.print(min);
      } else {
        mod.print(min);
      }
      mod.print("\n");
    }
    printBoth("Settings will be applied relative to start time ");
    printAsHM(relativeStartTime);
    printlnBoth();
  }
  // DDDDD
  mod.flush();
  Serial.printf("In-progress file size is %d\n", mod.size());
  Serial.println("rewrite 4");

  // Note that if other interpolation options (e.g. SPLINE) are implemented,
  // this must be updated.
  if (interpolateT) {
    mod.print("INTERP LINEAR\n");  // The default
  } else {
    mod.print("INTERP STEP\n");
  }
  Serial.println("rewrite 5");

  // The ramp plan.
  if (relativeStart) {
    mod.print("// Ramp plan relative to the start time.  Times are H:MM or HH:MM.\n");
  } else {
    mod.print("// Ramp plan in 24-hour time of days.  Times are in H:MM or HH:MM.\n");
  }
  Serial.println("rewrite 6");
  // DDDDD
  mod.flush();
  Serial.printf("In-progress file size is %d\n", mod.size());
  mod.print("// Time");
  for (int j = 0; j < NT; j++) {
    mod.print("     T");
    mod.print(j + 1);
  }
  mod.print("\n");
  for (short k = 0; k < rampSteps; k++) {
    int t = rampMinutes[k];
    if (t < 10 * 60) mod.print("0");
    mod.print((int)floor(t / 60));
    mod.print(":");
    if (t % 60 < 10) mod.print("0");
    mod.print(t % 60);
    mod.print("     ");

    for (int j = 0; j < NT; j++) {
      mod.print("  ");
      mod.print(((double)(rampHundredths[j][k]) / 100.0), 2);
    }
    mod.print("\n");
  }
  // DDDDD
  mod.flush();
  Serial.printf("In-progress file size is %d\n", mod.size());

  mod.close();

  Serial.println("rewrite 7");

  // Now we are ready to move files.  Unfortunately, there is no "move" or "rename".  We have to
  // open both files and copy the bytes!
  // First the backup

  if (!myFileCopy("/Settings.ini", newName)) {
    // It it exists, unknown error.  If not try copying the modified file anyway.
    if (SDF.exists("/Settings.ini")) return false;
    // Carry on, hoping we can create one.
  }
  // Get rid of the old version of the active file which was just copied.
  if (!SDF.remove("/Settings.ini")) return false;

  // We just removed Settings.ini, but be sure.
  if (SDF.exists("/Settings.ini")) {
    Serial.println("Settings.ini was not removed as planned.  FATAL ERROR.");
    return false;
  } else {
    Serial.println("Correctly, Settings.ini does not exist at this time.");
  }

  // Copy in the fresh one.
  if (!myFileCopy("/SetMods.xxx", "/Settings.ini")) {
    Serial.println("FATAL ERROR: failed to copy modified Settings.ini to replace the removed one.");
    return false;
  }
  Serial.println("rewrite 8");

  SDF.remove("/SetMods.xxx");
  Serial.println("rewrite 9");

  return true;
}

/**
 * Write a generic Settings.ini, archiving anything which was there before.
 * This is mainly for a new SD card or after recovery from a serious error.
 */
bool resetSettings() {
  if (SDF.exists("/Settings.ini")) {
    char newName[32];
    bool goodName = createBackupFileName(newName);
    if (!myFileCopy("/Settings.ini", newName)) {
      Serial.println("FATAL ERROR: failed to copy an existing Settings.ini to a backup name.");
      return false;
    }
    if (!SDF.remove("/Settings.ini")) {
      Serial.println("Could not remove old Settings.ini.  Is it write protected?");
      return false;
    }
  }
  // Old Settings.ini is gone.  Write a new one.
  File32 f = SDF.open("/Settings.ini", O_WRONLY | O_CREAT);
  if (!f) {
    Serial.println("Could not open a new Settings.ini. Remove and repair the SD card.");
    return false;
  } else {
    // f.print(defaultINI);
    f.print("// A typical ramp preceeded by some tests\n// Start time (1 PM in this example)\n");
    f.print("START 13:00\n// LINEAR is the default, so this is just a reminder.\nINTERP LINEAR\n");
    f.print("// Times in column 1 are relative to START.\n// The temperatures are targets for each tank in Celsius.\n");
    int i;
    f.print("0:00");
    for (i = 0; i < NT; i++) f.printf("%5d", 24);
    f.print("\n");
    f.print("3:00");
    for (i = 0; i < NT; i++) f.printf("%5d", 28 + i);
    f.print("\n");
    f.print("6:00");
    for (i = 0; i < NT; i++) f.printf("%5d", 28 + i);
    f.print("\n");
    f.print("7:00");
    for (i = 0; i < NT; i++) f.printf("%5d", 24);
    f.print("\n");
    f.close();
    // Now refresh the arrays from the new file.
    readRampPlan();
  }
  return true;
}

/*
 * Find an unused name in WebBack and set it in "name".  Make
 * the directory but not the file at this time.  Return true if successful.
 */
bool createBackupFileName(char* name) {
  if (!SDF.exists("/WebBack")) {
    if (!SDF.mkdir("/WebBack")) {
      Serial.println("ERROR: Failed to make a backup directory.  Abandoning INI rewrite.");
      return false;
    }
  }
  for (int i = 0; i < 1000; i++) {
    sprintf(name, "/WebBack/Rbk%03u.ini", i);
    if (!SDF.exists(name)) {
      return true;
    }
  }
  return false;
}

/**
 * Alternative arguments.
 */
bool myFileCopy(const char* ooo, String nnn) {
  return myFileCopy(ooo, nnn.c_str());
}

/**
 * File does not have a copy function, so open a new file and copy the contents.
 * return true if successful, otherwise false.
 */
bool myFileCopy(const char* ooo, const char* nnn) {
  Serial.printf("my Copy 0 from %s to %s\n", ooo, nnn);

  if (ooo[0] != '/') {
    Serial.println("ERROR: file to copy must start with forward slash!");
    return false;
  } else if (nnn[0] != '/') {
    Serial.println("ERROR: copy destination must start with forward slash!");
    return false;
  }

  if (!SDF.exists("/")) {
    Serial.printf("SD root directory is gone!\n");
    return false;
  }

  if (!SDF.exists(ooo)) {
    Serial.printf("Original file %s does not exist!\n", ooo);
    return false;
  }
  Serial.println("my Copy 1");

  File32 f = SDF.open(ooo, O_RDONLY);
  File32 copy = SDF.open(nnn, O_WRONLY | O_CREAT);  // NOTE: this will overwrite if name is re-used!
  int origSize = f.size();

  if (!f) {
    Serial.printf("FATAL ERROR: failed to open original file %s.\n", ooo);
    return false;
  } else if (!copy) {
    Serial.printf("FATAL ERROR: failed to open new file %s for copying.\n", nnn);
    return false;
  }
  Serial.println("my Copy 2");

  uint32_t m_start = millis();

  int n;
  uint8_t buf[4096];  // Was 128.  Why so small?  512 is typical for SD
  int www;
  int watchdog = 0;  // Occasionally reset the watchdog.
  int dots = 0;
  // TODO: check for www != n, or www = 0.  Could create an unending loop.
  while ((n = f.read(buf, sizeof(buf))) > 0) {
    // Serial.printf("Read %d bytes from original file of size %d.  ", n, origSize);
    www = copy.write(buf, n);
    copy.flush();
    // Serial.printf(" wrote %d bytes to the copy.  New size = %d\n", www, copy.size());
    Serial.print(".");
    if (dots == 80) {
      Serial.println();
      dots = 0;
    }
    watchdog++;
    if (watchdog == 256) {
      // About once per megabyte, reset the timer.  In tests it times out at about 3.6 MB.
      esp_task_wdt_reset();
      watchdog = 0;
    }
  }
  Serial.println("\nmy Copy 3");

  // Not a full check, but at least be sure the file sizes match.
  copy.flush();
  if (copy.size() != origSize) {
    Serial.printf("my Copy 4 original size was %d, copied file was %d bytes.\n", origSize, copy.size());
    f.close();
    copy.close();
    SDF.remove(nnn);
    return false;
  }
  Serial.printf("Copy took %d seconds for %d bytes.\n", (int)((millis() - m_start) / 1000UL), origSize);
  Serial.println("my Copy 5");

  f.close();
  copy.close();


  Serial.println("my Copy 6 (complete)");

  return true;
}

/*
   Read from tryHere to a newline, into the provided buffer.
*/
int readHere(File32 f, char* buf, long tryHere) {
  f.seek(tryHere);
  int n = f.readBytesUntil('\n', buf, graphBufLen - 1);
  buf[max(0, n)] = '\0';
  return n;
}

/**
   If called from setup(), delete the plotting file on restart for easier testing.  Note that
   we do not want this during "real" runs, in case of accidental reboots.
   This can also be called on demand, if implemented.
*/
void clearTemps() {
  SDF.remove("GRAPHPTS.TXT");
}


/**
   The following printBoth functions allow printing identical output to the serial monitor and log file
   without having to put both types of print everywhere in the code.  It also moves checking for an
   available log file here.

   Note that there is only one println version, so the line feed calls need to be separate from data.

   Originally these were used for every write to the log file, but now that printf is available is
   has replaced the *Both calls in some key locations.  This mixed usage is not ideal, but printf
   is both faster and (once familiar) easier to read.
*/
void printBoth(const char* str) {
  if (logFile) logFile.print(str);
  Serial.print(str);
}
void printBoth(unsigned int d) {
  if (logFile) logFile.print(d);
  Serial.print(d);
}
void printBoth(int d) {
  if (logFile) logFile.print(d);
  Serial.print(d);
}
void printBoth(double d, int places) {
  if (logFile) logFile.print(d, places);
  Serial.print(d, places);
}
void printlnBoth() {
  if (logFile) logFile.println();
  Serial.println();
}
//void printBoth(byte d) {
//  if (logFile) logFile.print(d);
//  Serial.print(d);
//}
//void printBoth(unsigned long d) {
//  if (logFile) logFile.print(d);
//  Serial.print(d);
//}
//void printBoth(long d) {
//  if (logFile) logFile.print(d);
//  Serial.print(d);
//}
//void printBoth(double d) {
//  if (logFile) logFile.print(d);
//  Serial.print(d);
//}

//void printBoth(String str) {
//  if (logFile) logFile.print(str);
//  Serial.print(str);
//}
//void printBoth(char c) {
//  if (logFile) logFile.print(c);
//  Serial.print(c);
//}
//// Format time as decimal (for example)
//void printBoth(uint8_t n, int f) {
//  if (logFile) logFile.print(n, f);
//  Serial.print(n, f);
//}
