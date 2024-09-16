/*
 * Boards notes.
 * The latest Arduino ESP32 Board, 2.0.17 as of 11 Sep 2024 causes sensor detection to fail!!!
 * Back tests:
 * 2.0.10 - good
 * 2.0.12 - good
 * 2.0.13 - good
 * 2.0.17 retest - 1 good after first installation, the 4 bad after both power cycles and reset button. Also failed after reinstall.
 * These tests were done with just the OneWire example called DS18x20_Temperature and OneWire 2.3.8
 *
 * Workaround: refuse to upgrade past 2.0.13 until compatible versions exist.  This may involve a board update, OneWire update, or both.
 *
 */

void printAddress(DeviceAddress deviceAddress);

int compareChar(const void *a, const void *b) {
    return (*(int*)a-*(int*)b);
}

void sensorsInit()
{
    // ***** INPUT *****
  // Start up the TempSensor library.
  // sensors is a DallasTemperature object.
  // thermometer is an array of DeviceAddress values.
  sensors.begin(); // IC Default 9 bit. If you have troubles consider upping it 12. Ups the delay giving the IC more time to process the temperature measurement
  Serial.printf("Found %d sensor devices.\n", sensors.getDeviceCount());

  // From here we will work with all devices found
  int dc = sensors.getDeviceCount();

  // A temporary thermometer list long enough for all cases.
  // After selecting and ordering, some or all will be copied to 
  // thermometer[NT].
  DeviceAddress tt[max(dc, NT)];
  DeviceAddress needsSettingsEntry[max(dc, NT)]; // Just the new ones.

  int i, j;
  for (i = 0; i < max(dc, NT); i++) {
    if (!sensors.getAddress(tt[i], i)) {
      // This seems to get the value of the last valid sensor.  Set it
      // to zero to avoid confusion.
      for (int j = 0; j < 8; j++) tt[i][j] = (uint8_t) 0;
      Serial.print("Unable to find address for Sensor "); Serial.println(i+1);
    } else {
      // show the address we found on the bus
      Serial.print("Sensor "); Serial.print(i+1); Serial.print(" Address: ");
      printAddress(tt[i]);
      Serial.println();
      // set the resolution to 12 bit
      // This gives 4096 distinct values in the range -127 to +127
      // resulting in temperature steps of 255/4096 = 0.062 degrees C.
      // This is much finer than the claimed 0.5 C sensor accuracy, but
      // can produce odd-looking steps on graphs.
      sensors.setResolution(tt[i], 12);
    }
  }
  // Now we have array tt which is either full of all sensors
  // or has some with zero addresses if NT > dc.

  // A separate pass to figure out how to group the sensors if we have more than one set.
  // To consider:
  // 1) NT > sensor count is a problem.
  // 2) NT < sensor count requires deciding which sensors to ignore
  // 3) We want to group sensors in groups 0, 1, etc. but the reference array
  //    may return larger values we don't know until all are found. i.e. we
  //    may be using sets 2 and 5 in the array as our sets 0 and 1.
  int tanksMoved = 0;
  int extras = 0;
  if (NT > 4 || dc > 4) {
    // Work with the dc sensors  to sort them by group.
    DeviceAddress a;
    int setFound;
    int allFound[dc];
    int unique[dc]; // oversized - normally we'll find 0, 1 or 2 sets.
    int uCount = 0; // Unique set numbers, != 0
    for (i=0; i < dc; i++) {
      for (j = 0; j < 8; j++) a[j] = tt[i][j];  // Copy current address to a
      setFound = findSet(a);                 // Get its set number, or -1
      allFound[i] = setFound;
      if (setFound < 0) {                    // If no set, copy to a list for user action.
        for (j = 0; j < 8; j++) needsSettingsEntry[extras][j] = a[j];
        extras++;
      } else {
        // Identify unique values.
        bool found = false;
        for (j = 0; j < uCount; j++) {
          if (unique[j] == setFound) found = true;
        }
        if (!found) {
          unique[uCount] = setFound;
          uCount++;
        }
      }
    }
    Serial.println("allFound list:");
    for (i = 0; i < dc; i++) Serial.printf(" %d, ", allFound[i]);
    Serial.println();

    Serial.printf("There were %d unknown (new) sensors  and the rest were in %d unique sets.\n", extras, uCount);
    // Sort the list of unique sets, but not the allFound list.
    // Note that we only sort the first uCount values in a longer array, since later values are undefined.
    qsort(unique, (size_t) uCount, (size_t) sizeof(setFound), compareChar);
    Serial.println("Unique sets after sorting:");
    for (i = 0; i < uCount; i++) Serial.printf(" %d, ", unique[i]);
    Serial.println();

    // Now we copy addresses to thermometer in set order, saving -1 for last.
    int k, iSet = 0;
    int setMoved;
    // Move all thermometers with known sets to a tank, stopping
    // if all tanks are filled.
    for (i = 0; iSet < uCount & tanksMoved < NT; i++) {
      setMoved = 0;
      iSet = unique[i];
      // Copy all addresses matching iSet.
      for (k = 0; k < dc && tanksMoved < NT; k++) {
        if (allFound[k] == iSet) {
          for (int j = 0; j < 8; j++) thermometer[tanksMoved][j] = tt[k][j];
          tanksMoved++;
          setMoved++;
        }
      }
    }
    // If we have used all thermometers from known sets and still have 
    // unfilled tanks, use the unknowns.
    if (tanksMoved < NT) {
      // Copy all addresses matching iSet.
      for (k = 0; k < dc && tanksMoved < NT; k++) {
        if (allFound[k] == -1) {
          for (int j = 0; j < 8; j++) thermometer[tanksMoved][j] = tt[k][j];
          tanksMoved++;
        }
      }
    }

  } else {
    // Simpler code for base case with no more than 4 tanks and 4 sensors.
    for (i = 0; i < min(dc, NT); i++) {
      for (int j = 0; j < 8; j++) thermometer[i][j] = tt[i][j];
      tanksMoved++;
    }
  }

  // Another pass just to print any unknown addresses in array initializer format for use
  // in settings.h
  if (extras > 0) {
    Serial.printf("There were %d sensors not known from Settings.ini.\n", extras);
    Serial.println("Please add these to addressSets there, updating the knownAddressSets value.");
    Serial.println("Be sure to include a comma between sets.");
    Serial.println("addressSet = \n  {");
    bool comma = false;
    for (int i = 0; i<extras; i++) {
      // Skip zero addresses, assuming that checking the first 2 bytes is enough.
      if (needsSettingsEntry[i][0] > 0 || needsSettingsEntry[i][1] > 0) {
        if (comma) Serial.println(",");
        comma = true;
        Serial.print("    ");
        printAddressBytes(needsSettingsEntry[i]);
      }
    }
    Serial.println("\n  }");
  }
}

/**
 * Given a DeviceAddress find the set it belongs to.
 * If not found, return -1.
 */
int findSet(DeviceAddress a) {
  // Crudely look at each sensor in each set and return the set number if
  // a match is found.
  //int sensorSetCount = (int) (sizeof(addressSets) / sizeof(addressSets[0][0][0]));
  for (int i=0; i < knownAddressSets; i++) {
    for (int j=0; j < 4; j++) {
      // The first condition to fail should end the evaluation, so put the 
      // address parts which seem to vary most often at the front.
      if (addressSets[i][j][7] == a[7] &&
          addressSets[i][j][1] == a[1] &&
          addressSets[i][j][2] == a[2] &&
          addressSets[i][j][3] == a[3] &&
          addressSets[i][j][4] == a[4] &&
          addressSets[i][j][5] == a[5] &&
          addressSets[i][j][6] == a[6] &&
          addressSets[i][j][0] == a[0] ) return i;
    }
  }
  return -1;
}


// function to print a device address
// A DeviceAddress is simply an array of 8 8-bit integers.
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to print a device address
// A DeviceAddress is simply an array of 8 8-bit integers.
void printAddressBytes(DeviceAddress deviceAddress)
{
  /*
  Serial.print("{");
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    Serial.print("0x");
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    if (i < 7) Serial.print(",");
  }
  Serial.println("}");
   */
  // What about printf?
  Serial.printf("{ 0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x,0x%02x }", deviceAddress[0], deviceAddress[1],
    deviceAddress[2], deviceAddress[3],deviceAddress[4], deviceAddress[5],deviceAddress[6], deviceAddress[7]);

}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();
}

// main function to print information about a device
void printData(DeviceAddress deviceAddress)
{
  Serial.print("Device Address: ");
  printAddress(deviceAddress);
  Serial.print(" ");
  printTemperature(deviceAddress);
  Serial.println();
}
