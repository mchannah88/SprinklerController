// ---------------------------------------------------
// SprinklerController.ino
// ---------------------------------------------------

// 1. LIBRARIES
// --------------
#include <WebServer.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <time.h> // Library for time-keeping

// 2. SETTINGS
// -------------
// Replace with your network credentials
const char* ssid = "Charlotte's Interwebs";
const char* password = "bumblebee298";

// GPIO pins for the 5 sprinkler zones
const int zonePins[5] = {23, 22, 21, 19, 18};
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;
const char* tz_info = "EST5EDT,M3.2.0/2,M11.1.0/2"; // Example: US Eastern Time

// 3. GLOBAL OBJECTS & VARIABLES
WebServer server(80);       // CHANGED: WebServer object
JsonDocument scheduleDoc;
unsigned long last_check = 0;
time_t zoneStopTime[5] = {0, 0, 0, 0, 0};
bool timeSynchronized = false;

// 4. HELPER FUNCTIONS
// --- (Full function code below for clarity) ---
void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An error has occurred while mounting LittleFS");
  }
  Serial.println("LittleFS mounted successfully");
}
void loadSchedule() {
  File file = LittleFS.open("/schedule.json", "r");
  if (!file || file.size() == 0) {
    Serial.println("Failed to open schedule.json for reading, or file is empty. Using default.");
    deserializeJson(scheduleDoc, "{}");
  } else {
    DeserializationError error = deserializeJson(scheduleDoc, file);
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
    } else {
      Serial.println("Schedule loaded successfully.");
    }
  }
  file.close();
}
void saveSchedule() {
  File file = LittleFS.open("/schedule.json", "w");
  if (!file) {
    Serial.println("Failed to open schedule.json for writing");
    return;
  }
  if (serializeJson(scheduleDoc, file) == 0) {
    Serial.println("Failed to write to file");
  } else {
    Serial.println("Schedule saved successfully.");
  }
  file.close();
}


// 5. SETUP
void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 5; i++) {
    pinMode(zonePins[i], OUTPUT);
    digitalWrite(zonePins[i], LOW);
  }
  initLittleFS();
  loadSchedule();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  setenv("TZ", tz_info, 1);
  tzset();
  Serial.println("Time configuration started. Will sync in main loop.");

  // --- CHANGED: Web server handlers for standard WebServer ---
  server.on("/", HTTP_GET, []() {
    File file = LittleFS.open("/index.html", "r");
    if (file) {
      server.streamFile(file, "text/html");
      file.close();
    } else {
      server.send(404, "text/plain", "404: File Not Found");
    }
  });
  
  server.on("/style.css", HTTP_GET, []() {
    File file = LittleFS.open("/style.css", "r");
    if (file) {
      server.streamFile(file, "text/css");
      file.close();
    }
  });

  server.on("/script.js", HTTP_GET, []() {
    File file = LittleFS.open("/script.js", "r");
    if (file) {
      server.streamFile(file, "text/javascript");
      file.close();
    }
  });
  
  server.on("/schedule", HTTP_GET, []() {
    String json;
    serializeJson(scheduleDoc, json);
    server.send(200, "application/json", json);
  });
  
  server.on("/schedule", HTTP_POST, []() {
    if (server.hasArg("plain") == false) {
      server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Body required\"}");
      return;
    }
    String body = server.arg("plain");
    DeserializationError error = deserializeJson(scheduleDoc, body);
    if (error) {
      server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"Invalid JSON\"}");
      return;
    }
    saveSchedule();
    server.send(200, "application/json", "{\"status\":\"success\", \"message\":\"Schedule saved\"}");
  });

  server.begin();
  Serial.println("HTTP server started");
}

// 6. MAIN LOOP
void loop() {
  // CHANGED: Must call server.handleClient() in the loop for WebServer
  server.handleClient();

  // The rest of the logic is the same as before
  if (!timeSynchronized) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      timeSynchronized = true;
      Serial.println("\nTime synchronized successfully!");
      char timeStr[32];
      strftime(timeStr, sizeof(timeStr), "%A, %B %d %Y %H:%M:%S", &timeinfo);
      Serial.println(timeStr);
    } else {
      delay(500); // Give a small delay to allow server.handleClient() to run
      return;
    }
  }

  if (millis() - last_check < 1000) {
    return;
  }
  last_check = millis();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  if (timeinfo.tm_sec == 0) {
    char timeStr[32];
    strftime(timeStr, sizeof(timeStr), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    Serial.println(timeStr);
  }

  time_t now;
  time(&now);

  for (int i = 0; i < 5; i++) {
    if (zoneStopTime[i] != 0 && now >= zoneStopTime[i]) {
      digitalWrite(zonePins[i], LOW);
      zoneStopTime[i] = 0;
      Serial.printf("Zone %d: Turned OFF.\n", i + 1);
    }
  }

  if (timeinfo.tm_sec == 0) {
    for (int i = 0; i < 5; i++) {
      String zoneKey = "zone" + String(i + 1);
      if (!scheduleDoc.containsKey(zoneKey) || !scheduleDoc[zoneKey]["enabled"].as<bool>() || zoneStopTime[i] != 0) {
        continue;
      }
      JsonArray days = scheduleDoc[zoneKey]["days"];
      bool todayIsScheduled = false;
      for (int day : days) {
        if (day == timeinfo.tm_wday) {
          todayIsScheduled = true;
          break;
        }
      }
      if (todayIsScheduled) {
        String startTime = scheduleDoc[zoneKey]["startTime"];
        int scheduledHour = startTime.substring(0, 2).toInt();
        int scheduledMin = startTime.substring(3, 5).toInt();
        if (timeinfo.tm_hour == scheduledHour && timeinfo.tm_min == scheduledMin) {
          int durationMinutes = scheduleDoc[zoneKey]["duration"];
          digitalWrite(zonePins[i], HIGH);
          zoneStopTime[i] = now + (durationMinutes * 60);
          Serial.printf("Zone %d: Turned ON for %d minutes.\n", i + 1, durationMinutes);
        }
      }
    }
  }
}