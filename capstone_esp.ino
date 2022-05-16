#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

Adafruit_ADS1115 ads; //ADS1115 initialization

//PH
#define pHpin     34
float calibration_value = 20.24 - 0.7; //21.34 - 0.7
int phval = 0; 
unsigned long int avgval; 
int buffer_arr[10],temp;

float ph_act;
float ph;


//TDS
#define TdsSensorPin 35
#define VREF 3.3              // analog reference voltage(Volt) of the ADC
#define SCOUNT  30            // sum of sample point

int analogBuffer[SCOUNT];     // store the analog value in the array, read from ADC
int analogBufferTemp[SCOUNT];
int analogBufferIndex = 0;
int copyIndex = 0;

float averageVoltage = 0;
float tdsValue = 0;
float temperature = 25;       // current temperature for compensation

// median filtering algorithm
int getMedianNum(int bArray[], int iFilterLen){
  int bTab[iFilterLen];
  for (byte i = 0; i<iFilterLen; i++)
  bTab[i] = bArray[i];
  int i, j, bTemp;
  for (j = 0; j < iFilterLen - 1; j++) {
    for (i = 0; i < iFilterLen - j - 1; i++) {
      if (bTab[i] > bTab[i + 1]) {
        bTemp = bTab[i];
        bTab[i] = bTab[i + 1];
        bTab[i + 1] = bTemp;
      }
    }
  }
  if ((iFilterLen & 1) > 0){
    bTemp = bTab[(iFilterLen - 1) / 2];
  }
  else {
    bTemp = (bTab[iFilterLen / 2] + bTab[iFilterLen / 2 - 1]) / 2;
  }
  return bTemp;
}




const char* ssid     = "Kavini";
const char* password = "Shiku@2018";
const char* host = "192.168.43.99"; 



void setup(void)
{

  Serial.begin(115200);

//WiFi
   Serial.println();
   Serial.print("Connecting to ");
   Serial.println(ssid);

   WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP()); 
    
  
  
  pinMode(TdsSensorPin,INPUT); //TDS
  ads.setGain(GAIN_TWOTHIRDS); //ADC resolution
  ads.begin();

      if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  display.display();
  delay(50);
 display.clearDisplay();
}

void loop(void)
{
  int16_t adc0;

  adc0 = ads.readADC_SingleEnded(0);


  //Serial.print("AIN0: "); Serial.println(adc0);
  float voltage = adc0* (3.3 /4096);
  

  
//PH
    for(int i=0;i<10;i++) 
 { 
 buffer_arr[i]=analogRead(pHpin);
 delay(30);
 }
 for(int i=0;i<9;i++)
 {
 for(int j=i+1;j<10;j++)
 {
 if(buffer_arr[i]>buffer_arr[j])
 {
 temp=buffer_arr[i];
 buffer_arr[i]=buffer_arr[j];
 buffer_arr[j]=temp;
 }
 }
 }
 avgval=0;
 for(int i=2;i<8;i++)
 avgval+=buffer_arr[i];
 float volt=(float)avgval*3.3/4095.0/6;  
 //Serial.print("Voltage: ");
 //Serial.println(volt);
  ph_act = -5.70 * volt + calibration_value;

  if (ph_act >=12.80 && ph_act <=13.40){ //water
  ph= map (ph_act,12.80,13.40,6,7);
 }

  if (ph_act >=12.53 && ph_act <=13.25){ //milk
  ph=map (ph_act,13.21,14,9,10);
 }

  if (ph_act >=14.94 && ph_act <=15.90){//orange juice
  ph=map (ph_act,14.94,15.90,3,4);
 }

 


  //TDS
  static unsigned long analogSampleTimepoint = millis();
  if(millis()-analogSampleTimepoint > 40U){     //every 40 milliseconds,read the analog value from the ADC
    analogSampleTimepoint = millis();
    analogBuffer[analogBufferIndex] = analogRead(TdsSensorPin);    //read the analog value and store into the buffer
    analogBufferIndex++;
    if(analogBufferIndex == SCOUNT){ 
      analogBufferIndex = 0;
    }
  }   
  
  static unsigned long printTimepoint = millis();
  if(millis()-printTimepoint > 800U){
    printTimepoint = millis();
    for(copyIndex=0; copyIndex<SCOUNT; copyIndex++){
      analogBufferTemp[copyIndex] = analogBuffer[copyIndex];
      
      // read the analog value more stable by the median filtering algorithm, and convert to voltage value
      averageVoltage = getMedianNum(analogBufferTemp,SCOUNT) * (float)VREF / 4096.0;
      
      //temperature compensation formula: fFinalResult(25^C) = fFinalResult(current)/(1.0+0.02*(fTP-25.0)); 
      float compensationCoefficient = 1.0+0.02*(temperature-25.0);
      //temperature compensation
      float compensationVoltage=averageVoltage/compensationCoefficient;
      
      //convert voltage value to tds value
      tdsValue=(133.42*compensationVoltage*compensationVoltage*compensationVoltage - 255.86*compensationVoltage*compensationVoltage + 857.39*compensationVoltage)*0.5;
      
      //Serial.print("voltage:");
      //Serial.print(averageVoltage,2);
      //Serial.print("V   ");

    }
  }

Serial.print("Turbidity: "); 
  Serial.println(voltage);
  delay(4000);

  display.setTextColor(WHITE);
  display.setCursor(0,6);
  display.setTextSize(2);
  display.println("Turbidity");
  display.setTextSize(3);
  display.setCursor(0,35);
  display.print(voltage);
  display.print(" V");
  display.display();
  delay(5000);
  display.clearDisplay();


  Serial.print("pH Value: ");
 Serial.println(ph);
 //delay(4000);

  //display.clearDisplay();
   display.setTextColor(WHITE);
   display.setCursor(0,6);
  display.setTextSize(2);
  display.setCursor(20,0);
  display.println("Ph Value");
  display.setTextSize(3);
  display.setCursor(30,30);
  display.print(ph);
  display.display(); 
  delay(5000);
  display.clearDisplay();

  
      Serial.print("TDS Value:");
      Serial.print(tdsValue,0);
      Serial.println("ppm");
      //delay(4000);

      display.setTextColor(WHITE);
      display.setCursor(0,6);
      display.setTextSize(2);
      display.println("TDS Value");
      display.setTextSize(2);
      display.setCursor(0,35);
      display.print(tdsValue);
      display.print(" ppm");
      display.display();
      delay(5000);
      display.clearDisplay();
      delay(1000);



//WiFi begins
    Serial.print("connecting to ");
    Serial.println(host);

    // Use WiFiClient class to create TCP connections
    WiFiClient client;
    HTTPClient https;


 String con= String("http://192.168.43.99/capstone/connect.php?") + 
                   ("TDS=") + tdsValue + ("&pH=") + ph + ("&Turbidity=") + voltage;
 Serial.println(con);
https.begin(client,con);
int statusCode=https.GET();
if (statusCode>0){
  String result=https.getString();
  Serial.println(result);
  }
  else{
   Serial.println("error detected"); 
    }



/*
    const int httpPort = 80;
    if (!client.connect(host, httpPort)) {
        Serial.println("connection failed");
        return;


     client.print(String("GET http://192.168.103.235/capstone/connect.php?") + 
                          ("TDS=") + tdsValue + ("&pH=") + ph + ("&Turbidity=") + voltage +  
                          " HTTP/1.1\r\n" +
                 "Host: " + host + "\r\n" +
                 "Connection; close\r\n\r\n");

    unsigned long timeout = millis();
    while (client.available() == 0) {
        if (millis() - timeout > 1000) {
            Serial.println(">>> Client Timeout !");
            client.stop();
            return;
        }
    }

    // Read all the lines of the reply from server and print them to Serial
    while(client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
        
    }

    Serial.println();
    Serial.println("closing connection");

    }*/
       
}
