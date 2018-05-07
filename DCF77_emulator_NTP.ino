//  Basado en el trabajo de Michael Margolis, Tom Igoe ,Ivan Grokhotkov  DOMINIO PUBLICO
#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <Ticker.h>
#include <WiFiUdp.h>
#include <Time.h>
#include "si5351.h"
#include "Wire.h"

ESP8266WiFiMulti WiFiMulti;
Si5351 si5351; // Pines D1 SCL   D2 SDA
Ticker tickerSetLow;
const int ledPin = 2;
//complete array of pulses for three minutes
//0 = no pulse, 1=100msec, 2=200msec
int ArrayImpulsi[60];
int ContaImpulsiParziale = 0;
int Ore,Minuti,Secondi,Giorno,Mese,Anno,DayOfW;
const int timeZone = 1;     // Central European Time

int Dls;                    //DayLightSaving
// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "3.europe.pool.ntp.org", 3600, 60000);
time_t epoch ;

void setup(){
  
tickerSetLow.attach_ms(100, DcfOut );
si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
si5351.set_freq(7750000ULL, SI5351_CLK0); // La frecuencia de DCF77 es 77.5 khz
si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_6MA); // La potencia maxima de emision del si5351 es DRIVE_8MA
si5351.output_enable(SI5351_CLK0, 0) ; 

  pinMode (ledPin, OUTPUT);
  digitalWrite (ledPin, LOW);

  Serial.begin(115200);
  Serial.println("\n\nDCF77 transmisor.\n");
  WiFi.disconnect();
  
    WiFiMulti.addAP("YOURSSID", "YOURPASSWD");

    //WiFi.disconnect();
    while(WiFiMulti.run() != WL_CONNECTED) {
        delay(200);
        Serial.print ( "." );
    }

Serial.println("\n\nConexión WiFi completada.\n\n");
  
  timeClient.begin();
  timeClient.update();
 
  while ( epoch < 1464148800 ) {
    delay ( 500 );
    timeClient.update();
    epoch = timeClient.getEpochTime(); 
    Serial.print ( "*" );                 }

    si5351.output_enable(SI5351_CLK0, 1);
    Serial.println ( "\n" );
    Serial.println(timeClient.getFormattedTime());
    
}

// 1525637993

void loop() {
  
    CodificaTempo();

    delay(0);
     
}

void CodificaTempo() {

  int DayToEndOfMonth,DayOfWeekToEnd,DayOfWeekToSunday;

  epoch = timeClient.getEpochTime() + 60 ; // DCF77 envia el minuto siguiente al actual
 
   //calculate actual day to evaluate the summer/winter time of day ligh saving
    DayOfW = weekday(epoch);
    Giorno = day(epoch);
    Mese = month(epoch);
    Anno = year(epoch);

    //calcolo ora solare o legale
    Dls = 0;    //default winter time
    //From April to september we are surely on summer time
    if (Mese > 3 && Mese < 10) {
      Dls = 1;
    };
    //March, month of change winter->summer time, last last sunday of the month
    //March has 31days so from 25 included on sunday we can be in summer time
    if (Mese == 3 && Giorno > 24) {
      DayToEndOfMonth = 31 - Giorno;
      DayOfWeekToSunday = 7 - DayOfW;
      if (DayOfWeekToSunday >= DayToEndOfMonth)
        Dls = 1;
    };
    //Octobee, month of change summer->winter time, l'ultima Domenica del mese
    //Even Octobee has 31days so from 25 included on sunday we can be in winter time
    if (Mese == 10) {
      Dls = 1;
      if (Giorno > 24) {
        DayToEndOfMonth = 31 - Giorno;
        DayOfWeekToEnd = 7 - DayOfW;
        if (DayOfWeekToEnd >= DayToEndOfMonth)
        Dls = 0;
      };
    };

    if (Dls == 1)
      epoch += 3600;
  
    Ore = hour(epoch);
    Minuti = minute(epoch);
    Secondi = second(epoch);

  int n,Tmp,TmpIn;
  int ParityCount = 0;

  //i primi 20 bits di ogni minuto li mettiamo a valore logico zero
  for (n=0;n<20;n++)
    ArrayImpulsi[n] = 1;

  //DayLightSaving bit
  if (Dls == 1)
    ArrayImpulsi[17] == 2;
  else
    ArrayImpulsi[18] == 2;
    
  //il bit 20 deve essere 1 per indicare tempo attivo
  ArrayImpulsi[20] = 2;

  //calcola i bits per il minuto
  TmpIn = Bin2Bcd(Minuti);
  for (n=21;n<28;n++) {
    Tmp = TmpIn & 1;
    ArrayImpulsi[n] = Tmp + 1;
    ParityCount += Tmp;
    TmpIn >>= 1;
  };
  if ((ParityCount & 1) == 0)
    ArrayImpulsi[28] = 1;
  else
    ArrayImpulsi[28] = 2;

  //calcola i bits per le ore
  ParityCount = 0;
  TmpIn = Bin2Bcd(Ore);
  for (n=29;n<35;n++) {
    Tmp = TmpIn & 1;
    ArrayImpulsi[n] = Tmp + 1;
    ParityCount += Tmp;
    TmpIn >>= 1;
  }
  if ((ParityCount & 1) == 0)
    ArrayImpulsi[35] = 1;
  else
    ArrayImpulsi[35] = 2;
   ParityCount = 0;
  //calcola i bits per il giorno
  TmpIn = Bin2Bcd(Giorno);
  for (n=36;n<42;n++) {
    Tmp = TmpIn & 1;
    ArrayImpulsi[n] = Tmp + 1;
    ParityCount += Tmp;
    TmpIn >>= 1;
  }
  //La libreria de Arduino el Domingo es 1 y el sabado 6... pero en DCF77 el domingo es 7 !!!
  DayOfW = DayOfW - 1; if (DayOfW == 0) DayOfW = 7;
  TmpIn = Bin2Bcd(DayOfW);
  for (n=42;n<45;n++) {
    Tmp = TmpIn & 1;
    ArrayImpulsi[n] = Tmp + 1;
    ParityCount += Tmp;
    TmpIn >>= 1;
  }
  //calcola i bits per il mese
  TmpIn = Bin2Bcd(Mese);
  for (n=45;n<50;n++) {
    Tmp = TmpIn & 1;
    ArrayImpulsi[n] = Tmp + 1;
    ParityCount += Tmp;
    TmpIn >>= 1;
  }
  //calcola i bits per l'anno
  TmpIn = Bin2Bcd(Anno - 2000);   
  for (n=50;n<58;n++) {
    Tmp = TmpIn & 1;
    ArrayImpulsi[n] = Tmp + 1;
    ParityCount += Tmp;
    TmpIn >>= 1;
  }
  //parita' di data
  if ((ParityCount & 1) == 0)
    ArrayImpulsi[58] = 1;
  else
    ArrayImpulsi[58] = 2;

  //ultimo impulso mancante
  ArrayImpulsi[59] = 0;

//for debug: print the whole 180 secs array       Serial.print(':'); for (n=0;n<60;n++){  Serial.print(ArrayImpulsi[n]); } Serial.println();

}

int Bin2Bcd(int dato) {
  int msb,lsb;
  if (dato < 10)
    return dato;
  msb = (dato / 10) << 4;
  lsb = dato % 10; 
  return msb + lsb;
}


void DcfOut() {

 
    switch (ContaImpulsiParziale++) {
      case 0:
   
        if (ArrayImpulsi[Secondi] != 0) {
         digitalWrite(ledPin, LOW);  
         si5351.output_enable(SI5351_CLK0, 0); }
         break;
      case 1:
        if (ArrayImpulsi[Secondi] == 1) {
         digitalWrite(ledPin, HIGH); 
         si5351.output_enable(SI5351_CLK0, 1); }
        break;
      case 2:
        digitalWrite(ledPin, HIGH); 
        si5351.output_enable(SI5351_CLK0, 1);
        break;
      case 9:
                ContaImpulsiParziale = 0;
                

   if (Secondi == 1 || Secondi == 15 || Secondi == 21  || Secondi == 29 ) Serial.print("-");
   if (Secondi == 36  || Secondi == 42 || Secondi == 45  || Secondi == 50 ) Serial.print("-");

            if (ArrayImpulsi[Secondi] == 1) Serial.print("0");
            if (ArrayImpulsi[Secondi] == 2) Serial.print("1"); 
            
               if (Secondi == 59 ) Serial.println();

               
               
        break;
    };
    
  };
