/**
 * SPIFFS uploads seen online don't work on this system.  This sketch reads
 * from SD and writes to SPIFFS so the files only have to be present once and the 
 * SD card only has to be reliable once.
 * Also, serving web files from SPIFFS is much faster than from SD.
 * Most of the code is copy-pasted from the SPIFFS_Test example.
 * 1) Copy the htdocs directory from CBASS-32/htdocs to the root directory of a microSD
 *    card.
 * 2) Insert the card in the CBASS-32 system you wish to initialize.
 * 3) Build and run this sketch on that system.
 * 4) The Arduino NANO ESP32 now has those files permanently installed.
 * 5) Optionally, you may delete the files from the SD card, as they are no
 *    longer needed there.
 * PROTIP: Initialize all of your Arduinos at once before assembling them into
 *    the enclosures.  You can do this by swapping them into a single CBASS-32 using
 *    a single microSD card if that is more convenient.  You just need to load and
 *    run SPIFFS_upload on each.
 */
#include "FS.h"
#include "SPIFFS.h"
#define FORMAT_SPIFFS_IF_FAILED true

#include <SD.h>
#define SD_CS D5

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}

void copyFile_toFS(fs::FS &fs2, const char *path) {
  Serial.printf("Reading file: %s from SD to SPIFFS\r\n", path);

  File fileIn = SD.open(path);
  if (!fileIn || fileIn.isDirectory()) {
    Serial.println("- failed to open file for reading");
    return;
  }
  Serial.print("Original size is "); Serial.println(fileIn.size());
  deleteFile(fs2, path);
  File fileOut = fs2.open(path, "a"); //FILE_WRITE);
  if (!fileOut || fileOut.isDirectory()) {
    Serial.println("- failed to open file for writing");
    return;
  }

  Serial.println("- copying file");
  size_t n, ns;
  uint8_t buf[4096];  // Was 128.  Why so small?  512 is typical for SD
  size_t www;
  while ((n = fileIn.read(buf, sizeof(buf))) > 0) {
    // Serial.printf("Read %d bytes from original file of size %d.  ", n, origSize);
    www = fileOut.write(buf, n);
    Serial.print(".");
  }
  fileOut.flush();
  Serial.print("\nCopied size is "); Serial.println(fileOut.size());

  uint64_t iS = fileIn.size();
  uint64_t oS = fileOut.size();
  if (iS == oS) {
    Serial.printf("Success: copied %llu bytes.\n", iS);
  } else {
    Serial.printf("===== ERROR  ===== original was %llu bytes,", iS);
    Serial.printf(" but copy was %llu.\n", oS);
  } 

  fileIn.close();
  fileOut.close();
}



/**
 * Not a function we need except for testing.
 */
void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void SDinit() {
  pinMode(SD_CS, OUTPUT);
  bool init = false;
  for (int i = 0; (i < 5) && !init; i++) {
    init = SD.begin(SD_CS);
    if (!init) {
      Serial.println("Failed to initialize microSD card.  Trying again.");
      delay(500);
    }
  }
  // Option: instead of waiting for a fatal error to set off
  // the watchdog, we could immediately
  // ESP.restart();
  if (!init) {
    Serial.println("SD initialization failed!");
    return;
  }

  Serial.println("SD initialization done.");
  if (!SD.exists("/")) {
    Serial.println("but root directory does not exist!");
    Serial.println("SD has no root directory!");
    while (1)
      ;
  }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);
  Serial.printf("Level %d\n", levels);
  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
        Serial.printf("Up to Level %d\n", levels);

      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("\tSIZE: ");
      Serial.print(file.size());
      Serial.print("\tPATH:");
      Serial.println(file.path());
    }
    file = root.openNextFile();
  }
}

void setup() {
  Serial.begin(115200);
  delay(5000);
  if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  SDinit();

  listDir(SPIFFS, "/", 0);
  //deleteFile(SPIFFS, "/test.txt");
  //deleteFile(SPIFFS, "/fakesub/hello.txt");

  // Copy each file, deleting any prior version first.
  copyFile_toFS(SPIFFS, "/htdocs/trash.png");
  copyFile_toFS(SPIFFS, "/htdocs/plus_circle.png");
  copyFile_toFS(SPIFFS, "/htdocs/favicon.ico");
  copyFile_toFS(SPIFFS, "/htdocs/page.css");
  copyFile_toFS(SPIFFS, "/htdocs/plotly.js");
  

  Serial.println("\nSPIFFS dir:");
  listDir(SPIFFS, "/", 0);

  Serial.println("\nSD htdocs dir:");
  listDir(SD, "/htdocs", 0);


  Serial.println("SPIFFS insertion complete");
}

void loop() {
  // put your main code here, to run repeatedly:
}
