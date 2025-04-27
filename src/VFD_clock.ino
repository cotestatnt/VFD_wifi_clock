#include <SPI.h>
#include <FS.h>
#include <time.h>                      
#include <sys/time.h>                  
#include <Ticker.h>  

#define TEST_SEGMENTS_ORDER 0

Ticker vfdRefresh, timeTicker, adcTicker;

#define UTC_START  1563321600   // (UTC) 17/07/2019 00:00:00
#define STR_LENGHT 20
#define ADC_NUM_SAMPLES 30

#include "vfd_max6921.h"

char display_buf[STR_LENGHT];
uint16_t dimValue = 50;                           // PWM value related to ambient light
uint32_t vfd_refresh_time;
bool risingEdge = false;
bool showDay = false ;

uint8_t Day, Year, Month, Hour, Min, Sec;
time_t now;
struct tm lt;

// Configurable options (captive portal)
String timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";     // https://github.com/nayarsystems/posix_tz_db/blob/master/zones.json 
bool dimLight = true;                               // Auto-adjust brightness 
bool dayView = true;                                // Show day and month
int16_t lightOffset = -50;                           // Offset value

// #include "captive.h"
#include "webserver.h"

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
  
  Serial.begin(115200);
  Serial.println();  

  setupWebserver();
  // configTime(3600, 3600, "time.google.com", "time.windows.com", "pool.ntp.org");
  configTime(timeZone.c_str(), "time.google.com", "time.windows.com", "pool.ntp.org");

  //Initialize the hardware SPI
  SPI.pins(CLK, D6, DATA, LOAD);
  SPI.begin();         
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  
  // This ticker will update local time every 1000ms  
  timeTicker.attach_ms(1000, dateTimeUpdate); 
  
  // This ticker will update the VFD every 1ms
  vfdRefresh.attach_ms(1, vfd_refresh); 
  // This ticker will update the ambient light every 5s
  adcTicker.attach(5, readLight);

#if TEST_SEGMENTS_ORDER
  // Test segments order and digit to segment mapping 
  test_segments_order();
#endif
}



void loop() {    
  if (runApPortal || millis() - portalStartTime < PORTAL_TIMEOUT ) {
    myWebServer.run();
    if (runApPortal && millis() - portalStartTime > PORTAL_TIMEOUT) {
      runApPortal = false;
      myWebServer.stop();
      WiFi.mode(WIFI_STA);
      Serial.println(F("Captive portal timeout\nSwitching to STA mode"));
    }
  }

  static uint32_t reconnecTime = millis();
  if (!runApPortal && WiFi.status() == WL_DISCONNECTED && millis() - reconnecTime > 10000) {
    Serial.println(F("WiFi disconnected. Reconnecting..."));
    reconnecTime = millis();
    WiFi.begin();
  }

  // If enabled, show day and month at the beginning of every minutes
  if(Sec > 0 && Sec < 5  && dayView){                   
     showDay = true;
     snprintf(display_buf, sizeof(display_buf), "%02d %s %04d", Day, month_abbrevs[Month], 1900+Year);

     // Reset index and calculate string length
     if(!risingEdge){
       risingEdge = true;
       dayIndex = -9;
  
       for(uint8_t i=0; i<STR_LENGHT; i++){
         if(display_buf[i] == '\0'){
           dayStrlen = i;
           break;
         }      
       }        
     } 
     // Show day and first 4 char of month for 1 second and after scroll text
     Sec < 2 ? vfd_set_string(display_buf, 0) : vfd_scroll_string(display_buf, 175); 
  } 
   else {
    showDay = false;
    risingEdge = false;
    // Show time hh.mm.ss
    snprintf(display_buf, sizeof(display_buf), "%02d.%02d.%02d", Hour, Min, Sec);
    vfd_set_string(display_buf, 0); 
   }
}


void readLight(){
  uint8_t count;
  static uint16_t lightSamples[ADC_NUM_SAMPLES];  // Ambient light samples
  static uint16_t sample_index = 0;
  long ambientLight;                              // Ambient light
  
  // Add a new sample
  lightSamples[sample_index] = analogRead(A0);
  sample_index = (sample_index + 1) % ADC_NUM_SAMPLES;
  for(count=0; count<ADC_NUM_SAMPLES; count++)
    ambientLight += lightSamples[count];
  ambientLight /= count;  
  ambientLight = analogRead(A0);
  Serial.printf("\nAmbient light: %ld (%d)", ambientLight, sample_index);
  
  // Set brightness or LCD light always ON in not enabled
  dimValue = map(ambientLight, 30, 1023, 200, 0) + lightOffset;

  // 0 Max brightness (0) to 500 Min brightness 
  dimValue = constrain(dimValue, 0, 200);

  // TEST
  // dimLight = false;
  // dimValue = 200;
  // analogWrite(BLANK, dimValue);

  Serial.printf("\nDimming value (+ %d offset): %d", lightOffset, dimValue);
  dimLight ? analogWrite(BLANK, dimValue) : digitalWrite(BLANK, LOW);  
}


void dateTimeUpdate(){
  now = time(nullptr);      
  lt = *localtime(&now);
  // char buf[30];
  // strftime(buf, sizeof(buf), " %d/%m/%Y - %H:%M:%S", &lt);    
  // Serial.printf("\nLocal time: %s", buf); 
  Year = lt.tm_year;  
  Month = lt.tm_mon;
  Day = lt.tm_mday;
  Hour = lt.tm_hour;
  Min = lt.tm_min;
  Sec = lt.tm_sec;     
}
