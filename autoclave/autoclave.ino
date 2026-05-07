#include "max6675.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);  //sometimes the adress is not 0x27. Change to 0x3f if it dosn't work.
#include <HX711_ADC.h>

//Inputs and outputs
int firing_pin = 11;
int thermoDO = 9;
int thermoCS = 10;
int thermoCLK = 12;


//pins:
const int HX711_dout_1 = 3; //mcu > HX711 no 1 dout pin albverde
const int HX711_sck_1 = 4; //mcu > HX711 no 1 sck pin albastru
const int HX711_dout_2 = 5; //mcu > HX711 no 2 dout pin galben 
const int HX711_sck_2 = 6; //mcu > HX711 no 2 sck pin
const int HX711_dout_3 = 7; //mcu > HX711 no 2 dout pinrosu
const int HX711_sck_3 = 8; //mcu > HX711 no 2 sck pin
//HX711 constructor (dout pin, sck pin)
HX711_ADC LoadCell_1(HX711_dout_1, HX711_sck_1); //HX711 1
HX711_ADC LoadCell_2(HX711_dout_2, HX711_sck_2); //HX711 2
HX711_ADC LoadCell_3(HX711_dout_3, HX711_sck_3); //HX711 3



//Start a MAX6675 communication with the selected pins
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

byte real_temperature = 0;
byte setpoint = 113;

void setup() {
  pinMode (firing_pin,OUTPUT); 
  Serial.begin(57600);       //Start serial com with the BT module (RX and TX pins   
  lcd.init();       //Start the LC communication
  lcd.backlight();  //Turn on backlight for LCD

  byte calibrationValue_1 = 105; // calibration value load cell 1
  byte calibrationValue_2 = 104; // calibration value load cell 2
  byte calibrationValue_3 = 103; // calibration value load cell 2
  
  LoadCell_1.begin();
  LoadCell_2.begin();
  LoadCell_3.begin();

  unsigned long stabilizingtime = 2000; // tare preciscion can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  byte loadcell_1_rdy = 0;
  byte loadcell_2_rdy = 0;
  byte loadcell_3_rdy = 0;
  while ((loadcell_1_rdy + loadcell_2_rdy + loadcell_3_rdy) < 3) { //run startup, stabilization and tare, both modules simultaniously
    if (!loadcell_1_rdy) loadcell_1_rdy = LoadCell_1.startMultiple(stabilizingtime, _tare);
    if (!loadcell_2_rdy) loadcell_2_rdy = LoadCell_2.startMultiple(stabilizingtime, _tare);
    if (!loadcell_3_rdy) loadcell_3_rdy = LoadCell_3.startMultiple(stabilizingtime, _tare);
  }
  if (LoadCell_1.getTareTimeoutFlag()||LoadCell_2.getTareTimeoutFlag()||LoadCell_3.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
  }
  
  
  LoadCell_1.setCalFactor(calibrationValue_1); // user set calibration value (float)
  LoadCell_2.setCalFactor(calibrationValue_2); // user set calibration value (float)
  LoadCell_3.setCalFactor(calibrationValue_3); // user set calibration value (float)
  Serial.println("Startup is complete");

}

void loop() {    
               //Increase the previous time for next loop
  boolean newDataReady = 0;
  
  float a,b,c;
  // check for new data/start next conversion:
  if (LoadCell_1.update()) newDataReady = true;
  LoadCell_2.update();
  LoadCell_3.update();
  if ((newDataReady)) {
    
       a = LoadCell_1.getData();
       b = LoadCell_2.getData();
       c = LoadCell_3.getData();
      newDataReady = 0;

    }
  


  
  real_temperature = thermocouple.readCelsius();  //get the real temperature in Celsius degrees
  if (real_temperature < setpoint && a + b + c > -2000)
    digitalWrite(firing_pin,HIGH);
  else
    digitalWrite(firing_pin,LOW);
  
  if(a + b + c > -2000)
  {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Set: ");
    lcd.setCursor(5,0);
    lcd.print(setpoint);
    lcd.setCursor(8,0);
    lcd.print(";g=");
    lcd.setCursor(11,0);
    lcd.print((int)(a+b+c));
    lcd.setCursor(0,1);
    lcd.print("Real temp: ");
    lcd.setCursor(11,1);
    lcd.print(real_temperature);
  }
  else {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("S-a stricat ceva la kukta!");
    
  }

  
  Serial.println(real_temperature);
  if(Serial.available()>0)
   { 
      setpoint = Serial.read();
   }
   delay(500);
}