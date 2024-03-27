const char monthsOfTheYear[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};


// Get the current date as a printable string
String getdate() {
  return getdate(rtc.now());
}

// Show the provided Date time as a date in a String.
String getdate(DateTime t) {
   String dateStr =  String(t.year(), DEC) + "_" + String(monthsOfTheYear[t.month()-1])  + "_" + String(t.day(), DEC);
   return dateStr;
}

// Get the current time of day as a String.
// The format is HH:MM:SS in 24-hour time.
String gettime() {
  return gettime(rtc.now());
}

// Format the provided DateTime as time of day in a String.
// The format is HH:MM:SS in 24-hour time.
String gettime(DateTime t) {
   String tStr =  String(t.hour(), DEC) + ":" 
       + String((t.minute() < 10) ? "0" : "")  // Zero-pad minutes
       + String(t.minute(), DEC)  + ":" 
       + String((t.second() < 10) ? "0" : "")  // Zero-pad seconds
       + String(t.second(), DEC);
   return tStr;
}

/**
 * Set the time from a C string like
 * 27/01/2024 15:38:46
 * If it's not in that format or the time
 * doesn't make sense, do nothing and return
 * false.
 * "plus" seconds are added to the time, allowing for a lag
 * between click and clock set.  Default is 1 second.
 */
bool settime(const char* dateTime, int plus = 1) {
  int day, month, year, hh, mm, ss;
  // Pull all the numbers into variables.
  sscanf(dateTime, "%d/%d/%d %d:%d:%d", &day, &month, &year, &hh, &mm, &ss);
  // Check that all are in range
  if (day >  0 && day < 32 && month > 0 && month < 13 && year > 2020 && year < 2050
               && hh >=0 && hh < 24 && mm >=0 && mm < 60 && ss >= 0 && ss < 60) {
     rtc.adjust(DateTime(year, month, day, hh, mm, ss) + TimeSpan(0, 0, 0, plus));
     Serial.print("CBASS time is now "); Serial.print(gettime()); Serial.print(" on "); Serial.println(getdate());
  } else {
    Serial.printf("Failed to extract a date and time from $s\n", dateTime);
    return false;
  }
  return true;
}


void checkTime()
{
  t = rtc.now(); // Use global t so its available for logging and such.
  Serial.print("Time: ");
  Serial.print(t.hour(), DEC);
  Serial.print(":");
  if (t.minute() < 10) Serial.print(0);
  Serial.println(t.minute(), DEC);
  return;
}



void clockInit() {
  while (!rtc.begin()) {
    Serial.println("Couldn't start RTC");
    delay(2000);
  }
  Serial.print("Started RTC at ");
  checkTime();
}

//void rtc_to_24hourMode() {
//  uint8_t oldVal = rtc.read_register(DS3231_STATUSREG);
//}
