/*
 * ESP8266 GPS UDP Tracker
 * Version: 1.1
 * 
 * Copyright (C) 2025 Scops Owl Designs (Sc0ps)
 * Email: ScopsOwlDesigns@gmail.com
 * Repository: https://github.com/sc0ps/ESP8266-GPS-UDP-tracker
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>

// ===================== Configuration =====================
struct ConfigData {
  String ssid;
  String password;
  String udpAddress;
  int udpPort;
  int ledPin;
  int gpsRx;
  int gpsTx;
};

ConfigData config;

// ===================== Globals =====================
ESP8266WebServer server(80);
WiFiUDP udp;
SoftwareSerial* gpsSerial = nullptr;

bool wifiConnected = false;
bool gpsFail = false;
unsigned long lastGPS = 0;
unsigned long lastHeartbeat = 0;

// ===================== Helpers =====================
void setDefaultPinsIfUnset() {
  if (config.ledPin <= 0) config.ledPin = 2;   // D4
  if (config.gpsRx <= 0)  config.gpsRx  = 13;  // D7
  if (config.gpsTx <= 0)  config.gpsTx  = 12;  // D6
}

bool loadConfig() {
  config.ssid = ""; config.password = "";
  config.udpAddress = ""; config.udpPort = 5050;
  config.ledPin = 2; config.gpsRx = 13; config.gpsTx = 12;

  if (!LittleFS.exists("/config.json")) return false;
  File f = LittleFS.open("/config.json", "r");
  if (!f) return false;

  JsonDocument doc;
  if (deserializeJson(doc, f)) { f.close(); return false; }
  f.close();

  config.ssid       = doc["ssid"]       | config.ssid;
  config.password   = doc["password"]   | config.password;
  config.udpAddress = doc["udpAddress"] | config.udpAddress;
  config.udpPort    = doc["udpPort"]    | config.udpPort;
  config.ledPin     = doc["ledPin"]     | config.ledPin;
  config.gpsRx      = doc["gpsRx"]      | config.gpsRx;
  config.gpsTx      = doc["gpsTx"]      | config.gpsTx;

  setDefaultPinsIfUnset();
  return true;
}

bool saveConfig() {
  JsonDocument doc;
  doc["ssid"]       = config.ssid;
  doc["password"]   = config.password;
  doc["udpAddress"] = config.udpAddress;
  doc["udpPort"]    = config.udpPort;
  doc["ledPin"]     = config.ledPin;
  doc["gpsRx"]      = config.gpsRx;
  doc["gpsTx"]      = config.gpsTx;

  File f = LittleFS.open("/config.json", "w");
  if (!f) return false;
  serializeJson(doc, f);
  f.close();
  return true;
}

void connectWiFi() {
  if (config.ssid.isEmpty()) return;
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.ssid.c_str(), config.password.c_str());
  Serial.printf("Connecting to Wi-Fi: %s\n", config.ssid.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500); Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.printf("\nWi-Fi connected. IP: %s\n",
                  WiFi.localIP().toString().c_str());
  } else {
    wifiConnected = false;
    Serial.println("\nWi-Fi connection failed.");
  }
}

void beginGPSSerial() {
  if (gpsSerial) { gpsSerial->end(); delete gpsSerial; gpsSerial = nullptr; }
  gpsSerial = new SoftwareSerial(config.gpsRx, config.gpsTx, false);
  gpsSerial->begin(9600);
  Serial.printf("GPS Serial on RX=%d, TX=%d @9600 baud\n",
                config.gpsRx, config.gpsTx);
}

// ===================== Web Server =====================
void setupServer() {
  // Mount LittleFS
  if (!LittleFS.begin()) {
    Serial.println("⚠️  LittleFS mount failed, formatting...");
    LittleFS.format();
    if (!LittleFS.begin()) {
      Serial.println("❌ LittleFS still failed after format!");
      return;
    }
  }
  Serial.println("✅ LittleFS mounted successfully.");

  // Serve all static files
  server.serveStatic("/", LittleFS, "/");

  // Root or 404 handler (no redirect-loop)
  server.onNotFound([]() {
    String path = server.uri();
    if (path == "/" || path == "/index") {
      server.sendHeader("Location", "/index.html", true);
      server.send(302, "text/plain", "");
    } else {
      server.send(404, "text/plain",
                  "File Not Found: " + path + "\n");
    }
  });

  // --- API Endpoints ---
  server.on("/getConfig", HTTP_GET, []() {
    JsonDocument doc;
    doc["ssid"]       = config.ssid;
    doc["password"]   = config.password;
    doc["udpAddress"] = config.udpAddress;
    doc["udpPort"]    = config.udpPort;
    doc["ledPin"]     = config.ledPin;
    doc["gpsRx"]      = config.gpsRx;
    doc["gpsTx"]      = config.gpsTx;
    String json; serializeJson(doc, json);
    server.send(200, "application/json", json);
  });

  server.on("/saveConfig", HTTP_POST, []() {
    JsonDocument doc;
    if (deserializeJson(doc, server.arg("plain"))) {
      server.send(400, "text/plain", "Invalid JSON"); return;
    }
    config.ssid       = doc["ssid"]       | config.ssid;
    config.password   = doc["password"]   | config.password;
    config.udpAddress = doc["udpAddress"] | config.udpAddress;
    config.udpPort    = doc["udpPort"]    | config.udpPort;
    config.ledPin     = doc["ledPin"]     | config.ledPin;
    config.gpsRx      = doc["gpsRx"]      | config.gpsRx;
    config.gpsTx      = doc["gpsTx"]      | config.gpsTx;
    setDefaultPinsIfUnset();

    if (saveConfig()) {
      server.send(200, "text/plain", "Configuration saved. Rebooting...");
      delay(1000); ESP.restart();
    } else {
      server.send(500, "text/plain", "Failed to save configuration.");
    }
  });

  server.on("/factoryReset", HTTP_POST, []() {
    if (LittleFS.exists("/config.json")) {
      LittleFS.remove("/config.json");
      server.send(200, "text/plain", "Factory reset complete. Rebooting...");
      delay(1000); ESP.restart();
    } else {
      server.send(200, "text/plain", "No configuration found to reset.");
    }
  });

  // Optional diagnostic: list files
  server.on("/list", HTTP_GET, []() {
    Dir dir = LittleFS.openDir("/");
    String out = "Files on LittleFS:\n";
    while (dir.next())
      out += " - " + dir.fileName() +
             " (" + String(dir.fileSize()) + " bytes)\n";
    server.send(200, "text/plain", out);
  });

  server.begin();
  Serial.println("🌐 Web server started.");
}

// ===================== Setup =====================
void setup() {
  Serial.begin(115200);
  Serial.println("\nGPS UDP Tracker v1.2 Booting...");

  bool hasConfig = false;
  if (LittleFS.begin()) hasConfig = loadConfig();
  else Serial.println("LittleFS mount failed.");

  setDefaultPinsIfUnset();
  pinMode(config.ledPin, OUTPUT);
  digitalWrite(config.ledPin, HIGH);
  beginGPSSerial();

  if (hasConfig && !config.ssid.isEmpty()) connectWiFi();
  else {
    Serial.println("Starting Access Point...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("gps_tracker_setup", "12345678");
    Serial.printf("AP active: SSID=gps_tracker_setup  IP=%s\n",
                  WiFi.softAPIP().toString().c_str());
  }

  setupServer();
  udp.begin(config.udpPort);
  Serial.printf("UDP listening on port %d\n", config.udpPort);
  lastGPS = millis();
}

// ===================== Loop =====================
void loop() {
  static String gpsBuffer = "";
  while (gpsSerial && gpsSerial->available()) {
    char c = gpsSerial->read();
    Serial.write(c);
    if (c == '\n') {
      if (wifiConnected && !config.udpAddress.isEmpty()) {
        udp.beginPacket(config.udpAddress.c_str(), config.udpPort);
        udp.print(gpsBuffer + "\n");
        udp.endPacket();
      }
      gpsBuffer = "";
      lastGPS = millis();
      gpsFail = false;
    } else if (isPrintable(c)) gpsBuffer += c;
  }

  if (millis() - lastGPS > 30000) gpsFail = true;

  static unsigned long lastBlink = 0;
  static bool ledState = HIGH;

  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    digitalWrite(config.ledPin, HIGH);
    connectWiFi();
  } else {
    wifiConnected = true;
    if (gpsFail) {
      if (millis() - lastBlink >= 1000) {
        lastBlink = millis();
        ledState = !ledState;
        digitalWrite(config.ledPin, ledState ? LOW : HIGH);
      }
    } else if (millis() - lastHeartbeat >= 1000) {
      lastHeartbeat = millis();
      digitalWrite(config.ledPin, LOW);
      delay(50);
      digitalWrite(config.ledPin, HIGH);
    }
  }

  server.handleClient();
}
