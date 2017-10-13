#include <TM1637Display.h>
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Time.h>
 
const int CLK = D6; //Set the CLK pin connection to the display
const int DIO = D5; //Set the DIO pin connection to the display

uint8_t data[] = { 0xff, 0xff, 0xff, 0xff };

const byte digit_pattern[16] =
{
  B00111111,  // 0
  B00000110,  // 1
  B01011011,  // 2
  B01001111,  // 3
  B01100110,  // 4
  B01101101,  // 5
  B01111101,  // 6
  B00000111,  // 7
  B01111111,  // 8
  B01101111,  // 9
  B01110111,  // A
  B01111100,  // b
  B00111001,  // C
  B01011110,  // d
  B01111001,  // E
  B01110001   // F
};

int ledState = LOW;
unsigned long previousMillis = 0; 
const long interval = 1000;  
int numCounter = 0;
int minutos = 0;
int hora = 0;
int TimeZone = 2;
const char *ssid     = "WLAN_XX";
const char *password = ".999999999.";

TM1637Display display(CLK, DIO); //set up the 4-Digit Display.

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "hora.roa.es", TimeZone * 3600, 600000);

void setup(){
  
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password); 
  while (WiFi.status() != WL_CONNECTED)
  {
     delay(100);
  }
  Serial.print("\n\nConexión Wifi.\nIP address: ");
  Serial.println(WiFi.localIP());
  
  display.setBrightness(0x0a); //set the diplay to maximum brightness 0x0a
  timeClient.begin();
}


void loop() {
  
timeClient.update();

time_t t = timeClient.getEpochTime();
setTime(t);

Serial.println(timeClient.getFormattedTime());
LCDprint();

}


void LCDprint() {
  
hora = hour();
minutos = minute();

data[0] = digit_pattern[hora / 10];
data[1] = digit_pattern[hora % 10 ];
data[2] = digit_pattern[minutos / 10];
data[3] = digit_pattern[minutos % 10];

unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
    // save the last time you blinked the LED
    previousMillis = currentMillis;

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW) {
      ledState = HIGH;  
    } else {
      ledState = LOW;
           }
                                                   }
    
  if (ledState == HIGH) { data[1] = data[1] | B10000000 ; }
  display.setSegments(data);  
  yield();

}
