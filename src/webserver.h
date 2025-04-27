#include <FS.h>
#include <LittleFS.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <esp-fs-webserver.h>   // https://github.com/cotestatnt/esp-fs-webserver

#define PORTAL_TIMEOUT 600000  // 600 seconds

FSWebServer myWebServer(LittleFS, 80);

bool runApPortal = false;   // Flag to check if we are in captive portal mode
uint32_t portalStartTime = 0; 

extern String timeZone; 
extern bool dimLight;
extern int16_t lightOffset;
extern bool dayView;   

// Var labels (in /setup webpage)
#define TIME_ZONE       "Stringa \"Time Zone\""
#define DIM_LIGHT       "Regola luminosità automaticamente"
#define OFFSET_LIGHT    "Offset luminosità display"
#define MONTH_DATE      "Mostra data e mese"


////////////////////////////////  Filesystem  /////////////////////////////////////////
void startFilesystem() {
  // FILESYSTEM INIT
  if ( !LittleFS.begin()) {
    Serial.println("ERROR on mounting filesystem. It will be formmatted!");
    LittleFS.format();
    ESP.restart();
  }
  myWebServer.printFileList(LittleFS, Serial, "/", 2);
}

void getFsInfo(fsInfo_t* fsInfo) {
	fsInfo->fsName = "LittleFS";
}


////////////////////  Load application options from filesystem  ////////////////////
bool loadOptions() {
  if (LittleFS.exists(myWebServer.getConfigFilepath())) {
    // Config file will be opened on the first time we call this method
    // myWebServer.getOptionValue(TIME_ZONE, timeZone);
    // myWebServer.getOptionValue(DIM_LIGHT, dimLight);
    // myWebServer.getOptionValue(OFFSET_LIGHT, lightOffset);
    // myWebServer.getOptionValue(MONTH_DATE, dayView);

    // Serial.println(F("Application options loaded."));
    Serial.printf(TIME_ZONE ": %s\n", timeZone.c_str());
    Serial.printf(DIM_LIGHT ": %s\n", dimLight ? "true" : "false"); 
    Serial.printf(OFFSET_LIGHT ": %d\n", lightOffset);
    Serial.printf(MONTH_DATE ": %s\n", dayView ? "true" : "false");
    return true;
  }
  else {
    Serial.println(F("Config file not exist"));
  }
  return false;
}



void setupWebserver() {
    // FILESYSTEM INIT
    startFilesystem();

    // Load configuration (if not present, default will be created when webserver will start)
    loadOptions();
  
    // Try to connect to stored SSID, start AP if fails after timeout
    myWebServer.setAP("VFD_CLOCK", "123456789");
    IPAddress myIP = myWebServer.startWiFi(15000);
    if (myWebServer.getAPMode()) {
        Serial.println(F("AP started"));
        runApPortal = true;
        Serial.println(F("AP started"));
    }
    else {
      Serial.println(F("Connected to WiFi: "));
    }

    // Configure /setup page and start Web Server
    // myWebServer.addOptionBox("Options");
    // myWebServer.addOption(TIME_ZONE, timeZone);
    // myWebServer.addOption(DIM_LIGHT, dimLight);
    // myWebServer.addOption(OFFSET_LIGHT, lightOffset);
    // myWebServer.addOption(MONTH_DATE, dayView);
    // Enable ACE FS file web editor and add FS info callback function
    myWebServer.enableFsCodeEditor(getFsInfo);
    // start web server
    myWebServer.begin();
    portalStartTime = millis();
    Serial.print(F("IP Address: "));
    Serial.println(myIP);
  }
  
