// Environment Monitoring System Edge device
// Get DHT11 and Laser PM100 
// Written by Sean Chen ww01'2015
// modified by Sean to add the IIC 1602 display

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <dht.h>

#define uchar unsigned char
#define uint unsigned int


// Uncomment whatever type you're using!
#define DHTTYPE DHT11 // DHT 11 
//#define DHTTYPE DHT22 // DHT 22 (AM2302)
//#define DHTTYPE DHT21 // DHT 21 (AM2301)

//#define DHT11PIN 2
//dht11 DHT11;
/*********PIN Definition for Galileo Board with 2 pins to drive single line device**************/

#define DHTIN 2 // what pin we're connected to single line device
#define DHTOUT 3
// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

LiquidCrystal_I2C lcd(0x27,16,2);
DHT dht(DHTIN,DHTOUT, DHTTYPE);
/*********Variables Updata**************/
int Humidity_Update = 0;      
int Temperature_Update = 0;  
int PM25_Update = 0;
int PM10_Update = 0;
// float UV_Update = 0;
//float CO2_Update = 0;

int Humidity_Last = 0;      
int Temperature_Last = 0;  
int PM25_Last = 0;
int PM10_Last = 0;

//PM data from KM-100
int PM_SETPin = 5;
int PM_RESETPin = 4;
uchar inByte = 0;         // incoming serial byte from PM2.5
uchar recData[24];
boolean RecStar = false;

void setup() {
  lcd.init();
  // delay(1000);
  // lcd.init();

  // lcd.clear();

  lcd.setCursor(0,0);
  lcd.print(" Intel-CD E.M.S ");
  lcd.setCursor(0,1);
  lcd.print("CD Site IoT Team");
  lcd.backlight();  
  
  // initialize serial:
  Serial.begin(9600);
  Serial1.begin(9600);
  pinMode(PM_SETPin,OUTPUT);
  pinMode(PM_RESETPin, OUTPUT);
  digitalWrite(PM_SETPin, HIGH);
  digitalWrite(PM_RESETPin, HIGH);
  dht.begin();

  delay(5000);  
   
}

void loop() {
  Sensor_All(); // 
  Update();    //
  refresh_display();
  // Wait a few seconds between next measurements.
  delay(5000);
}

void Update()
{
  Humidity_Last = Humidity_Update;      
  Temperature_Last = Temperature_Update;  
  PM25_Last = PM25_Update;
  PM10_Last = PM10_Update;

  Serial.print("Temperature:");Serial.println(Temperature_Update);
  Serial.print("Humidity:");Serial.println(Humidity_Update);
  Serial.print("PM25:");Serial.println(PM25_Update);
  Serial.print("PM10:");Serial.println(PM10_Update);

  Serial1.print(Temperature_Update);Serial1.print("|");
  Serial1.print(Humidity_Update);Serial1.print("|");
  Serial1.print(PM25_Update);Serial1.print("|");
  Serial1.print(PM10_Update);Serial1.print("|");
  
  // lcd.setCursor(0,0);
  // lcd.print("T:");
  // lcd.setCursor(2,0);
  // lcd.print(Temperature_Update);

  // lcd.setCursor(7,0);
  // lcd.print("--");
  // lcd.setCursor(9,0);
  // lcd.print("H:");
  // lcd.setCursor(11,0);
  // lcd.print(Humidity_Update);
  
  // lcd.setCursor(0,1);
  // lcd.print("PM25:");
  // lcd.setCursor(5,1);
  // lcd.print(PM25_Update);

  // lcd.setCursor(8,1);
  // lcd.print("PM10:");
  // lcd.setCursor(13,1);
  // lcd.print(PM10_Update);
}

void Sensor_All()
{
  Sensor_TemperatureHumidity();
  Sensor_PM();
}

void Sensor_TemperatureHumidity()
{
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  Humidity_Update = h;

  // Read temperature as Celsius
  float t = dht.readTemperature();
  Temperature_Update = t;

  // Read temperature as Fahrenheit
  float f = dht.readTemperature(true);
   
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println("Failed to read from DHT sensor!");
    lcd.setCursor(0,0);
    lcd.print(" DHT Read Fail! ");
    delay(500);
    Humidity_Update = Humidity_Last;      
    Temperature_Update = Temperature_Last;      
    return;
  }

  // Compute heat index
  // Must send in temp in Fahrenheit!
  float hi = dht.computeHeatIndex(f, h);
}

void Sensor_PM()
{
  read_PM();
  boolean flag = verify_PM();
 
  //check PM device recvice data CRC result
  if(flag == true)
  {
    PM25_Update = recData[6]*256 + recData[7];
    PM10_Update = recData[8]*256 + recData[9];
    Serial.println("Good PM data");
      //pass;
  }
  else
  {
    lcd.setCursor(0,1);
    lcd.print("  PM Read Fail! ");
    delay(500);
    PM25_Update = PM25_Last; 
    PM10_Update = PM10_Last;   
    Serial.println("wrong PM");
  }
  memset(recData,0x00,24);
}

void read_PM()
{
  int i=0;

  //digitalWrite(PM_SETPin, HIGH);
  //delay (800);
  
  Serial.println("PM Sensor Data:");
  while (Serial1.available() > 0) 
  {
      // get incoming byte:
      inByte = Serial1.read();
      // read first analog input:
      if (inByte == 0x42)
      {
        RecStar = true;
      }  
      if (RecStar == true)
      {
        recData[i]=inByte;
        //Serial.print(recData[i], HEX);
        Serial.print(0x0F & (inByte>>4), HEX);
        Serial.print(0x0F & inByte,HEX);
        Serial.print("-");

        i++;
        if (i==24)
        {
          i=0;
          RecStar = false;
         break;
        }
      }
  }
  //digitalWrite(PM_SETPin,LOW);
  //delay(200);
}

boolean verify_PM()
{
  uchar i=0;
  int CRC = 0;
  
  if (recData[0]==0x42 && recData[1]==0x4d){
    Serial.println("CRC data:");
    for (i=0;i<22;i++){
      CRC+=recData[i];
      //Serial.println("");
      //Serial.print(CRC_high, HEX);
      //Serial.println(CRC_low, HEX);
      // Serial.print(0x0F & (CRC>>4), HEX);
      // Serial.print(0x0F & CRC_high,HEX);
      // Serial.print(0x0F & (CRC_low>>4), HEX);
      // Serial.print(0x0F & CRC_low,HEX);
      // Serial.print(" ");
    }
    Serial.print(CRC, HEX);
  }  
  
  if (CRC == (recData[22]*256 + recData[23])){
    return true;
  }
  else{
    return false;
  }
}

void refresh_display()
{

  //lcd.backlight();  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" Intel-CD E.M.S ");
  lcd.setCursor(0,1);
  lcd.print("CD Site IoT Team");
  delay(1000);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Temperature:");
  lcd.setCursor(12,0);
  lcd.print(Temperature_Update);
  lcd.setCursor(15,0);
  lcd.print("C");

  lcd.setCursor(0,1);
  lcd.print("  Humidity:");
  lcd.setCursor(12,1);
  lcd.print(Humidity_Update);
  lcd.setCursor(15,1);
  lcd.print("%");
  delay(3000);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("PM2.5:");
  lcd.setCursor(7,0);
  lcd.print(PM25_Update);
  lcd.setCursor(11,0);
  lcd.print("ug/m3");

  lcd.setCursor(0,1);
  lcd.print("PM 10:");
  lcd.setCursor(7,1);
  lcd.print(PM10_Update);
  lcd.setCursor(11,1);
  lcd.print("ug/m3");
  
  //delay(3000);
  //lcd.noBacklight();  

}