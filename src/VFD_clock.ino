#include <SPI.h>
#include <FS.h>
#include <time.h>                      
#include <sys/time.h>                  
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>        //https://github.com/tzapu/WiFiManager
#include <Ticker.h>  
#include "datetime_util.h"

time_t now; 
#define UTC_START  1563321600   // (UTC) 17/07/2019 00:00:00
#define STR_LENGHT 20

#define  BLANK    D0
#define  LOAD     D8
#define  CLK      D5
#define  DATA     D7

#define  START_AP D3
#include "vfd_max6921.h"

Ticker refresh, NTPSync, timeTicker, adcTicker;
struct tm lt;
uint8_t Day, Year, Month, Hour, Min, Sec;

// Used in many main loop functions to format text for display
char display_buf[STR_LENGHT];
bool shouldSaveConfig = false;
bool startCaptive = false;

WiFiEventHandler connectedHandler;
WiFiEventHandler disconnectedHandler;
WiFiEventHandler gotIPHandler;

// Configurable options
String timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";     // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.json 
bool dimLight = true, dayView = true;               // Auto-adjust brightness; show day and month
uint16_t brightness = 75;                           // Target value
#include "captive.h"

uint32_t vfd_refresh_time;
bool risingEdge = false, showDay = false ;

#define NUM_SAMPLES 30
uint16_t dimValue;                          // PWM value

void setup(){        
  time_t rtc = UTC_START;
  timeval tv = { rtc, 0 };
  timezone tz = { 0, 0 };
  settimeofday(&tv, &tz);
   
  pinMode(DATA, OUTPUT);
  pinMode(CLK, OUTPUT);
  pinMode(LOAD, OUTPUT);
  pinMode(BLANK, OUTPUT);  
  digitalWrite(DATA, LOW);
  digitalWrite(CLK, LOW);
  digitalWrite(LOAD, LOW);
  digitalWrite(BLANK, LOW);
  analogWriteFreq(5000);
  
  pinMode(START_AP, INPUT_PULLUP);
  Serial.begin(115200);
  Serial.println();  

  //Initialize the hardware SPI
  SPI.pins(CLK, D6, DATA, LOAD);
  SPI.begin();         
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));

  // Read config file if exist.
  readConfig();
  
  // Start WiFi connection in "station mode"
  connectedHandler = WiFi.onStationModeConnected(&onConnected);
  gotIPHandler = WiFi.onStationModeGotIP(&onGotIP);  

  startTickers();   
  //test_segments_order();
}





void loop() {    
  if(digitalRead(START_AP) == LOW) {
    delay(200) ;    // small delay for debounce button
    startCaptive = true;    
  }
  if (startCaptive)  captivePortal();  
  if (shouldSaveConfig)  saveConfig();

  if(micros() - vfd_refresh_time > 500){
     vfd_refresh_time = micros();
     vfd_refresh();
  }


  if(Sec > 0 && Sec < 5  && dayView){                   
     showDay = true;
     sprintf(display_buf, "%02d %s %04d\0", Day, month_abbrevs[Month - 1], 1900+Year);
     if(!risingEdge){
       risingEdge = true;
       dayIndex = -9;
  
       for(uint8_t i=0; i<STR_LENGHT; i++){
         if(display_buf[i] == '\0'){
           dayStrlen = i;
           Serial.print("String lenght: ");
           Serial.println(dayStrlen);
           break;
         }      
       }        
     } 
     Sec < 2 ? vfd_set_string(display_buf, 0) : vfd_scroll_string(display_buf, 175); 
  } 
   else {
    showDay = false;
    risingEdge = false;
   }
}




void dateTimeUpdate(){
  if(WiFi.status() == WL_CONNECTED)
      Serial.print("*");

  now = time(nullptr);      
  lt = *localtime(&now);
  char buf[30];
  strftime(buf, sizeof(buf), " %d/%m/%Y - %H:%M:%S", &lt);     
  Year = lt.tm_year;  
  Month = lt.tm_mon;
  Day = lt.tm_mday;
  Hour = lt.tm_hour;
  Min = lt.tm_min;
  Sec = lt.tm_sec;      

  //sprintf(display_buf, "%02d.%02d.%02d", 23, 35, 56);  // 23.35.56 only for test
  if (!showDay){
    snprintf(display_buf, sizeof(display_buf), "%02d.%02d.%02d", Hour, Min, Sec);
    vfd_set_string(display_buf, 0); 
  }

}

void readLight(){
  uint8_t count;
  static uint16_t lightSamples[NUM_SAMPLES];  // Ambient light samples
  static uint16_t sample_index = 0;
  float ambientLight;                         // Ambient light
  
  // Add a new sample
  lightSamples[sample_index] = analogRead(A0);
  sample_index = (++sample_index) % NUM_SAMPLES;
  for(count=0; count<NUM_SAMPLES; count++)
    ambientLight += lightSamples[count];
  ambientLight /= count;  
  ambientLight *= (brightness/100);
  
  // Set brightness or LCD light always ON in not enabled
  dimValue = constrain(ambientLight, 0, 1023);
  dimLight = false;
  dimLight ? analogWrite(BLANK, 1023 - dimValue) : digitalWrite(BLANK, LOW);  
}

void startTickers() {         
  // This ticker will update local time every 1000ms
  timeTicker.attach_ms(1000, dateTimeUpdate );

  // This ticker will sync local time with NTP once a day
  NTPSync.attach(86400L, []() {
    // TEST with NetTime http://www.timesynctool.com/ 
    //configTime(0, 0, "192.168.43.11", "192.168.43.11", "192.168.43.11"); 
    configTime(0, 0, "pool.ntp.org", "time.google.com", "time.windows.com");    
    Month = lt.tm_mon;
    Year = lt.tm_year;
  });

  adcTicker.attach(5, readLight);
}
