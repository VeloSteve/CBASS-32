/********************************************************
   CBASS Control Software
   This software uses a PID controller to switch heating and
   cooling devices.  These control the water temperature in
   small aquaria used for thermal tolerance experiments on coral.

   We use "time proportioning
   control"  Tt's essentially a really slow version of PWM.
   First we decide on a window size (5000mS say.) We then
   set the pid to adjust its controlOutput between 0 and that window
   size.  Lastly, we add some logic that translates the PID
   controlOutput into "Relay On Time" with the remainder of the
   window being "Relay Off Time".

   Beginning in 2023 this version of the sketch is designed to support
   "CBASS-32", a version of CBASS based on the Arduino Nano ESP32 product.
   The Nano provides dual processor cores at a faster clock rate, more memory,
   and built-in WiFi and Bluetooth.  It is smaller and no more expensive.
   The only real drawback is that it has fewer I/O pins, so some work
   is needed to control the relay outputs.

   TODO: (remove this when done)
   - Verify that the display and relays (all 16 outputs) work as expected
   - Ensure that WiFi and primary operations can not interfere with
     each other, for example by both attempting to access the SD card
     at the same time.
   - Stress test, for example hold all 16 relays in the ON state at
     once on two actual power bars.  At the same time server temperature
     graphs to a couple of devices while accepting control commands
     from a third.

   This software, including the other files in the same directory
   are subject to copyright as described in LICENSE.txt in this
   directory.  Please read that file for terms.

   Other imported libraries may be subject to their own terms.

   The software is hosted at https://github.com/VeloSteve/CBASS-32
 ********************************************************/
// C++ standard library so we can have a vector of PIDs
#include <vector>

// SD card library (do NOT use SD.h!)
#include <SdFat.h>
// Adafruit TFT LCD Display
#include <Adafruit_ILI9341.h>
// PID Library
#include <PID_v1.h>
//#include <PID_AutoTune_v0.h>

// Libraries for the DS18B20 Temperature Sensor
#include <OneWire.h>
#include <DallasTemperature.h>
// Real time clock
#include <RTClib.h>
// SPIFFS internal file system on ESP32
#include "SPIFFS.h"
#define FORMAT_SPIFFS_IF_FAILED true

// CBASS settings and constants.
#include "Settings.h"

// Web server
#include <ESPAsyncWebSrv.h>
#include "WebPieces.h"

// Add a watchdog timer so the system restarts if it somehow hangs.  This could prevent a fire in extreme cases!
#include <esp_task_wdt.h>
#define WDT_TIMEOUT 28  // How long to wait before rebooting in case of trouble (seconds).

/* Arduino IDE auto-generates function prototypes, but often fails!  When the
   compiler says something like
      "error: 'startDisplay' was not declared in this scope"
  put the prototype here.  Typically it is just the first line of the function
  definition with " {" replaced by ";".
  To be safe, put every function used in this file into this list.
 */
void startDisplay();
void SDinit();
void clockInit();
void checkTime();
void RelaysInit();
IPAddress connectWiFi();
void readRampPlan();
void rampOffsets();
void getCurrentTargets();
void PIDinit();
void applyTargets();
void ShowRampInfo();
void sensorsInit();
String gettime();
void relayTest();
void getTemperatures();
void saveGraphTemps();
void updateRelays();
void SerialReceive();
void SerialSend();
void displayTemperatureStatusBold();
void printLogHeader();
void printBoth(const char *str);
void printBoth(unsigned int d);
void printBoth(int d);
void printBoth(double d, int places);
void printlnBoth();
String getdate();
void fatalError(const __FlashStringHelper *msg);
void nonfatalError(const __FlashStringHelper *msg);
void defineWebCallbacks();
void checkSD(char* txt);
void setupMessages();
void pauseLogging(boolean a);
DataPoint makeDataPoint(unsigned long timestamp, DateTime t, double targ[], double a[]);
String dataPointToJSON(DataPoint p);
void dataPointPrint(DataPoint p);

// ===== Global variables are defined below. =====
const int port = 80;
AsyncWebServer server(port);
IPAddress myIP;

// The TFT display uses SPI communication (not I2C), shared with the SD card.
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);  // The CS and DC pins.

// The Real Time Clock is now DS3231, if there is trouble with older hardware, try RTC_DS1307.
RTC_DS3231  rtc;
DateTime  t;

// A file for logging the data.
File32 logFile;
boolean logPaused = false;
unsigned long startPause = 0;


// Storage for the characters of keyword while reading Settings.ini.
// Now expanded from 16 bytes to 128 so it can be used for longer 
// messages in other places.
const byte BUFMAX = 128; // INTERP is the longest keyword for now.
char iniBuffer[BUFMAX];

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(SENSOR_PIN);
// Temperature sensors
DallasTemperature sensors(&oneWire);
DeviceAddress thermometer[NT];

//Define Variables we'll Need
// Ramp plan
unsigned int rampMinutes[MAX_RAMP_STEPS];  // Time for each ramp step.
int rampHundredths[NT][MAX_RAMP_STEPS];  // Temperatures in 1/100 degree, because is half the storage of a float.
short rampSteps = 0;  // Actual defined steps, <= MAX_RAMP_STEPS
boolean interpolateT = true; // If true, interpolate between ramp points, otherwise step.
boolean relativeStart = false;  // Start from midnight (default) or a specified time.
unsigned int relativeStartTime;  // Start time in minutes from midnight
#ifdef COLDWATER
// Light plan
unsigned int lightMinutes[MAX_LIGHT_STEPS];
byte lightStatus[MAX_LIGHT_STEPS];
short lightSteps = 0;
short lightPos = 0; // Current light state
char lightStateStr[NT][4]; // = {"LLL", "LLL", "LLL", "LLL"}; // [number of entries][characters + null terminator]
#endif // COLDWATER

//Temperature Variables
// Note that "controlOutput" was formerly "tempOutput", but it is not in temperature units, so the name was misleading.
double tempT[NT];
double setPoint[NT], tempInput[NT], controlOutput[NT], correction[NT];
double chillOffset;
unsigned int i;  // General use

// Range of values used [-TPCwindow, TPCwindow] by the PID algorithm.
const int TPCwindow=10000;

// Time Windows: Update LCD 2/sec; Serial (logging) 1/sec, Ramp Status 1/sec
const unsigned int LCDwindow = 1000;
const unsigned int SERIALwindow = 15000;  // Default 1000; how often to log.
unsigned int GRAPHwindow = 5000;

const unsigned int serialHeaderPeriod = 10000; // Print header after this many lines.  Not useful for automated use.
unsigned int SerialOutCount = serialHeaderPeriod + 1;  // Print at the top of any new log.

// Display Conversion Strings
char setPointStr[5];  // Was an array of [NT][5], but we only need one at a time.
char tempInputStr[5];
char RelayStateStr[NT][4]; // [number of entries][characters + null terminator]
// char RelayStateStr[][4] = {"OFF", "OFF", "OFF", "OFF"}; // [number of entries][characters + null terminator]

//Specify the links and initial tuning parameters
// Perhaps with Autotune these would need to be variable, but as
// it stands there is no reason to copy the defined constants into a double.
// double kp = KP, ki = KI, kd = KD; //kp=350,ki= 300,kd=50;

// A vector of PID Controllers which will be instantiated in setup().
std::vector<PID> pids;
// A vector of DataPoints for graphing.
const int maxGraphPoints = (int)6*3600/(GRAPHwindow/1000); // Allow up to 6 hours of points!
std::vector<DataPoint> graphPoints;

// Formerly "printDate" No spaces or commas.  Otherwise just something that gets logged.
String logLabel = "CBASS-32";

//TimeKeepers
unsigned long now_ms = millis(),SERIALt, LCDt, GRAPHt;
String bootTime;

/////////////////////////////////////////////
//     SETUP                               //
/////////////////////////////////////////////
void setup()
{
  for (i=0; i<NT; i++) {
    // Instantiate a PID on each pass, using the given arguments.  Append it to the vector
    pids.emplace_back(PID(&tempInput[i], &controlOutput[i], &setPoint[i], KP, KI, KD, DIRECT));
  }

  // Learning point: the sketch will not load of the line below is active, even though
  // it compiles with no trouble!  Fortunately, this line doesn't seem necessary.
  //graphPoints.reserve(maxGraphPoints);

  // Start "reset if hung" watchdog timer. 8 seconds is the longest available interval.
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);

  // ***** INITALIZE OUTPUT *****`
  Serial.begin(38400);    // 9600 traditionally. 38400 saves a bit of time
  delay(5000);            // Serial does not initialize properly without a delay.
  esp_task_wdt_reset();

  startDisplay();             // TFT display
  Serial.println("\n===== Booting CBASS-32 45=====");
  Serial.printf("Running on core %d.\n", xPortGetCoreID());
  tftMessage("    Less wiring,\n    more science!");

  clockInit();  // Keep this before any use of the clock for logging or ramps.
  bootTime = gettime();

  // Start the filesystem for SD card access.
  tftMessage("Starting file systems.");
  SDinit();                   // SD card


  // This starts the in-memory filesystem used for web files.
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  // Start WiFi and the web server
  sprintf(iniBuffer, "Connecting WiFi to\n %s.", ssid);
  tftMessage(iniBuffer);
  myIP = connectWiFi();
  esp_task_wdt_reset();
  tftMessage("Defining web pages");
  defineWebCallbacks();
  server.begin();

  checkTime();  // Sets global t and we save the start time.
  delay(2000);
  esp_task_wdt_reset();

  readRampPlan();
  #ifdef COLDWATER
    readLightPlan();
  #endif
  esp_task_wdt_reset();
  rampOffsets();  // This does not need repeating in the main loop.

  // Get the target temperatures as will be done in the loop.
  getCurrentTargets();
  applyTargets();
  ShowRampInfo();

  RelaysInit();

  PIDinit();

  sensorsInit();
  getTemperatures(); // Not needed for control until the loop() starts, but the web server is already active.

  esp_task_wdt_reset();

  // Some text to the display and Serial monitor.
  setupMessages();

  // Do this after the data setup steps, which are quicker, so web pages have content before waiting for the relay tests.
  relayTest();
  // relayShow();  // Just for fun!  Lots of relay switching.
  Serial.println();

  // Clear the LCD screen before loop() because we may not fully clear it in the loop.
  tft.fillScreen(BLACK);

  // These timers use cumulative time, so start them at a current time, rather than zero.
  // Subtract the repeat interval so they activate on the first pass.
  SERIALt = millis() - SERIALwindow;
  LCDt = millis() - LCDwindow;
  GRAPHt = millis() - GRAPHwindow;
}

/**
 * This loop checks temperatures and updates the heater and chiller states.  As of 21 Apr 2022 it takes about
 * 1055 ms for the quickest loops.  When timed activities such as getting new temperature targets
 * and printing the new values trigger, this rises to about 1235 ms.
 * On this same date, the default triggers are 60000 ms for temperature target updates, 1000 ms for serial logging,
 * and 500 ms for LCD display updates.  When the latter two are smaller than the actual loop time, they simply
 * run every time.
 * With the ESP32 processor the fast loops take only about 7 ms!  With temperature checks 294 ms and with logging, 304 ms.
 * TODO: update timings for the ESP32.  There will be big changes, probably not equally distributed.

 * Within each loop most of the time is in 4 areas: 479 ms in requestTemperatures, 193 ms getting and adjusting
 * temperatures, 199 ms logging (to serial monitor and SD card), 183 ms updating the LCD display.
 * On loops when target values are updated, 3 ms is spent doing that work  and 113 ms printing the result.
 *
 * This loop time is effective for thermal control.  Changes to the code which affect the loop time substatially
 * should be followed by physical testing of temperature histories.
 */
/////////////////////////////////////////////
//     LOOP                                //
/////////////////////////////////////////////
unsigned long timer24h = 0;
void loop()
{
  // ***** Time Keeping *****
  now_ms = millis();
  t = rtc.now();  // Do this every loop so called functions don't have to.

  // ***** INPUT FROM TEMPERATURE SENSORS *****
  getTemperatures();
  // Update temperature targets.  This originally had a delay, but it takes almost no time.
  // checkTime(); // Print time of day, redundant with logging.
  getCurrentTargets();
  applyTargets();
  //ShowRampInfo(); // To display on serial monitor.
#ifdef COLDWATER
  // Similar to getCurrentTargets/applyTargets, but for lighting.
  getLightState();
#endif


  if (now_ms - GRAPHt > GRAPHwindow) {
    if (graphPoints.size() >= maxGraphPoints) graphPoints.erase(graphPoints.begin());
    graphPoints.emplace_back(makeDataPoint(now_ms, t, setPoint, tempT));
    //Serial.printf("graphPoints size is now %d elements. Est. total bytes = %d Stack high water = %d.\n", 
    //    graphPoints.size(), graphPoints.size()*sizeof(DataPoint) + sizeof(graphPoints), uxTaskGetStackHighWaterMark( NULL ));
    GRAPHt += GRAPHwindow;
  }


  // ***** UPDATE PIDs *****
  for (i=0; i<NT; i++) pids[i].Compute();

  //***** UPDATE RELAY STATE for TIME PROPORTIONAL CONTROL *****
  // For ESP we may want to slow this down if relays switch too often.
  updateRelays();

  //***** UPDATE SERIAL MONITOR AND LOG *****
  if (now_ms - SERIALt > SERIALwindow) {
    // Logging is skipped during certain web operations.
    if (!logPaused) {
      SerialReceive();
      SerialSend();
      SERIALt += SERIALwindow;
    } else {
      // Adjust timing so we log again promptly, but don't add extra "make up" log lines.
      SERIALt = millis() - SERIALwindow;
      tftPauseWarning(true);
    }
  }

  //***** UPDATE LCD *****
  if (now_ms - LCDt > LCDwindow)
  {
    displayTemperatureStatusBold();
    LCDt += LCDwindow;
  }
  esp_task_wdt_reset();
}

void SerialSend()
{
  //WARNING: the last argument to open() must be _WRITE for Mega, but _APPEND for ESP32. New: O_WRONLY | O_CREAT for SdFat.
  logFile = SDF.open("/LOG.txt",  O_WRONLY | O_CREAT | O_APPEND);
  //uint64_t lfs = logFile.size();
  if (!logFile) {
    Serial.println("ERROR: failed to write log file.");
    checkSD("After failing to open LOG.txt"); // debug
    fatalError(F("Unable to open LOG.txt for writing."));
  }
  // more reliable than O_APPEND alone ?
  logFile.seekEnd(0);
  if (SerialOutCount > serialHeaderPeriod) {
    printLogHeader();
    SerialOutCount = 0;
  }
  // General data items not tied to a specific tank:
  Serial.printf("%s,%s,%d,%d,%d,%d,", logLabel, getdate(), now_ms, t.hour(), t.minute(), t.second());
  logFile.printf("%s,%s,%d,%d,%d,%d,", logLabel, getdate(), now_ms, t.hour(), t.minute(), t.second());
  // Per-tank items
  for (i=0; i<NT; i++) {
    Serial.printf("%.2f,%.2f,%.2f,%.1f,%s,", setPoint[i], tempInput[i], tempT[i], controlOutput[i], RelayStateStr[i]);
    logFile.printf("%.2f,%.2f,%.2f,%.1f,%s,", setPoint[i], tempInput[i], tempT[i], controlOutput[i], RelayStateStr[i]);
  }
  printlnBoth();
  Serial.flush();
  //logFile.sync(); // Same as flush() on Arduino, close covers this.
  //Serial.printf("Log size increased from %llu to %llu.\n", lfs, (uint64_t)logFile.size());
  logFile.close();
  //Serial.println("Closed log file.");
  SerialOutCount++;
}

/**
 * Typically this will be called once from setup() with no logFile open
 * and then periodically during loop().  Having the code here keeps
 * the two versions in sync.
 * This assumes 5 data items per tank after the date and time information.
 *
 * This originally used printBoth for everything.  Not that it really matters
 * but this printf approach is about 4 times faster.  Unfortunately it means
 * either a loop with "ifs" inside for the log file, or one "if" and two loops.
 */
void printLogHeader() {
  Serial.print(F("LogLabel,Date,N_ms,Th,Tm,Ts,"));
  if (logFile) {
    logFile.print(F("LogLabel,Date,N_ms,Th,Tm,Ts,"));

    // Normally loop from 0, but here we want tank numbers.
    for (int i=1; i<=NT; i++) {
      logFile.printf("T%dSP,T%dinT,TempT%d,T%doutT,T%dRelayState", i, i, i, i, i);
      if (i < NT) logFile.print(",");
    }
    logFile.println();
  }
  for (int i=1; i<=NT; i++) {
    Serial.printf("T%dSP,T%dinT,TempT%d,T%doutT,T%dRelayState", i, i, i, i, i);
    if (i < NT) Serial.print(",");
  }
  Serial.println();
}

void SerialReceive()
{
  if (Serial.available())
  {
    Serial.read();
    Serial.flush();
  }
}

/**
 * Messages displayed on TFT and Serial monitor during startup.
 */
void setupMessages() {
  tft.fillScreen(BLACK);
  tft.setTextSize(3);
  tft.setCursor(0, 0);  // remember that the original LCD counts characters.  This counts pixels.
  tft.print("LogLabel is:");
  tft.setCursor(0, LINEHEIGHT3);
  tft.print(logLabel);
  Serial.println("LogLabel is:");
  Serial.println(logLabel);
  // Show RTC time so we know it's working.
  tft.setCursor(0, LINEHEIGHT*4);
  tft.print("Time: "); tft.print(gettime());
  delay(1000);
  esp_task_wdt_reset();

  tft.setCursor(0, LINEHEIGHT3*5);
  tft.print(" 5s Pause...");
  Serial.printf("\n\nInitialization sequence.\n");
  Serial.println();
  Serial.println(" 5s Pause...");
  delay(RELAY_PAUSE);
  esp_task_wdt_reset();
  Serial.println();
}

/**
 * Tell the sensors to get fresh temperatures and then store them after
 * applying any corrections we may have in place.
 */
void getTemperatures() {
  sensors.requestTemperatures();

  // Get temperatures for each tank by address so we have a definite
  // association between tanks, sensors, and addresses.
  for (i=0; i<NT; i++) {
    tempT[i] = sensors.getTempC(thermometer[i]) - correction[i];
    if (0.0 < tempT[i] && tempT[i] < 80.0)  tempInput[i] = tempT[i];
  }
  /*  Original approach
  for (i=0; i<NT; i++) {
    tempT[i] = sensors.getTempCByIndex(i) - correction[i];
    if (0.0 < tempT[i] && tempT[i] < 80.0)  tempInput[i] = tempT[i];
  }
   */
}

void ShowRampInfo() {
  for (int i=0; i<NT; i++) {
    Serial.printf("Tank %d target: %7.2f\n", (i+1), setPoint[i]);
  }
}

