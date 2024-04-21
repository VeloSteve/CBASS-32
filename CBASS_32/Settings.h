#define NT 4 // The number of tanks supported.  CBASS-32 supports up to 8, though 4 is standard.
#define RELAY_PAUSE 5000 // Milliseconds to wait between some startup steps - standard is 5000, but a small number is handy when testing other things.

// The original CBASS from the Barshis lab uses Iceprobe chillers.
// The Logan lab modifications use moving cold water, and add light controls.
#undef COLDWATER  //#define for liquid cooling and lights.  Omit or #undef for the original behavior.

// WiFi can connect to an existing network and become a server (station) at the assigned
// address or it can create its own network and act as its own WiFi access point.
#define WIFIAP 1
#define WIFISTATION 2
#define WIFIMODE WIFISTATION    // Choose one of the two above

#define MAGICWORD "Auckland"

// For the chosen WiFi mode, provide the name of the access point and a password.
#if WIFIMODE == WIFIAP
    // Credentials for a network CBASS-32 will create.
    const char *ssid = "CBASS_AP";
    const char *password = "1234abcd";
#else
    // Credentials of an existing network.
    const char *ssid = "Anthozoa";
    const char *password = "16391727";
    //const char *ssid = "BirdDino";
    //const char *password = "1Milpitas";

#endif

// ***** Temp Program Inputs *****
double RAMP_START_TEMP[NT];
const short MAX_RAMP_STEPS = 20; // Could be as low as 7, 24*12+1 allows every 5 minutes for a day, with endpoints.

#ifdef COLDWATER
#define CHILLER_OFFSET 0.0
#else
#define CHILLER_OFFSET 0.20
#endif
// Initialize 8 values even though often only 4 will be used.
const double TANK_TEMP_CORRECTION[] = {0, 0, 0, 0, 0, 0, 0, 0}; // Is a temperature correction for the temp sensor, the program subtracts this from the temp readout e.g. if the sensor reads low, this should be a negative number

// ***** PID TUNING CONSTANTS ****
#define KP 2000//5000//600 //IN FIELD - Chillers had higher lag, so I adjusted the TPCwindow (now deprecated) and KP to 20 secs, kept all proportional
#define KI 10//KP/100//27417.54//240 // March 20 IN FIELD - with 1 deg steps, no momentum to take past P control, so doubled I. (10->40)
#define KD 1000//40  //

// Most relays are on when sent "1", so that is the default.  Switch the 1 and 0 if you
// have "normally on" relays.
#define RELAY_ON 1
#define RELAY_OFF 0

// Arduino pin numbers for relays for tanks 1 to 4.
// The integers correspond to the CBASS-R v 1.2 schematic.
const int HeaterRelay[] = {A0, A1, D6, D2}; // A0=D17, A1=D18

// Tanks 5-8, if they exist, are controlled by a shift register, so instead
// of pin numbers we associated tanks with bits 1-8.  Be aware that locations
// 0-3 in the arrays correspond to tanks 5-8. 
const byte HeaterRelayShift[] = {2, 4, 5, 0};  // These are the bit location in the control
const byte ChillRelayShift[] = {7, 6, 3, 1};   // byte.  The pin numbers on the DB9 will be 1-4 and 6-9.
// count 12 (expect chill) gives heater 3
// count 13 (expect 3) gives heater 1

#ifdef COLDWATER
// TODO: replace relays 9-16 with shift register commands.
const int ChillRelay[] = {0, 1, 2, 3};   // Bits, not pins!
const int LightRelay[] = {A7, A6, A3, A2}; 
const short MAX_LIGHT_STEPS = 4; // typically we have only 2 for sunrise and sunset
#else
const int ChillRelay[] = {A7, A6, A3, A2}; // Digital synonyms, A7 = D24, A6 = D23, A3 = D20 A2 = D19.
#endif

/* 
 *  Old version kept for DB9 pins and colors:
#define T1HeaterRelay 17  // T1 Heat DB9 pin 9 black wire Arduino Digital I/O pin number 17
#define T1ChillRelay  22  // T1 Chill DB9 pin 1 brown wire Arduino Digital I/O pin number 22
#define T2HeaterRelay 15  // T2 Heat DB9 pin 8 white wire Arduino Digital I/O pin number 15
#define T2ChillRelay  24 // T2 Heat DB9 pin 2 red wire Arduino Digital I/O pin number 24
#define T3HeaterRelay 16  // T3 Heat DB9 pin 7 purple wire Arduino Digital I/O pin number 16
#define T3ChillRelay  23  // T3 Chill DB9 pin 3 orange Arduino Digital I/O pin number 23
#define T4HeaterRelay 25 // T4 Heat DB9 pin 6 blue wire Arduino Digital I/O pin number 25
#define T4ChillRelay  14  // T4 Chill DB9 pin 4 yellow wire Arduino Digital I/O pin number 14
 */

// The shift register for the second DB9 connector (closest to the SD card) is controlled
// by these pins
#define LATCH_PIN D8    // RCLK
#define DATA_PIN D7   // SER
#define CLOCK_PIN D9  // SRCLK

// Other Arduino pins
#define SENSOR_PIN 21  // Sensor pin must be the ESP GPIO number, NOT the Arduino number like the rest!
#define TFT_CS   D4
#define TFT_DC   D3
// SD card.  Note that there is a second SD card on the back of the display, which we don't currently use.
// This is for the active one.
// unconnected #define SD_DETECT  10
#define SD_CS  D5
// SdFat has more options than SD.h.  This gives a reasonable set of defaults.
#define SD_FAT_TYPE 1             // 1 implies FAT16/FAT32
#define SPI_CLOCK SD_SCK_MHZ(50)  // 50 is the max for an SD.  Set lower if there are problems.
#define SD_CONFIG SdSpiConfig(SD_CS, SHARED_SPI, SPI_CLOCK)

// For fat type 1, use
SdFat32 SDF;  // Example files use lower-case sd, but this fits old CBASS code.
//File32 file;  Files should be declared like this (not with just File).
// Normally use "#undef" for ALLOW_UPLOADS because it is dangerous.  Only enable
// with #define during develpment when you have to ability to remove and reconfigure
// the SD card in case of a mistake.
#undef ALLOW_UPLOADS

// Display constants
#define TFT_WIDTH 320
#define TFT_HEIGHT 240
#define TFT_LAND 3  // Rotation for normal text  1 = SPI pins at top, 3 = SPI at bottom.
#define TFT_PORT 2  // Rotation for y axis label, typically 1 less than TFT_LAND
#define LINEHEIGHT 19 // Pixel height of size 2 text is 14.  Add 1 or more for legibility.
#define LINEHEIGHT3 28 // Pixel height of size 3 text is ??.  Add 1 or more for legibility.
// From GFX docs: "Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc"

// Colors are RGB, but 16 bits, not 24, allocated as 5, 6, and 5 bits for the 3 channels.
// For example, a light blue could be 0x1F1FFF in RGB, but 0x1F3A in 16-bit form. To convert
// typical RBG given as a,b,c, use 2048*a*31/255 + 32*b*63/255 + c*31/255
#define DARKGREEN 0x05ED  // Normal green is too pale on white. 
#define DARKERGREEN 0x03F4  // Normal green is too pale on white. 
#define LIGHTBLUE 0x1F3A  // Normal blue is too dark on black.
// Easier names for some of the colors in the ILI9341 header file.
#define BLACK   ILI9341_BLACK
#define BLUE    ILI9341_BLUE
#define RED     ILI9341_RED
#define GREEN   ILI9341_GREEN
#define CYAN    ILI9341_CYAN
#define MAGENTA ILI9341_MAGENTA
#define YELLOW  ILI9341_YELLOW
#define WHITE   ILI9341_WHITE

// Temperature sensor addresses.  If we know how they are joined into sets
// of 4 we can assign them to tanks in groups.  We do not expect the addresses
// to be in any particular order across sets, but they are discovered in a consistent
// order.
// This array can grow to hold the addresses of every sensor set you have access to.
// The sketch uses it as a reference for grouping sensors at run time.
// Array size is [number of supported sensor sets][maximum tanks per set][bytes per address]
const int knownAddressSets = 2;
char addressSets[knownAddressSets][4][8] = {
  {  // My first set.
    { 0x28,0xd0,0x0c,0x96,0xf0,0x01,0x3c,0x47 },
    { 0x28,0x08,0x16,0x96,0xf0,0x01,0x3c,0xe3 },
    { 0x28,0x41,0xe7,0x76,0xe0,0x01,0x3c,0x68 },
    { 0x28,0x1f,0x49,0x96,0xf0,0x01,0x3c,0xc7 }
  },
  {  // Returned from Logan Lab loan
    { 0x28,0x74,0xc3,0x95,0xf0,0x01,0x3c,0x02 },
    { 0x28,0xb6,0xbb,0x95,0xf0,0x01,0x3c,0xa0 },
    { 0x28,0x95,0xd1,0x95,0xf0,0x01,0x3c,0xe3 },
    { 0x28,0x7d,0xa1,0x95,0xf0,0x01,0x3c,0x06 }
  }
};

// Store collected time and temperature information together.
// old style as sent to Tchart.html:
// {"NT":[4],"timeval":[54492],"CBASStod":["7:37:39"],"tempList":[19.8,20.1,19.8,20.6],"targetList":[24.0,24.0,24.0,24.0]}
struct DataPoint
{
  long unsigned int timestamp;
  DateTime time; 
  double target[NT];
  double actual[NT];
};

// Keep this since it may be nice in a display, but comment to save memory until we need it.
/*
const byte degree[8] = // define the degree symbol
{
  B00110,
  B01001,
  B01001,
  B00110,
  B00000,
  B00000,
  B00000,
  B00000
};
 */
