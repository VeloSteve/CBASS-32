#include <SdFat.h>
#define NT 6 // The number of tanks supported.  CBASS-32 supports up to 8, though 4 is standard.
#define SD_CS  D5
#define SD_FAT_TYPE 1             // 1 implies FAT16/FAT32
#define SPI_CLOCK SD_SCK_MHZ(50)  // 50 is the max for an SD.  Set lower if there are problems.
#define SD_CONFIG SdSpiConfig(SD_CS, SHARED_SPI, SPI_CLOCK)

SdFat32 SDF;
File32 settingsFile;

const short MAX_RAMP_STEPS = 20;

short rampSteps = 0;  // Actual defined steps, <= MAX_RAMP_STEPS
boolean interpolateT = true; // If true, interpolate between ramp points, otherwise step.
boolean relativeStart = false;  // Start from midnight (default) or a specified time.
unsigned int relativeStartTime;  // Start time in minutes from midnight
unsigned int rampMinutes[MAX_RAMP_STEPS];
int rampHundredths[NT][MAX_RAMP_STEPS]; 

/* In the "real" program this does more... */
void fatalError(const __FlashStringHelper *msg) {
  Serial.print(F("Fatal error: "));
  Serial.println(msg);
  while (1);
}


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

void readRampPlanNEW() {
  int maxLine = 128;
  char lineBuffer[maxLine+1];
  char *lb = lineBuffer;
  int nRead, nParse, pos, hh, mm;
  int rampStamps = 0;
  double temp;
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
    if (nRead == 0 || lineBuffer[0] == '/') {
      ;  // an empty line or comment, move on.
    } else if (!strncmp(lineBuffer, "START", 5)) {
      // The line should contain a start time after START
      nParse = sscanf(lineBuffer + 5, "%d:%d", &hh, &mm);
      if (nParse != 2) {
        fatalError(F("Invalid START time in Settings.ini"));
      } else if (hh < 0 || hh > 23) {
        fatalError(F("START time hour must be from 0 to 23."));
      } else if (mm < 0 || hh > 59) {
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
      int tank = 0;
      while (token != NULL) {
        if (tank < NT) {
          sscanf(token, "%f", &temp);
          rampHundredths[tank++][rampSteps] = round(temp * 100);
        } else {
          Serial.printf("WARNING: Ignoring extra tank %d in Settings.ini with NT = %d\n", tank + 1, NT);
        }
        token = strtok(NULL, " \t");
      }
      if (tank < NT) {
        // If the user specified NT tanks and the are less than NT in the plan, an experiment
        // could be ruined.  Consider this a fatal error.  Try to keep the web server
        // running so this can be addressed without pulling the card.
        Serial.printf("Line contained %d tanks, but CBASS is configured for %d\n", tank, NT);
        fatalError(F("Settings.ini has fewer temperatures than you have tanks.  Edit (/RampPlan), reset (/ResetRampPlan), or adust NT."));
      }
    } else {
      Serial.printf("Bad line: >%s<\n", lineBuffer);
      fatalError(F("Settings.ini has an illegal line.  Edit (/RampPlan) or reset (/ResetRampPlan)."));
    }
  }
}

void setup() {
  Serial.begin(38400);    // 9600 traditionally. 38400 saves a bit of time
  delay(5000);  
  Serial.println("=== Read Settings.ini 3 ===");
  SDinit();                   // SD card
  readRampPlanNEW();
  Serial.println("Ramp plan reading is complete.");
}

void loop() {
  // put your main code here, to run repeatedly:

}
