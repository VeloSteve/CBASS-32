/**
 * This file contains three types of entries.
 * 1) Class definitions, for now only DataPoint.
 * 2) Constants the user will rarely or never change.
 * 3) Function predeclarations so the main *.ino file doesn't
 *    need a long list of functions the proprocessor fails
 *    to insert.
 */

struct DataPoint;  // pre-declare a struct used as a function argument below.

// Prototypes, typically just the first line of the function
//  definition with " {" replaced by ";".
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
String dataPointToJSON(DataPoint p);
void dataPointPrint(DataPoint p);

// Store collected time and temperature information together.
// old style as sent to Tchart.html:
// {"NT":[4],"timeval":[54492],"CBASStod":["7:37:39"],"tempList":[19.8,20.1,19.8,20.6],"targetList":[24.0,24.0,24.0,24.0]}
struct DataPoint
{
  long unsigned int timestamp;
  DateTime time; 
  double target[NT];
  double actual[NT];
    // A constructor so this can be built automatically by emplace_back
  DataPoint(long unsigned int t, DateTime dt, double tar[NT], double act[NT]) {
    timestamp = t;
    time = dt;
    //target = tar;
    memcpy(&target, &tar[0], NT*sizeof(double));
    memcpy(&actual, &act[0], NT*sizeof(double));
  } 
};

// Colors are RGB, but 16 bits, not 24, allocated as 5, 6, and 5 bits for the 3 channels.
// For example, a light blue could be 0x1F1FFF in RGB, but 0x1F3A in 16-bit form. To convert
// typical RBG given as a,b,c in decimal, use 2048*a*31/255 + 32*b*63/255 + c*31/255
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