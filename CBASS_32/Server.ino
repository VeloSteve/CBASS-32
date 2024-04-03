/*
 * The Server.ino file is meant to contain the definitions of all the web pages and
 * other server requests needed by CBASS-32.
 The initial strategy will be
 * 1) Define each file location or page name in a compact "server.on()" or "server.serveStatic()" call.
 * 2) Keep large static files on the SD card in /htdocs
 * 3) Keep smaller chunks of HTML, javascript, and possibly CSS in WebPieces.h as PROGMEM variables.
 * 4) Most pages will have a static header and footer and a dynamic body.
 * 5) The dynamic bodies will be defined in calls to "processor()"
 * 6) Pages will normally use a response object in addition to the request object allowing
 *    for headers and parts of the response to be added sequentially.
 * 7) Another convention: variables set to be picked up by processor() will start with "p_"
 */


File32 fileForWeb;
bool ledState = false;
bool goodTimeChange = false;
String postBuffer;
String p_message, p_title;
const int maxPathLen = 256;
char dirPath[maxPathLen];  // For directory paths received as arguments.  Don't let this overrun.
char fnBuffer[maxPathLen];

// Prototypes
void openForSend(char *fn);
void sendFromFile(WiFiClient client);
void sendAsHM(unsigned int t, WiFiClient client);
bool setNewStartTime(String queryString);
int timeOrNegative(String s);
void sendXY(AsyncResponseStream *rs);
String sendFileInfo();
void sendRampForm(AsyncResponseStream *rs);
void sendAsHM(unsigned int t, AsyncResponseStream *rs);
bool rewriteSettingsINI();
boolean receivePlanJSON(String js, AsyncResponseStream *response);
String processor(const String &var);
String showDateTime();
char *getFileName(File32 f);
void pauseLogging(boolean a);

/**
 * Read up to maxLen bytes from the log file starting at logChunkPos and be
 * ready for more calls until everything has been read.  When returning zero
 * (no more bytes) reset the position marker.
 */
/*
uint32_t logChunkPos = 0;
File32 fileForChunks;
size_t logChunks(uint8_t *buffer, size_t maxLen) {
  Serial.println("In logChunks.");
  size_t maxRead = 4096;
  int bytesRead = 0;
  // Stop logging to avoid race conditions.  Be sure to reset
  // this regardless of result.
  pauseLogging(true);
  if (!fileForChunks) {
    fileForChunks = SDF.open("/LOG.txt", O_RDONLY);
  }
  fileForChunks.seekSet(logChunkPos);
  // Read no more than the server asks for, but also no more than maxRead,
  // which is meant as a reasonable speed/memory/safety compromise.
  bytesRead = fileForChunks.read(buffer, min(maxRead, maxLen));
  Serial.printf("logChunks returning %d bytes, start = %d,  maxLen = %d, maxRead = %d.\n", bytesRead, logChunkPos, maxLen, maxRead);

  if (bytesRead <= 0) {
    logChunkPos = 0;
    fileForChunks.close();
    pauseLogging(false);
  } else {
    logChunkPos += bytesRead;
  }
  return bytesRead;
}
 */

/**
 * Return parts of a file. Each chunk is placed in the buffer, and
 * a static variable records the position of the next byte to read.
 * TODO: Verify that the correct thing happens if two users request
 * the same file (or different ones for that matter) simultaneously.
 * buffer: a buffer provided by the calling function, where output bytes are placed.
 * maxLen: the size of the buffer, thus an upper bound on the bytes to read.
 * fName: the name of the file in the filesystem.
 */
uint32_t fileChunkPos = 0;
size_t fileChunks(uint8_t *buffer, size_t maxLen, const char *fName) {
  static File32 fileForChunks;
  if (!fileChunkPos) Serial.printf("In file chunks for %s.\n", fName);
  size_t maxRead = 4096;
  int bytesRead = 0;
  // Stop logging to avoid race conditions.  Be sure to reset
  // this regardless of result.
  pauseLogging(true);
  if (!fileForChunks) {
    fileForChunks = SDF.open(fName, O_RDONLY);
  }
  fileForChunks.seekSet(fileChunkPos);
  // Read no more than the server asks for, but also no more than maxRead,
  // which is meant as a reasonable speed/memory/safety compromise.
  bytesRead = fileForChunks.read(buffer, min(maxRead, maxLen));
  //Serial.printf("fileChunks returning %d bytes, start = %d,  maxLen = %d, maxRead = %d.\n", bytesRead, fileChunkPos, maxLen, maxRead);

  if (bytesRead <= 0) {
    fileChunkPos = 0;
    fileForChunks.close();
    pauseLogging(false);
    Serial.printf("Completed sending %s.", fName);
  } else {
    fileChunkPos += bytesRead;
  }
  return bytesRead;
}


/**
 * This is the main function which sets up handling of requests.  It uses the
 * globally-defined server variable to access methods of an AsyncWebServer.
 * Once defined these will be activated behind the scenes as
 * requests come in.
 */
void defineWebCallbacks() {
  Serial.print("Defining callbacks...");

  // Root page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Sending root web page.");
    p_title = "CBASS-32 Start Page";
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", basePage, processor);
    response->addHeader("Server", "ESP Async Web Server");
    request->send(response);
  });

  /**
   * While just "/" gets the root page, anything else not defined explicitly will be
   * looked for in /htdocs and served without modification.  This will typically be
   * javascript or CSS, since we want to process the HTML pages.
   * Speed: the plotly.js file served from SPIFFS takes 230 to 500ms.  Sending from SD
   * as a chunked responsed takes 8.7 seconds or more!
   */
  server.serveStatic("/", SPIFFS, "/htdocs/").setCacheControl("max-age=3600");  // for a full day: 86400");


  /**
   * In progress: replace simple HTML links with a form requiring the magic word.  Enforce
   * the requirement on the server side.
   */
  server.on("/LogManagement", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Sending log management page.");
    p_title = "CBASS-32 Log Management";
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", logHTML2, processor);
    response->addHeader("Server", "ESP Async Web Server");
    request->send(response);
  });

  // Roll over the log and let the user know the results.

  server.on("/LogRoll", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Rolling over LOG.txt");
    p_title = "Log Rollover Result";


    int rCode = 200;
    if (!request->hasParam("magicWord")) {
      rCode = 401;  // Unauthorized
      p_message = "  No magic word received with log roll request.";
      Serial.println(p_message);
    } else {
      AsyncWebParameter *p = request->getParam("magicWord");
      Serial.printf("LogDownload got magicWord %s\n", p->value().c_str());

      if ((p->value()).equals(MAGICWORD)) {
        rCode = 200;
      } else {
        rCode = 401;
        p_message = "  Wrong or missing magic word.";
      }
    }
    if (rCode != 200) {
      AsyncWebServerResponse *response;
      response = request->beginResponse(rCode, "text/html", p_message.c_str());
      request->send(response);
      return;
    }


    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/plain", rollLogNow, processor);
    response->addHeader("Server", "ESP Async Web Server");
    request->send(response);
  });

  // Allow the user to synchroize CBASS time to their device.
  server.on("/SyncTime", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Synchronize time");
    p_title = "Synchronize CBASS Clock";
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", syncTime, processor);
    response->addHeader("Server", "ESP Async Web Server");
    request->send(response);
  });


  // Javascript for sending back table data from the ramp plan page.
  server.on("/tableScript.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Sending table data javascript.");
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", tableScript, processor);
    response->addHeader("Server", "ESP Async Web Server");
    response->addHeader("cache-control", "public, max-age=200");  // 86400");
    request->send(response);
  });

  // Javascript for the ramp plan
  server.on("/rampPlan.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Sending ramp plan plot javascript");
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/javascript", rampPlanJavascript, processor);
    response->addHeader("Server", "ESP Async Web Server");
    request->send(response);
  });


  /**
   * Part 1:
   * Receive data and update ramp plan
   * WARNING: long request bodies will result in this being called more than once.  The
   * user code is responsible for putting it back together. It breaks at arbitrary, for example
   * between digits of a number!
   * 1620 bytes gets broken pretty close to the end.  Test with that much even though typical CBASS
   * work will use less.  Maybe this is based on the typical 1500 byte ethernet packet?
   *
   *
   * Note that this does not return a web page, just a message, so the usual HTML parts are omitted.
   */
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    Serial.printf("Checking url for request body >%s<.\n", (request->url()).c_str());
    if (request->url() == "/updateRampPlan") {
      Serial.printf("In requestBody for updateRampPlan, len %d index %d total %d\n.", len, index, total);
      // Is this the first part?
      if (!index) {
        postBuffer = "";  // This is a String.
        Serial.printf("BodyStart: %u B\n", total);
      }
      // Save this chunk.
      postBuffer += (const char *)data;
      Serial.print("Latest buffer --->");
      Serial.print(postBuffer);
      Serial.println("<---");
      for (size_t i = 0; i < len; i++) {
        Serial.write(data[i]);
      }
      Serial.println();
      // Have we got everything?
      if (index + len == total) {
        Serial.printf("BodyEnd: %u B\n", total);
        AsyncResponseStream *response = request->beginResponseStream("text/html");

        if (!receivePlanJSON(postBuffer, response)) {
          // request->send(200, "text/plain", "false");
          response->setCode(400);
          request->send(response);
        } else {
          request->send(200, "text/plain", "true");
        }
        postBuffer = "";
      }
    }
  });




  // Plot and monitor temperatures
  server.on("/Tchart.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Sending chart page.");
    p_title = "Temperature Monitor";
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", plotlyHTML, processor);
    response->addHeader("Server", "ESP Async Web Server");
    request->send(response);
  });

  // Return current temperatures.  This is another case where no processor() call is needed
  // so we can just write to a stream.
  server.on("/currentT", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Sending latest Temps.");
    AsyncResponseStream *response = request->beginResponseStream("text/csv");
    response->addHeader("Server", "ESP CBASS-32");
    sendXY(response);

    request->send(response);
  });

  // TODO require password
  server.on("/Reboot", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Rebooting by Web request!");
    Serial.flush();
    p_title = "CBASS-32 Web Reboot";
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", rebootHTML, processor);
    response->addHeader("Server", "ESP Async Web Server");
    request->send(response);
    delay(500);
    ESP.restart();
  });

  // Update CBASS time from the user's device.
  server.on("/updateDateTime", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Updating time from device");
    bool changed = false;
    //p_title = "Synchronize CBASS Clock";
    // Before responding, set the clock if parameters are correct.
    // The hasParam checks are rather verbose, but they make it easier to diagnose
    // problems.
    int rCode = 200;
    if (!request->hasParam("magicWord")) {
      rCode = 401;  // Unauthorized
      Serial.println("  No magic word received.");
    } else if (!request->hasParam("datetime")) {
      rCode = 406;  // Not acceptable
      Serial.println("  No time string received.");
    } else if (!request->hasParam("sync")) {
      rCode = 406;  // Not acceptable
      Serial.println("  No sync parameter received.");
    } else {
      AsyncWebParameter *p = request->getParam("sync");
      Serial.printf("updateDateTime got sync %s\n", p->value().c_str());
      if ((p->value()).equals("true")) {
        p = request->getParam("magicWord");
        Serial.printf("updateDateTime got magicWord %s\n", p->value().c_str());

        if ((p->value()).equals(MAGICWORD)) {
          p = request->getParam("datetime");
          Serial.printf("updateDateTime got time %s\n", p->value().c_str());
          // The value should look like:
          // 27/01/2024 15:38:46
          changed = settime(p->value().c_str());
          if (!changed) rCode = 500;
        } else {
          rCode = 401;
        }
      } else {
        rCode = 406;
      }
    }
    Serial.print("updateDateTime changed flag is ");
    Serial.println(changed);

    AsyncWebServerResponse *response;
    if (changed) {
      response = request->beginResponse(200, "text/html", showDateTime());
    } else {
      response = request->beginResponse_P(rCode, "text/html", String("Failure.  Time is still " + showDateTime() + ".").c_str(), processor);
    }
    response->addHeader("Server", "ESP Async Web Server");
    request->send(response);
  });

  // List files
  server.on("/files", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.print("Showing SD files...");
    if (request->hasParam("path")) {
      AsyncWebParameter *p = request->getParam("path");
      if (strlen(p->value().c_str()) > maxPathLen - 1) {
        strcpy(dirPath, "");  // This flags that we didn't get a valid path.
      } else {
        strcpy(dirPath, p->value().c_str());
      }
    } else {
      strcpy(dirPath, "/");
    }
    p_title = "Directory Listing";

    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", dirListHTML, processor);
    response->addHeader("Server", "ESP CBASS-32");
    request->send(response);
  });

  // Display and edit the ramp plan.
  server.on("/RampPlan", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Sending ramp plan management form.");
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->addHeader("Server", "ESP CBASS-32");
    sendRampForm(response);

    request->send(response);
  });

  /* Reset the ramp plan.  This is mainly for quick recovery during development.  Later
   * this may be:
   * - removed since no one wants just the default and a good plan could be erased
   * - kept as a part of getting started on a new SD card
   *
   * With no parameters this offers the option and a place to enter the magic word.
   * With the word and reset=true it does the reset.
   *
   * Notes:
   * Replaceable part of page is
   * - Description of the action and the magic word box, in the basic state
   * - Same with an error message if the magic word is wrong.
   * - Failure message an no button if SD modification fails.
   * - Success message with suggestion to go to the edit/management page on success.  (or just go to the managment page?)
   */
  server.on("/ResetRampPlan", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Ramp plan reset page.");
    if (request->hasParam("magicWord") && request->hasParam("reset")) {
      Serial.println("  has both magic word and reset params");
      AsyncWebParameter *pm = request->getParam("magicWord");  // value() is a String
      AsyncWebParameter *pr = request->getParam("reset");
      p_message = "";
      if (pm->value() == MAGICWORD) {
        if (pr->value() == "true") {
          Serial.println("  Magic word matches, ok to reset");

          if (resetSettings()) {
            // Send the ramp management page.
            request->redirect("/RampPlan");
            return;
          } else {
            Serial.println("Reset failed.  You may need to repair the SD card manually.");
            p_message = "Reset failed.  You may need to repair the SD card manually.";
          }
        } else {
          Serial.println("Settings.ini reset was missing a required parameter.");
          p_message = "Settings.ini reset was missing a required parameter.";
        }
      } else {
        Serial.println("Magic word does not match!");
        p_message = "Magic word does not match!";
      }
      // Resend page with specified message.

    } else {
      Serial.println("Didn't get two parameters.  Sending the base page.");
      // No parameters (one one missing) so just use the base page with no message.
      ;
    }
    request->send_P(200, "text/html", iniResetPage, processor);
  });

  /**
   * Note that his sends a file for download, so it doesn't have html code processing.
   */
//  Chunked response - my function will provide the chunks using the SdFat library.
  server.on("/LogDownload", HTTP_GET, [](AsyncWebServerRequest *request) {
    fileChunkPos = 0;  // Lets the function know to start at zero.


    int rCode = 200;
    if (!request->hasParam("magicWord")) {
      rCode = 401;  // Unauthorized
      p_message = "  No magic word received with download request.";
      Serial.println(p_message);
    } else {
      AsyncWebParameter *p = request->getParam("magicWord");
      Serial.printf("LogDownload got magicWord %s\n", p->value().c_str());

      if ((p->value()).equals(MAGICWORD)) {
        rCode = 200;
      } else {
        rCode = 401;
        p_message = "  Wrong magic word.";
      }
    }
    if (rCode != 200) {
      AsyncWebServerResponse *response;
      response = request->beginResponse(rCode, "text/html", p_message.c_str());
      request->send(response);
      return;
    }

    Serial.println("In LogDownload call.");
    fileChunkPos = 0; // Start at byte zero - this should already be set.

    request->sendChunked(
      "text/plain",
      [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        // Generate some data
        // return logChunks((uint8_t *)buffer, maxLen);
        return fileChunks((uint8_t *)buffer, maxLen, "/LOG.txt");
      });
    pauseLogging(false);  // Normally set false in the last logChunks call, but verify.
  });

  // File upload
#ifdef ALLOW_UPLOADS
  server.on("/UploadPage", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Sending upload page.");
    p_title = "File Upload";
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", uploadHTML, processor);
    response->addHeader("Server", "ESP Async Web Server");
    request->send(response);
  });
#endif

#ifdef ALLOW_UPLOADS
  server.on(
    "/Upload", HTTP_POST, [](AsyncWebServerRequest *request) {
      AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Upload successful!");
      request->send(response);
    },
    [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      // Save uploaded file to SD
      // TODO: handle a subdirectoriesctory.
      // On SD (maybe not on SPIFFS) files must start with "/".
      //Serial.printf("Upload index = %d\n", index);
      if (!index) {
        Serial.println("All parameters");
        int params = request->params();
        for(int i=0;i<params;i++){
          AsyncWebParameter* p = request->getParam(i);
          if(p->isFile()){ //p->isPost() is also true
            Serial.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
          } else if(p->isPost()){
            Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
          } else {
            Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
          }
        }
        Serial.println("End parameters");
      }
      // Convention: Files and directories start but do not end with a "/".
      if (!filename.startsWith("/")) filename = "/" + filename;
      if (filename.endsWith("/")) filename.remove(filename.lastIndexOf("/"));
      static String dC;
      static String nD;
      static String fullPath;
      if (!index) {
        Serial.printf("UploadStart: %s\n", filename.c_str());
        AsyncWebParameter* p = request->getParam("dirChoices", true);  // "true" argument required since this is a POST, not GET.
        Serial.printf("dirChoices printable? %s\n", p->value().c_str()); delay(1000);

        //dC = String(p->value().c_str());
        dC = p->value();

        dC.trim();
        if (dC == "--new--") {
          // get the name, create the directory if needed, and change into it.
          p = request->getParam("newdir", true);
          nD = p->value();
          // For consistency, remove any leading "/"
          if (nD.startsWith("/")) nD.remove(0, 1);
          if (filename.endsWith("/")) filename.remove(filename.lastIndexOf("/"));
          if (SDF.exists(nD)) {
            Serial.printf("ERROR: no upload.  File or directory %s already exists.\n", nD);
            return;
            // XXX does this abort the upload or just the first of many calls?
          }
          //File32 root = SDF.open("/", O_WRONLY);
          SDF.mkdir(nD);
          fullPath = "/" + nD;
        } else if (dC == "/") {
          fullPath = "/";
        } else {
          // change to the selected directory.  It should exist since it came from the dropdown,
          // but check.
          fullPath = "/" + dC;
        }
        fullPath = fullPath + filename;
        Serial.printf("Upload had dC = %s, nD = %s, fullPath = %s.\n", dC.c_str(), nD.c_str(), fullPath.c_str());
        SDF.remove(fullPath);
        pauseLogging(true);  // Do not interrupt during a log write, but pause it.
      }
      File32 file = SDF.open(fullPath, O_WRONLY | O_CREAT | O_APPEND);
      file.seekEnd(0);  // Belt and suspenders.  Be sure we are at the end.
      if (file) {
        for (size_t i = 0; i < len; i++) {
          file.write(data[i]);
        }
        file.close();
      } else {
        Serial.printf("Failed to open destination %s on CBASS.\n", filename);
      }
      if (final) {
        pauseLogging(false);
        Serial.printf("UploadEnd: %s, %u B\n", fullPath.c_str(), index + len);
      }
    });
#endif

  // About page
  server.on("/About", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Sending About CBASS-32 web page.");
    p_title = "CBASS-32 Start Page";
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", aboutPage, processor);
    response->addHeader("Server", "ESP Async Web Server");
    request->send(response);
  });

  Serial.println("Sending board image.");
  fileChunkPos = 0; // Start at byte zero - this should already be set.
    //  Chunked response - my function will provide the chunks using the SdFat library.
  server.on("/32Board.png", HTTP_GET, [](AsyncWebServerRequest *request) {
    static int chunkPos = 0;  // Lets the function know to start at zero.
    request->sendChunked(
      "image/png",
      [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t {
        // Generate some data
        return fileChunks((uint8_t *)buffer, maxLen, "/htdocsSD/32Board.png");
      });
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found on this reef.");
  });
  Serial.println(" Done defining server actions.");

}


/**
 * Copy the current LOG.txt file to a backup directory.
 * A new empty log file will be automatically created
 * the next time a log line is added.
 *
 * The log copy will be timestamped in two ways.  It has proven
 * impractical to set a timestamp in the filesystem.  Also, filenames
 * of more than 8 characters (dot) 3 characters are no fully supported.
 * This is what will happen.
 * 1) A file name will be generated in the format YMDDHHMM.txt
 *    Y is the last digit of the current year.
 *    M is the month, written as 1-9, A, B, C for the 12 months.
 *    DD is the day of the month, zero padded
 *    HH is the hour of the day, 00 to 23
 *    MM is the minute, 00 to 59
 *    This rather cryptic system will allow default alphabetization to put
 *    the files in time order.
 * 2) A last line will be appended to the file stating the rollover time in a normal format.
 *
 * An early version appended text to the response stream, but not build a single String
 * of output so this can be inserted by the processor().
 */
String rollLog() {
  String result;
  // Generate the new file name.
  t = rtc.now();
  // Year
  String newName = String("/SaveLogs/");
  newName += String(t.year() % 10);
  // Month 1-9, A, B, C
  if (t.month() < 10) {
    newName += String(t.month());
  } else {
    // Cast to char is critical, or it prints the ASCII value as decimal.
    newName += String((char)('A' + t.month() - 10));
  }
  // Day, hour, and minutes are all 2 digits, zero padded.
  char buffer[64];  // Long enough for either the new file name (12+1 characters) or the human-friendly time and text later on.
  sprintf(buffer, "%02d%02d%02d.txt", t.day(), t.hour(), t.minute());
  newName += String(buffer);

  // Copy LOG.txt to the new name.  First be sure the directory is there.
  if (!SDF.exists("/SaveLogs")) {
    if (!SDF.mkdir("/SaveLogs")) {
      Serial.println("ERROR: Failed to make a log archive directory.  Abandoning log rollover!");
      result = "Failed to roll over the log.  Could not find or created directory /SaveLogs.";
      return result;
    }
  }

  // Important: make sure there's no conflict between this an ongoing logging.
  pauseLogging(true);
  delay(40);  // Probably not necessary, but allow any in-progress log line to complete.  A timing gave 2088 ms for 100 rapidly-sent log lines.

  if (myFileCopy("/LOG.txt", newName.c_str())) {
    SDF.remove("/LOG.txt");
    // Re-open the copy and append a message about the save date.
    Serial.println("Appending date and time to archived log file.");
    File32 newFile = SDF.open(newName.c_str(), O_WRONLY | O_CREAT | O_APPEND);
    newFile.seekEnd(0);
    sprintf(buffer, "archived on %s at %s local CBASS time.\n", getdate(t).c_str(), gettime(t).c_str());
    newFile.printf("\nThis file was %s", buffer);
    newFile.close();
    result = "Log copy was ";
    result += buffer;
    Serial.println(result);
  } else {
    result = "Log copy.  The original is unchanged.";
  }
  pauseLogging(false);

  //TODO HERE - this works, but could use some error checking.  Also, only an empty page is returned!

  // Ensure that the next logging call will include a header.
  SerialOutCount = serialHeaderPeriod + 1;
  return result;
}

/**
 * Send the latest temperature status in JSON format, for example
 * {"NT":[4],"timeval":[46],"CBASStod":["19:39:50"],"tempList":[0.0,0.0,0.0,0.0],"targetList":[32.0,27.0,36.0,38.0]}
 */
void sendXY(AsyncResponseStream *rs) {
  // Temp checks fill this: tempInput[]
  int tv = (int)(millis() / 1000);
  rs->printf("{\"NT\":[%d],", NT);
  rs->printf("\"timeval\":[%d],", tv);
  rs->print("\"CBASStod\":[\"");
  rs->print(gettime());
  rs->print("\"],\"tempList\":[");

  for (int i = 0; i < NT; i++) {
    rs->printf("%#.1f", tempInput[i]);
    if (i < NT - 1) rs->print(",");
  }

  rs->print("],\"targetList\":[");
  for (int i = 0; i < NT; i++) {
    rs->printf("%#.1f", setPoint[i]);
    if (i < NT - 1) rs->print(",");
  }
  rs->println("]}");
}

/**
 * List the files in the given path location.  It is an error if this is not a directory.
 * Originally this received a stream to write to and a path to examine.  Now the path
 * is a global and this returns a String.
 */
String sendFileInfo() {
  // dirPath is a global containing the directory to list.
  Serial.print("In sendFileInfo.\n");
  if (strlen(dirPath) == 0) {
    return String("No path, or the specified path is too long.");
  }
  const int maxOutputBuffer = 2048;
  char outputBuffer[maxOutputBuffer];  // The ugly process of building a String using sprintf or snprintf needs a buffer.
  pauseLogging(true);
  File32 root;
  root.open(dirPath);
  if (!root) {
    Serial.printf("Path >%s< not opened.\n", dirPath);
    sprintf(outputBuffer, "The specified path, \"%s\" could not be opened.", dirPath);
    pauseLogging(false);
    return String(outputBuffer);
  }
  if (!root.isDirectory()) {
    Serial.printf("Path >%s< is not a directory.\n", dirPath);
    sprintf(outputBuffer, "The specified path, \"%s\" is not a directory.", dirPath);
    pauseLogging(false);
    return String(outputBuffer);
  }

  // The output is a table with one row per file (and .. if a subdirectory)
  String rString = "<table><tr><th>Type</th><th>Name</th><th>Size</th></tr>\n";

  // Enable going up a level if not already at the top
  if (strlen(dirPath) > 1) {
    Serial.printf("Prepare to link one above %s\n", dirPath);
    char *last = strrchr(dirPath, '/');
    int pos = last - dirPath;
    if (pos == 0) {
      // Going to root, no parameter needed.
      sprintf(outputBuffer, "<tr><td>UP</td><td><a href=\"http://%s/files\">..</a></td></tr>\n", myIP.toString().c_str());
    } else {
      //Serial.printf("Adding up line with last of %s, pos = %d\n", last, pos);
      char sub[1 + strlen(dirPath)];  // Size for anything <= the full path.
      strncpy(sub, dirPath, pos);
      sub[pos] = '\0';  // strncpy doesn't do this automatically!
      sprintf(outputBuffer, "<tr><td>UP</td><td><a href=\"http://%s/files?path=%s\">..</a></td></tr>\n",
              myIP.toString().c_str(), sub);
    }
    rString += String(outputBuffer);
  }

  // SD.h way: File32 file = root.openNextFile();
  File32 file;
  int pass = 0;
  while (file.openNext(&root, O_RDONLY)) {
    getFileName(file);  // Put the name into fnBuffer;

    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(fnBuffer);
      if (strlen(dirPath) > 1) {
        sprintf(outputBuffer, "<tr><td>DIR</td><td><a href=\"http://%s/files?path=%s/%s\">%s</a></td></tr>\n", myIP.toString().c_str(), dirPath, fnBuffer, fnBuffer);
      } else {
        sprintf(outputBuffer, "<tr><td>DIR</td><td><a href=\"http://%s/files?path=/%s\">%s</a></td></tr>\n", myIP.toString().c_str(), fnBuffer, fnBuffer);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(fnBuffer);
      Serial.print("\tSIZE: ");
      Serial.println(file.size());
      // It is easy enough to get and print file dates, but not so easy to
      // set them as files are created!!!
      // time_t tt = file.getLastWrite();  // Returns seconds from 1970, but DateTime uses 2000.

      sprintf(outputBuffer, "<tr><td>FILE</td><td>%s</td><td>%d</td></tr>\n", fnBuffer, file.size());
    }
    rString += String(outputBuffer);
    // SD.h way: file = root.openNextFile();
    file.close();  // Even though we call a method on file once each pass, examples close it each time.
  }
  rString += "</table>\n";
  file.close();
  root.close();
  pauseLogging(false);

  //Serial.println("Before return, rString is:");
  //Serial.println(rString);
  return rString;
}

/**
 * SdFat files don't have .name().  Use .getName and return a buffer.
 * Note that fnBuffer is updated whether the return value is handled or not.
 */
char *getFileName(File32 f) {
  static char temp[maxPathLen];
  f.getName(fnBuffer, maxPathLen);
  // The ~ character interferes with later replacements by the text processor.  Replace with &tilde;
  // Note that strstr returns a pointer, not a number of characters.  This is normally only needed
  // when a text editor leaves a backup file with a "~" at the end of the name.
  char *pos;
  while (pos = strstr(fnBuffer, "~")) {
    // There is a ~ (otherwise null is returned from strstr)
    strncpy(temp, fnBuffer, pos-fnBuffer);  // Copy up to the ~
    temp[pos-fnBuffer] = '\0';                // null terminate (sprintf does this, strncpy does not)
    sprintf(temp + (pos-fnBuffer), "%s%s", "&tilde;", pos+1);  // append the tilde and the rest of the original.
    sprintf(fnBuffer, "%s", temp);  // Back into fnBuffer for another pass (rare) or as result.

  }
  return fnBuffer;
}

/**
 * This was originally a form, thus some of the names. Now javascript is used to
 * collect information from the table and other entry points and post it.
 */
void sendRampForm(AsyncResponseStream *rs) {

  Serial.println("Sending ramp plan management page.");

  rs->println("<html>");
  rs->println("<head><title>Ramp Plan</title>	<link rel=\"stylesheet\" type=\"text/css\" href=\"/page.css\" />");
  rs->println(" <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"> </head>");

  //TODO - probably should move  most of this to a static file and
  //insert any values with a processor OR javascript after loading.

  rs->println("<body><h1>Ramp Plan</h1>");
  rs->println("<div class=\"container\">");
  rs->println("<div id=\"messages\" class=\"wrapper flex\"></div>");

  short j;  // Tank counter
  // now show the ramp plan itself.
  const char *addIcon = "<img width=\"24\" src=\"/plus_circle.png\" onclick=\"addRow(this)\">";
  const char *remIcon = "<img width=\"24\" src=\"/trash.png\"  onclick=\"removeRow(this)\">";
  rs->println("<div class=\"wrapper flex\" id=\"tablediv\">");
  rs->print("<table id=\"rampTable\"><tr><th>Time</th>");
  for (j = 0; j < NT; j++) rs->printf("<th>Tank %d</th>", (j + 1));
  rs->println("</tr>");

  for (short k = 0; k < rampSteps; k++) {
    rs->print("<tr><td contenteditable=\"true\" class=\"time\" onblur=\"validateCellTime(this)\">");
    sendAsHM(rampMinutes[k], rs);
    rs->print("</td>");
    for (j = 0; j < NT; j++) {
      rs->print("<td contenteditable=\"true\" class=\"temperature\" onblur=\"validateCellTemp(this)\">");
      rs->print(((double)(rampHundredths[j][k]) / 100.0), 2);
      rs->print("</td>");
    }
    //  Here we add icons to add or remove rows.  Don't allow the first to be deleted, and keep at least 2.
    if (k > 0) {
      rs->printf("<td>%s</td><td>%s</td>", addIcon, remIcon);
    } else {
      rs->printf("<td>%s</td>", addIcon);
    }
    rs->println("</tr>");
  }

  rs->println("</table>");
  rs->println("<button id=\"sendbutton\" onclick=\"submitRampData()\" disabled>Update Data</button></div>");
  rs->print("<div id=\"rgraph\" class=\"wrapper flex\"></div>");

  // Add a graph!
  // Add the plotly javascript
  rs->print("<script src=\"plotly.js\" charset=\"utf-8\"></script>");
  // This uses plotly to display the ramp plan.
  rs->println("<script src=\"/rampPlan.js\"></script>");
  // This supports table operations and sends data to the server when the button is pushed.
  rs->println("<script src=\"/tableScript.js\"></script>");

  // Add the start time and magic word area.
  rs->println("<div class=\"wrapper flex\" id=\"tableextras\">");
    // Show the start time, if specified.
  if (relativeStart) {
    int hr = (int)(relativeStartTime / 60);
    int min = relativeStartTime - hr * 60;
    rs->printf("The current ramp start time is <input type=\"text\" id=\"StartTime\" name=\"StartTime\" value=\"%d:", hr);
    if (min < 10) {
      rs->print("0");
      //if (min < 1) rs->print("0");
    }

    rs->printf("%d\"> in 24 hour time.<br><br>\n", min);
    rs->print("<b>WARNING: changes will apply immediately, interrupting any ramp in progress.</b><br>");
    rs->print("Enter times in HH:MM or H:MM format only.  Seconds are not supported.<br>");

    rs->print("The \"Magic Word\" is not a secure password.  It is there to make you think twice.<br>");
    rs->println("<label for=\"MagicWord\">Magic Word:</label>");
    rs->println("<input type=\"text\" id=\"MagicWord\" name=\"MagicWord\" onblur=\"changedMagic(this)\"></div>");

    //rs->println("<input type=\"submit\" value=\"Change Start time\"><br><br>");

  } else {
    rs->println("Times in the ramp plan represent time of day.<br>");
  }

  rs->println(linkList);
  rs->println("</body></html>");
}

// Send minutes as hh:mm, e.g. 605 becomes 10:05
void sendAsHM(unsigned int t, AsyncResponseStream *rs) {
  if (t < 10 * 60) rs->print("0");
  rs->print((int)floor(t / 60));
  rs->print(":");
  if (t % 60 < 10) rs->print("0");
  rs->print(t % 60);
}


/**
 * This receives the data from a ramp plan update request.  It is parsed and validated,
 * and if that passes the Settings.ini file (after backup) is updated and the
 * running ramp is adjusted to match.
 *
 * The incoming data looks like:
 *  [{"time":"00:00","temp1":"52.00","temp2":"5.00","temp3":"5.00","temp4":"5.00"},
 *   {"time":"00:01","temp1":"5.00","temp2":"52.00","temp3":"52.00","temp4":"52.00"},
 *   {"time":"00:02","temp1":"52.00","temp2":"5.00","temp3":"5.00","temp4":"5.00"},
 *   {"time":"00:03","temp1":"5.00","temp2":"52.00","temp3":"52.00","temp4":"52.00"},
 *   {"startTime":"11:54"},
 *   {"magicWord":""}
 *  ]
 * Be sure that we handle NT != 4.
 *
 * Rather than writing a general JSON parser, which can be complex, just assume that
 * this data order will be consistent and parse the String manually.
 *
 * TODO: there is some validity checking, but another coding pass to catch more cases is needed.
 */
boolean receivePlanJSON(String js, AsyncResponseStream *rs) {
  Serial.printf("Received JSON string:\n%s.\n", js.c_str());
  //Serial.println("012345678901234567890123456789012345678901234567890123456789012345678901234567890");
  //Serial.println("0---------1---------2---------3---------4---------5---------6---------7---------8");

  String val;

  unsigned int minutes[MAX_RAMP_STEPS];  // Time for each ramp step.
  int hundredths[NT][MAX_RAMP_STEPS];    // Temperatures in 1/100 degree, because that takes half the storage of a float.
  int step = 0;
  int n = 0;
  int colon = 0;
  int startIndex = 0, endIndex = 0, newStart = 0;
  // Get all rows starting with time  indexOf returns -1 if not found.
  while ((startIndex = js.indexOf("\"time\"", endIndex) + 8) > 8) {

    // Get the time
    endIndex = js.indexOf("\",", startIndex);  // to next comma
    val = js.substring(startIndex, endIndex);  // Time as HH:MM or H:MM.
    Serial.printf("  debug 2 found %s time at %d for step %d\n", val, startIndex, step);

    colon = val.indexOf(":");
    minutes[step] = val.substring(0, colon).toInt() * 60 + val.substring(colon + 1).toInt();
    // Get NT temperatures, always working forward from the last position.
    // This was a list of single key:value pairs, but now we have one key, tempList
    // and a list (array).
    startIndex = js.indexOf("\"tempList", endIndex);
    startIndex += 13;  // past the name and punctuation
    for (n = 0; n < NT; n++) {
      endIndex = js.indexOf("\"", startIndex);
      val = js.substring(startIndex, endIndex);
      hundredths[n][step] = (int)(val.toFloat() * 100);
      startIndex = endIndex + 3;  // past ","  Not used after the last pass.
    }
    Serial.println();

    ///////////           request->send(200, "text/plain", "true");
    if (n != NT) {
      rs->printf("Found only %d temperatures for %d tanks.", n, NT);
      Serial.printf("Found only %d temperatures for %d tanks.\n", n, NT);
      return false;
    }
    step++;
  }
  // Serial.printf("debug 3 step (row) count is %d\n", step);

  if (step < 2) {
    rs->println("There must be at least two temperature input lines.");
    Serial.println("There must be at least two temperature input lines.");
    return false;
  } else if (MAX_RAMP_STEPS < step) {
    rs->printf("There are more than %d temperature input lines. Use fewer steps or recompile with a larger MAX_RAMP_STEPS.\n", MAX_RAMP_STEPS);
    Serial.printf("There are more than %d temperature input lines. Use fewer steps or recompile with a larger MAX_RAMP_STEPS.\n", MAX_RAMP_STEPS);
    return false;
  } else {
    // Save the new step count.
    rampSteps = step;
  }
  // No more times. Now get the start time and magic word.
  // These start from the front of the string so order doesn't matter.
  startIndex = js.indexOf("\"startTime\"") + 13;
  endIndex = 1 + js.indexOf("\"}", startIndex);
  val = js.substring(startIndex, endIndex);
  if (val.length() == 0 || val.indexOf(":") <= 0
      || val.indexOf(":") >= val.length() - 1) {
    rs->printf("Start time \"%s\" must be in H:MM or H:MM form.", val);
    Serial.printf("Start time \"%s\" must be in H:MM or H:MM form.", val);
    return false;
  }

  newStart = val.substring(0, colon).toInt() * 60 + val.substring(colon + 1).toInt();  // Minutes since midnight
  if (newStart == 0) {
    // rs->println("Ramp start time is midnight.  Be sure this is what you want.");
    Serial.println("Ramp start time is midnight.  Be sure this is what you want.");
  } else if (newStart >= 24 * 60) {
    rs->printf("Ramp start time in minutes (%d) is more than the minutes in a day!", newStart);
    Serial.printf("Ramp start time in minutes (%d) is more than the minutes in a day!", newStart);
    return false;
  }

  // Magic word
  startIndex = js.indexOf("\"magicWord\"") + 13;
  endIndex = js.indexOf("\"}", startIndex);
  val = js.substring(startIndex, endIndex);
  if (!val.equals(MAGICWORD)) {
    rs->println("{\"msg\":\"Magic word missing or incorrect.\"}");
    Serial.println("Magic word missing or incorrect.");
    return false;
  }

  // Now we trust the input.  Change arrays and save safely to Settings.ini.
  int j;
  for (int i = 0; i < step; i++) {
    rampMinutes[i] = minutes[i];
    for (j = 0; j < NT; j++) {
      rampHundredths[j][i] = hundredths[j][i];
    }
  }
  relativeStartTime = newStart;

  // Everything is updated. Commit to file in case of restarts.
  if (rewriteSettingsINI()) {
    Serial.println("===== Successful update of ramp plan! =====");
    return true;
  } else {
    rs->println("Failed to save Settings.ini of CBASS-32!  Modify card manually if necessary.");
    Serial.println("Failed to save Settings.ini of CBASS-32!  Modify SD card manually if necessary.");
    return false;
  }
}

/**
 * Generate a table with temperatures for each of NT tanks.  Values
 * will be filled in by the javascript in the page.
 */
String tableForNT() {
  Serial.println("In tableForNT");
  String s = "<table>\n<tr><td>Current T:</td>";

  // String objects seem to be better tolerated with ESP32 than the old processors,
  // but it still seems bad to create and destroy a String for each column.  Instead
  // go to the trouble of making a single buffer output for each supported number of tanks,
  // currently 1 to 8.
  // The compiler should be able to reduce this to a single line since NT is fixed.
  switch (NT) {
    case 1: s += "<td id=\"temp1\">T1</td>"; break;
    case 2: s += "<td id=\"temp1\">T1</td><td id=\"temp2\">T2</td>"; break;
    case 3: s += "<td id=\"temp1\">T1</td><td id=\"temp2\">T2</td><td id=\"temp3\">T3</td>"; break;
    case 4: s += "<td id=\"temp1\">T1</td><td id=\"temp2\">T2</td><td id=\"temp3\">T3</td><td id=\"temp4\">T4</td>"; break;
    case 5: s += "<td id=\"temp1\">T1</td><td id=\"temp2\">T2</td><td id=\"temp3\">T3</td><td id=\"temp4\">T4</td><td id=\"temp5\">T5</td>"; break;
    case 6: s += "<td id=\"temp1\">T1</td><td id=\"temp2\">T2</td><td id=\"temp3\">T3</td><td id=\"temp4\">T4</td><td id=\"temp5\">T5</td><td id=\"temp6\">T6</td>"; break;
    case 7: s += "<td id=\"temp1\">T1</td><td id=\"temp2\">T2</td><td id=\"temp3\">T3</td><td id=\"temp4\">T4</td><td id=\"temp5\">T5</td><td id=\"temp6\">T6</td><td id=\"temp7\">T7</td>"; break;
    case 8: s += "<td id=\"temp1\">T1</td><td id=\"temp2\">T2</td><td id=\"temp3\">T3</td><td id=\"temp4\">T4</td><td id=\"temp5\">T5</td><td id=\"temp6\">T6</td><td id=\"temp7\">T7</td><td id=\"temp8\">T8</td>"; break;
    default: s += "<td colspan=\"2\">WARNING: unsupported tank count</td>";
  }
  Serial.println("In tableForNT 2");

  s += "<td id=\"cbassTime\">CBASS Time: 0:00</td></tr>\n<tr><td>Target T:</td>";
  // Same thing for the target temperatures.
  switch (NT) {
    case 1: s += "<td id=\"set1\">T1</td>"; break;
    case 2: s += "<td id=\"set1\">T1</td><td id=\"set2\">T2</td>"; break;
    case 3: s += "<td id=\"set1\">T1</td><td id=\"set2\">T2</td><td id=\"set3\">T3</td>"; break;
    case 4: s += "<td id=\"set1\">T1</td><td id=\"set2\">T2</td><td id=\"set3\">T3</td><td id=\"set4\">T4</td>"; break;
    case 5: s += "<td id=\"set1\">T1</td><td id=\"set2\">T2</td><td id=\"set3\">T3</td><td id=\"set4\">T4</td><td id=\"set5\">T5</td>"; break;
    case 6: s += "<td id=\"set1\">T1</td><td id=\"set2\">T2</td><td id=\"set3\">T3</td><td id=\"set4\">T4</td><td id=\"set5\">T5</td><td id=\"set6\">T6</td>"; break;
    case 7: s += "<td id=\"set1\">T1</td><td id=\"set2\">T2</td><td id=\"set3\">T3</td><td id=\"set4\">T4</td><td id=\"set5\">T5</td><td id=\"set6\">T6</td><td id=\"set7\">T7</td>"; break;
    case 8: s += "<td id=\"set1\">T1</td><td id=\"set2\">T2</td><td id=\"set3\">T3</td><td id=\"set4\">T4</td><td id=\"set5\">T5</td><td id=\"set6\">T6</td><td id=\"set7\">T7</td><td id=\"set8\">T8</td>"; break;
    default: s += "<td colspan=\"2\">WARNING: unsupported tank count</td>";
  }
  s += "<td id=\"updateTime\">Last Update: 0:00</td></tr>\n</table>";
  Serial.println("In tableForNT 3");

  return s;
}

/**
 * Return a form control for specifying the target directory of an upload.  For now,
 * include root, the first level of subdirectories, and "new", which will accept a
 * typed name to be placed under root.
 */
#ifdef ALLOW_UPLOADS
String directoryInput() {
  // dirPath is a global containing the directory to list.
  Serial.print("In directoryInput.\n");
  const int maxOutputBuffer = 2048;
  char outputBuffer[maxOutputBuffer];  // The ugly process of building a String using sprintf or snprintf needs a buffer.
  File32 root;
  root.open("/");
  if (!root) {
    Serial.printf("Path >%s< not opened.\n", dirPath);
    sprintf(outputBuffer, "The specified path, \"%s\" could not be opened.", dirPath);
    return String(outputBuffer);
  }
  if (!root.isDirectory()) {
    Serial.printf("Path >%s< is not a directory.\n", dirPath);
    sprintf(outputBuffer, "The specified path, \"%s\" is not a directory.", dirPath);
    return String(outputBuffer);
  }

  // The output is a drop-down list
  String rString = "<label for=\"dirs\">Choose the target directory:</label><select name=\"dirChoices\" id=\"dirChoices\" onchange=updateNewName()>\n";
  rString += "<option value=\"/\">/, the base directory</option>\n";

  File32 file;
  int pass = 0;
  while (file.openNext(&root, O_RDONLY)) {
    getFileName(file);  // Put the name into fnBuffer;
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.println(fnBuffer);
      sprintf(outputBuffer, "<option value=\"%s\">%s</option>\n", fnBuffer, fnBuffer);
      rString += String(outputBuffer);
    }

    // SD.h way: file = root.openNextFile();
    file.close();  // Even though we call a method on file onec each pass, examples close it each time.
  }



  rString += "<option value=\"--new--\">new directory</option></select>\n<br><label for=\"newdir\">New directory:</label><input type=\"text\" id=\"newdir\" name=\"newdir\" disabled><br>\n";

  file.close();
  root.close();
  //Serial.println("Before return, rString is:");
  //Serial.println(rString);
  return rString;

}
#endif

/**
 * This function simply returns the current CBASS date and time.
 */
String showDateTime() {
  return getdate() + " " + gettime();
}

/**
 * This defines the replacements for any text between ~ characters in web templates.
 * This if/else structure isn't very efficient, but we don't make tons of calls and
 * it can help to put the most-used replacements first.
 *
 * NOTE: the default delimiter for substitution strings is "%", but that is too
 * commonly used.  "@" was used until needed for CSS.  Now we have "~" in WebResponseImpl.h.
 */
String processor(const String &var) {
  static char nt[5];  // Normally holds a 1 or 2-digit number, plus terminator.
  if (strlen(nt) == 0) {
    // Since nt is static, this only has to run once.
    sprintf(nt, "%d", NT);
  }
  if (var == "ERROR_MSG") return p_message;
  else if (var == "TITLE") {
    if (p_title.isEmpty()) return String("CBASS-32");
    else return p_title;
  } else if (var == "LINKLIST") return linkList;
  else if (var == "ROLLLOG") return rollLog();
  else if (var == "DIRLIST") return sendFileInfo();
  else if (var == "NT") return nt;
  else if (var == "IP") return myIP.toString();
  else if (var == "TABLE_NT") return tableForNT();
  else if (var == "DATETIME") return showDateTime();
#ifdef ALLOW_UPLOADS
  else if (var == "DIRECTORY_CHOICE") return directoryInput();
  else if (var == "UPLOAD_LINK") return String("<li><a href=\"/UploadPage\">Upload any file.</a></li>");
#else
  else if (var == "UPLOAD_LINK" || var == "DIRECTORY_CHOICE")   return String("");
#endif
  // If the whole if-else falls through return an empty String.
  return String();
}

/**
 * Pause logging to reduce conflicts, but also set a marker so we know how long it
 * has been blocked.
 */
void pauseLogging(boolean a) {
  if (a) {
    if (!logPaused) {
      startPause = millis();
      logPaused = true;  // Don't let a new log line start.
      delay(100);  // Let any in-progress log line complete.
    }
  } else {
    startPause = 0;
    logPaused = false;
    tftPauseWarning(false);
  }
}
