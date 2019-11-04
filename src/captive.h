void saveConfigCallback() {
  Serial.println("Configurazione salvata.");
  shouldSaveConfig = true;
}

void captivePortal() {  
  //WiFi.disconnect();
  startCaptive = false;
  //WiFi.mode(WIFI_AP);
  Serial.println("Start captive portal");
  WiFiManager wifiManager;
  //wifiManager.setBreakAfterConfig(true);
  //wifiManager.setDebugOutput(false);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.setConfigPortalTimeout(30);
  
  WiFiManagerParameter tzLabel("<p><br></p>Stringa \"Time Zone\"");
  WiFiManagerParameter tzLink("<a href='https://github.com/nayarsystems/posix_tz_db/blob/master/zones.json'>Elenco stringhe \"Timezone\"</a>");
  WiFiManagerParameter tzString("timeZone", "", timeZone.c_str(), 100);
  WiFiManagerParameter dimLightLabel("<br>Regola luminosità automaticamente");
  WiFiManagerParameter dimLightValue("dimLight", "Luminosità automatica", "1", 50);
  WiFiManagerParameter brightnessLabel("<br>Offset luminosità display");
  WiFiManagerParameter brightnessValue("brightness", "Offset luminosità", "100", 50);
  WiFiManagerParameter localTimeLabel("<p><br><br></p>Imposta data locale in caso di assenza di connessione<br>nel formato previsto (es. 01/07/2019 08:30:00).");
  WiFiManagerParameter localTimeValue("localTime", "01/07/2019 08:30:00", "", 80);

  WiFiManagerParameter showDateLabel("<br>Mostra data e mese");
  WiFiManagerParameter showDateValue("showdate", "Mostra giorno e mese", "1", 50);
  

  wifiManager.addParameter(&tzLabel);
  wifiManager.addParameter(&tzLink);
  wifiManager.addParameter(&tzString);
  wifiManager.addParameter(&dimLightLabel);
  wifiManager.addParameter(&dimLightValue);
  wifiManager.addParameter(&brightnessLabel);
  wifiManager.addParameter(&brightnessValue);
  wifiManager.addParameter(&localTimeLabel);
  wifiManager.addParameter(&localTimeValue);
  

  if (!wifiManager.startConfigPortal("VFD Clock")) {
    Serial.println("Failed to connect to AP");
    delay(2000);
    //restart and try again
    //ESP.restart();    
  }

  //read updated parameters         
  timeZone = tzString.getValue();
  dimLight = String(dimLightValue.getValue()).toInt();
  brightness = String(brightnessValue.getValue()).toInt();
  String localTime = localTimeValue.getValue();
  if(localTime.length() > 1){
    struct tm time;    
    /* Handle error */;
    if(strptime(localTime.c_str(), "%d/%m/%Y %H:%M:%S", &time) == NULL){
       Serial.println("Error parsing local time string. Check text.");  
    }
    time_t locTime = mktime(&time);  // timestamp in current timezone        
    timeval tv = { locTime, 0 };
    timezone tz = { 0, 0 };
    settimeofday(&tv, &tz);
  }

  Serial.println("\nUpdated parameter: ");
  Serial.println(timeZone);
  Serial.println(dimLight);
  Serial.println(brightness);
  shouldSaveConfig = true;  
}


//Read the custom parameters from FS  
void readConfig() {
  Serial.print("\nMounting FS...");
  if (SPIFFS.begin()) {
    Serial.println(" done");
    if (SPIFFS.exists("/config.json")) {
      Serial.print("Reading config file..");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);   // Allocate a buffer to store contents of the file.
        configFile.readBytes(buf.get(), size);         // Read file config.json into buffer
        DynamicJsonDocument doc(512);
        DeserializationError error = deserializeJson(doc, buf.get());
        if (!error) {
          timeZone = doc["timeZone"].as<String>();
          dimLight = doc["dimLight"];
          brightness = doc["brightness"];          
          Serial.println(" done.");
          configFile.close();
        }
        else {
          Serial.printf(" failed to parse json config file (%s)", error.c_str());
          captivePortal();
        }
      }
    }
  }
  else
    Serial.println(" failed to mount FS");
}

//save the custom parameters to FS  
void saveConfig(){  
  shouldSaveConfig = false;
  Serial.println("Saving config..");
  DynamicJsonDocument doc(512);
  doc["timeZone"] = timeZone;
  doc["dimLight"] = dimLight;
  doc["brightness"] = brightness;
    
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) { Serial.println("failed to open config file for writing"); }
  serializeJson(doc, configFile);
  serializeJson(doc, Serial);
  configFile.close();
  Serial.println(" done.\n Reading new config.json file.");  
  readConfig();
}


void disconnected(const WiFiEventStationModeDisconnected& evt) {
  Serial.println("Wait WiFi connection");      
}

void onDisconnected(const WiFiEventStationModeDisconnected& evt) {
  Serial.print("Disconnected from ");  
  Serial.println(evt.ssid);
  disconnectedHandler = WiFi.onStationModeDisconnected(&disconnected);
  startCaptive = true;
}

void onGotIP(const WiFiEventStationModeGotIP& evt) {
  Serial.print("IP address: ");  
  Serial.println(WiFi.localIP());  
  disconnectedHandler = WiFi.onStationModeDisconnected(&onDisconnected);

  // TEST with NetTime http://www.timesynctool.com/ 
  //configTime(0, 0, "192.168.43.11", "192.168.43.11", "192.168.43.11");

  configTime(0, 0, "time.google.com", "time.windows.com", "pool.ntp.org"); 
  // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.json  
  setenv("TZ", timeZone.c_str(), 1);
  tzset();
  lt = *localtime(&now);   
}

void onConnected(const WiFiEventStationModeConnected& evt) {
  Serial.print("\nConnected to ");
  Serial.println(evt.ssid);   
}
