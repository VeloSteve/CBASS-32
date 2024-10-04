#define NT 4 // The number of tanks supported.  CBASS-32 supports up to 8, though 4 is standard.  With light control NT <= 5.
#define RELAY_PAUSE 500 // Milliseconds to wait between some startup steps - standard is 5000, but a small number is handy when testing other things.

// The original CBASS from the Barshis lab uses Iceprobe chillers.
// The Logan lab modifications use moving cold water, and adds light controls.
// This definition had other purposes, but for not it only affects one offset value,
// which should be checked.
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
    const char *password = "Monterey";  // Minimum length 7 characters!
#else
    // Credentials of an existing network.
    const char *ssid = "your wifi";
    const char *password = "passsword here";

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

// CBASS-32 uses up to 16 relays.  Unlike CBASS-R, there is no longer a custom board for
// direct control of 12V water pumps for cold-water systems.  Those systems will use pumps
// powered from the power bar, typically USB-powered pumps.
#define SHIFT_PINS 16 // How many shift register pins to address.  Allows for future expansion.

// Please see the documentation for why the bits are in this arbitrary-looking order.
// Remember that DB9_UP_1 controls tanks 1-4, and is controlled by the lowest-value bits
// of shiftRegBits.
const byte HeaterRelay[] = {3, 4, 6, 7, 10, 12, 14, 8}; 
const byte ChillRelay[] = {5, 1, 2, 0, 15, 13, 11, 9};   
// Lights are also controlled by the shift register, using some of the same bits.  This means that
// systems with lights may use no more than 5 tanks, since 16 switched outlets are available.  Lights
// will be controlled by the bits "left over" beyond tank 5.
const byte LightRelay[] = {12, 14, 8, 13, 11, 9};

// The shift registers are controlled by these pins
#define LATCH_PIN D8    // RCLK
#define DATA_PIN D7   // SER
#define CLOCK_PIN D9  // SRCLK

// Other Arduino pins
#define SENSOR_PIN 21  // Sensor pin must be the ESP GPIO number, NOT the Arduino number like the rest!
#define TFT_CS D4
#define TFT_DC D5
#define SPI2_SCK D2
#define SPI2_COPI D3

// SD card.  Note that there is a second SD card on the back of the display, which we don't currently use.
// This is for the active one.
#define SD_CS  D6
// SdFat has more options than SD.h.  This gives a reasonable set of defaults.
#define SD_FAT_TYPE 1             // 1 implies FAT16/FAT32
#define SPI_CLOCK SD_SCK_MHZ(50)  // 50 is the max for an SD.  Set lower if there are problems.
#define SD_CONFIG SdSpiConfig(SD_CS, SHARED_SPI, SPI_CLOCK)

// For fat type 1, use
SdFat32 SDF;  // Example files use lower-case sd, but this fits old CBASS code.
//File32 file;  Files should be declared like this (not with just File).
// Normally use "#undef" for ALLOW_UPLOADS because it is dangerous.  Only enable
// with #define during development when you have to ability to remove and reconfigure
// the SD card in case of a mistake.
#define ALLOW_UPLOADS

// Display constants
#define TFT_WIDTH 320
#define TFT_HEIGHT 240
#define TFT_LAND 3  // Rotation for normal text  1 = SPI pins at top, 3 = SPI at bottom.
#define TFT_PORT 2  // Rotation for y axis label, typically 1 less than TFT_LAND
#define LINEHEIGHT 19 // Pixel height of size 2 text is 14.  Add 1 or more for legibility.
#define LINEHEIGHT3 28 // Pixel height of size 3 text is ??.  Add 1 or more for legibility.
// From GFX docs: "Desired text size. 1 is default 6x8, 2 is 12x16, 3 is 18x24, etc"



// Temperature sensor addresses.  If we know how they are joined into sets
// of 4 we can assign them to tanks in groups.  We do not expect the addresses
// to be in any particular order across sets, but they are discovered in a consistent
// order.
// This array can grow to hold the addresses of every sensor set you have access to.
// The sketch uses it as a reference for grouping sensors at run time.
//
// The sketch will print the addresses of any unrecogized sensors in the format
// needed below for easy cut-and-paste updates.
//
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


