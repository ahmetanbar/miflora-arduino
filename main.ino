#include <SoftwareSerial.h>

SoftwareSerial BTSerial(10,11); // RX | TX
#define delimiter "4f4b2b444953413a"
#define mac_address "C47C8D628AAC"
int pos = 0;
bool allowed = true;
String returnedString = "";
String temp = "";
int count = 0;

unsigned long timebt = 0;
char datacloud[1000];


struct decompose{
  char subject[4];
  int start;
  int len;
  boolean reverse;
  char extract[60];
};

struct decompose d[6] = {{"mac",16,12,true},{"typ",28,2,false},{"rsi",30,2,false},{"rdl",32,2,false},{"sty",44,4,true},{"rda",34,60,false}};

void extract_char(char * token_char, char * subset, int start ,int l, boolean reverse, boolean isNumber){
    char tmp_subset[l+1];
    memcpy( tmp_subset, &token_char[start], l );
    tmp_subset[l] = '\0';
    if (isNumber){
      char tmp_subset2[l+1];
      if (reverse) revert_hex_data(tmp_subset, tmp_subset2, l+1);
      else strncpy( tmp_subset2, tmp_subset , l+1);
      long long_value = strtoul(tmp_subset2, NULL, 16);
      sprintf(tmp_subset2, "%ld", long_value);
      strncpy( subset, tmp_subset2 , l+1);
    }else{
      if (reverse) revert_hex_data(tmp_subset, subset, l+1);
      else strncpy( subset, tmp_subset , l+1);
    }
    subset[l] = '\0';
}

void revert_hex_data(char * in, char * out, int l){
  //reverting array 2 by 2 to get the data in good order
  int i = l-2 , j = 0; 
  while ( i != -2 ) {
    if (i%2 == 0) out[j] = in[i+1];
    else  out[j] = in[i-1];
    j++;
    i--;
  }
  out[l-1] = '\0';
}

void strupp(char* beg)
{
    while (*beg = toupper(*beg))
       ++beg;
}

int strpos(char *haystack, char *needle) //from @miere https://stackoverflow.com/users/548685/miere
{
   char *p = strstr(haystack, needle);
   if (p)
      return p - haystack;
   return -1;
}

boolean process_data(int offset, char * rest_data, char * mac_adress){
  unsigned int data_length = 0;
  switch (rest_data[51 + offset]) {
    case '1' :
    case '2' :
    case '3' :
    case '4' :
        data_length = ((rest_data[51 + offset] - '0') * 2)+1;
    break;
    default:
        Serial.println("can't read data_length");
    return false;
    }
    
  char rev_data[data_length];
  char data[data_length];
  memcpy( rev_data, &rest_data[52 + offset], data_length );
  rev_data[data_length] = '\0';
  
  // reverse data order
  revert_hex_data(rev_data, data, data_length);
  double value = strtol(data, NULL, 16);
//  Serial.println(value);
  char val[12];
  String mactopic(mac_adress);

  // second value
  char val2[12];
  // Mi flora provides tem(perature), (earth) moi(sture), fer(tility) and lux (illuminance)
  // Mi Jia provides tem(perature), batt(erry) and hum(idity)
  // following the value of digit 47 we determine the type of data we get from the sensor
  switch (rest_data[47 + offset]) {
    case '9' :
          Serial.println("fer");
          Serial.println((double)value);
    break;
    case '4' :
          if (value > 65000) value = value - 65535;
          Serial.println("tem"); 
          Serial.println((double)value/10);
    break;
    case '6' :
          if (value > 65000) value = value - 65535;
          Serial.println("hum");
          Serial.println((double)value/10);
    break;
    case '7' :
          Serial.println("lux");
          Serial.println((double)value);
     break;
    case '8' :
          Serial.println("moi");
          Serial.println((double)value);
     break;
     
    case 'a' : // batteryLevel
          Serial.println("batt");
          Serial.println((double)value);
     break;

     case 'd' : // temp+hum
          char tempAr[8];
          // humidity
          memcpy(tempAr, data, 4);
          tempAr[4] = '\0';
          value = strtol(tempAr, NULL, 16);
          if (value > 65000) value = value - 65535;
          // temperature
          memcpy(tempAr, &data[4], 4);
          tempAr[4] = '\0';
          value = strtol(tempAr, NULL, 16);
          if (value > 65000) value = value - 65535;
     break;
    default:
    Serial.println("can't read values");
    return false;
    }
    return true;
}

void parseit(String returnedString){
  if (returnedString != "") {
    size_t pos = 0;
    while ((pos = returnedString.lastIndexOf(delimiter)) != -1) {
      String token = returnedString.substring(pos);
      returnedString.remove(pos,returnedString.length());
      char token_char[token.length()+1];
      token.toCharArray(token_char, token.length()+1); 
      if ( token.length() > 60){// we extract data only if we have detailled infos
        for(int i =0; i<6;i++)
        {
          extract_char(token_char,d[i].extract,d[i].start, d[i].len ,d[i].reverse, false);
          if (i == 3) d[5].len = (int)strtol(d[i].extract, NULL, 16) * 2; // extracting the length of the rest data
        }
  
        if((strlen(d[0].extract)) == 12) // if a mac adress is detected we publish it
        {
         strupp(d[0].extract);   
         // checking mac address is valid
         if(String(d[0].extract)!=mac_address){
          return;
         }
         String Service_data(d[5].extract);
         Service_data = Service_data.substring(14);
        if (strcmp(d[4].extract, "fe95") == 0) {
          int pos = -1;
          pos = strpos(d[5].extract,(char *)"209800");
          if (pos != -1) {
            boolean result = process_data(pos - 38,(char *)Service_data.c_str(),d[0].extract);
          }
          pos = -1;
          pos = strpos(d[5].extract,(char *)"20aa01");
          if (pos != -1){
            boolean result = process_data(pos - 40,(char *)Service_data.c_str(),d[0].extract);
          }
        }
       }         
      }
    }
  }
}



void setup() 
{
  Serial.begin(115200);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  
//  Serial.println("Enter AT commands:");
  BTSerial.begin(115200);
  delay(2000);
  Serial.println(millis());
}

void loop()
{
  if(allowed){
    BTSerial.write("AT+DISA?");
    count = 0;
    timebt = millis();
    allowed = false; 
  }
  
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

  if(millis()>(timebt+8000)){
    parseit(String(datacloud));
    memset(datacloud, 0, sizeof datacloud);
    allowed = true;
   }
   
 if (Serial.available())
    BTSerial.write(Serial.read());
}
