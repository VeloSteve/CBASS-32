#define NT 4
#include <RTClib.h>
#include <vector>


// Store collected time and temperature information together.
struct DataPoint
{
   DateTime time; 
   double target[NT];
   double actual[NT];
};

// These would be varying values in the applications.  Here
// they just fill space.
double setPoint[] = {21, 22, 23, 24};
double tempT[] = {22, 23, 24, 25};
DateTime t = DateTime("2024-04-05T10:56:59");

const int maxGraphPoints = (int)6*60*60/5; // Allow up to 6 hours of points ever 5 seconds.
std::vector<DataPoint> graphPoints;

int printAt = 1000;
int counter = 0;

DataPoint makeDataPoint(DateTime t, double targ[], double a[]) {
  DataPoint d;
  d.time = t;
  memcpy(&d.target, &targ[0], NT*sizeof(double));
  memcpy(&d.actual, &a[0], NT*sizeof(double));
  return d;
}

long unsigned int m;

void setup() {
  Serial.begin(115200);    // 9600 traditionally. 38400 saves a bit of time
  delay(5000);            // Serial does not initialize properly without a delay.
  // Set vector to max size in advance.
  //graphPoints.reserve(maxGraphPoints);
  Serial.printf("NR graphPoints has sizeof = %d, size() = %d, and max_size() = %d, capacity = %d\n",
      sizeof(graphPoints), graphPoints.size(), graphPoints.max_size(), graphPoints.capacity());
}

void loop() {
    // Note that erase() doesn't take a position. It requires the object you want to delete.
    if (graphPoints.size() >= maxGraphPoints) graphPoints.erase(graphPoints.begin());
    graphPoints.emplace_back(makeDataPoint(t, setPoint, tempT));
    if (counter >= printAt) {
      Serial.printf("graphPoints size is now %d elements. Est. total bytes = %d Stack high water = %d.  %d millis/loop\n", 
        graphPoints.size(), graphPoints.size()*sizeof(DataPoint) + sizeof(graphPoints), uxTaskGetStackHighWaterMark( NULL ), (int)((millis()-m)/printAt) );
      m = millis();
      counter = 0;
    } else {
      counter++;
    }

}

/* Results:
8 ms/pass, 6216 hight water after many loops (dropping!), total bytes 311052.
  vector, reserve size at start, erase first element on each pass after max size is reached.
  removing the reserve() call does not change the max_size() or any of the memory numbers!!!
  capacity() does change from 0 to 4320


graphPoints has sizeof = 12, size() = 0, and max_size() = 59652323
graphPoints size is now 1001 elements. Est. total bytes = 72084 Stack high water = 6604.  6 millis/loop
graphPoints size is now 2002 elements. Est. total bytes = 144156 Stack high water = 6524.  0 millis/loop
graphPoints size is now 3003 elements. Est. total bytes = 216228 Stack high water = 6524.  0 millis/loop
graphPoints size is now 4004 elements. Est. total bytes = 288300 Stack high water = 6524.  0 millis/loop
graphPoints size is now 4320 elements. Est. total bytes = 311052 Stack high water = 6524.  6 millis/loop
graphPoints size is now 4320 elements. Est. total bytes = 311052 Stack high water = 6524.  8 millis/loop
graphPoints size is now 4320 elements. Est. total bytes = 311052 Stack high water = 6524.  8 millis/loop
graphPoints size is now 4320 elements. Est. total bytes = 311052 Stack high water = 6296.  8 millis/loop
graphPoints size is now 4320 elements. Est. total bytes = 311052 Stack high water = 6296.  8 millis/loop


 */