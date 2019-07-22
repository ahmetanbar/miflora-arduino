#include <SoftwareSerial.h>
#include "miflora-arduino.h"

// Determine RX,TX pin of HM-10 bluetooth sensor,
// For your own Arduino, check documentation. For example we can't use all pins of Arduino Mega for serial.
SoftwareSerial BTSerial(10,11); // RX | TX
bool allowed = true;
String returnedString = "";
String temp = "";
int count = 0;
unsigned long timebt = 0;
// TODO:Change the way of getting data by static array.
char datacloud[1000];

void setup() 
{
  // Determine baud rate
  Serial.begin(115200);
  
  // Determine baud rate. But firstly, you should check your own HM-10's baud rate.
  BTSerial.begin(115200);
  // Take a breath.
  delay(2000);
}

void loop()
{
  // We wait eight seconds for able to take data from HM-10 succesfully
  if(allowed){
    // Send "AT+DISA?" command to HM-10. So it gets informationes from found devices 
    BTSerial.write("AT+DISA?");
    count = 0;
    timebt = millis();
    allowed = false; 
  }

  // After our command "AT+DISA?", HM-10 returns us some datas. When it return data, we detect it. 
  while (BTSerial.available()){
    int a = BTSerial.read();
    if (a < 16) {
      temp = "0";      
    }
    temp = temp + String(a,HEX);
    datacloud[count] = temp[0];
    datacloud[count+1] = temp[1];
    count+=2;
    temp = "";
  }

  // We ensure about taking data fully. So we wait eight seconds, before parsing the `datacloud`.
  if(millis()>(timebt+8000)){
    parseit(String(datacloud));
    memset(datacloud, 0, sizeof datacloud);
    allowed = true;
   }

 if (Serial.available())
    BTSerial.write(Serial.read());
}
