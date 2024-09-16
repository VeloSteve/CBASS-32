#define NT 8 // The number of tanks supported.  CBASS-32 supports up to 8, though 4 is standard.  With light control NT <= 5.
#define RELAY_PAUSE 500 // Milliseconds to wait between some startup steps - standard is 5000, but a small number is handy when testing other things.

#undef HDEBUG

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

// All tanks are controlled by shift registers starting with CBASS-32 V2.0.
// for up to 16 tanks.
// Two shift registers are daisy-chained together.  The first is connected to the
// incoming data line and to DB9_UP_1, so it controls the first 4 tanks.  The
// second receives data as it shifts out of the first and is connected to DB9_DOWN_2,
// controlling the remaining tanks and/or lights.
// The incoming data is stored in an integer with the lowest SHIFT_PINS bits used to 
// control the relays and the rest set to zero.
// Bits are placed into the shift registers starting with the most significant bit first,
// so for example if the value is 1100 0000 0000 0001 (spaces added for readabilty)
// the last two pins (QG and QH) on the second register and the first pin (QA) on the second register will
// be enabled.  By the way, those bits printed in base 10 will be 1 + 32,768 + 65,536 = 93,305.
// The mapping is
// reg-   reg  pin    net   DB9 controlled by
// ister  pin  label        pin bit (0 = low, 15 = high)
// 1      15    QA    CH_4  1    0
// 1      1     QB    CH_2  3    1
// 1      2     QC    CH_3  2    2
// 1      3     QD    HT_1  6    3
// 1      4     QE    HT_2  7    4
// 1      5     QF    CH_1  4    5
// 1      6     QG    HT_3  8    6
// 1      7     QH    HT_4  9    7
// 2      15    QA    HT_8  1    8
// 2      1     QB    CH_8  3    9
// 2      2     QC    HT_5  2   10
// 2      3     QD    CH_7  6   11
// 2      4     QE    HT_6  7   12
// 2      5     QF    CH_6  4   13
// 2      6     QG    HT_7  8   14
// 2      7     QH    CH_5  9   15
// Column names:
// register = the shift register, 1 or 2  (labeled 1 and 0 on the board - oops)
// reg pin = the pin number on the shift register chip.
// pin label = the pin as labeled on the datasheet and schematic
// net = the connection name on the schematic.  HT = heat, CH = chill, number = tank number
// DB9 pin = the pin number on the DB9 connector
// bit = the bit location in the control value  (shiftRegBits)
//
// Remember that DB9_UP_1 controls tanks 1-4, and is controlled by the lowest-value bits.
const byte HeaterRelay[] = {3, 4, 6, 7, 10, 12, 14, 8}; 
const byte ChillRelay[] = {5, 1, 2, 0, 15, 13, 11, 9};   
// Lights are also controlled by the shift register, using some of the same bits.  This means that
// systems with lights may use no more than 5 tanks, since 16 switched outlets are available.  Lights
// will be controlled by the bits "left over" beyond tank 5.
const byte LightRelay[] = {12, 14, 8, 13, 11, 9};

// Example bit values with all heaters on:
// 0001101110101010
// numbers 1, 3, 5, 7, 8, 9, 11, 12 from right (0 based) - matches array above, but wrong relays are on!
// 0001101110101010
// 0000111101100011
//     HHHH-HH---HH
//     8743-21---65 expected
// H8, C8 are lit on 2nd bar.
// Just record what lights up
/*
1  c4
2  c2
4  c3
8  h1
16  h2
32  c1  
64  h3
128 h4
256 h8
512 c8
1024 h5
2048 c7
4096 h6
8192 c6
16384 h7
32768 c5

What about H7 and C5? or Bits 0 and 1?
 */

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
// unconnected #define SD_DETECT  10
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

