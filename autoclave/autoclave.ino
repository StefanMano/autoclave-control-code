#include "max6675.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,16,2);  //sometimes the adress is not 0x27. Change to 0x3f if it dosn't work.

//Inputs and outputs
int firing_pin = 3;
int zero_cross = 8;
int thermoDO = 9;
int thermoCS = 10;
int thermoCLK = 12;

//Start a MAX6675 communication with the selected pins
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

int real_temperature = 0;
int setpoint = 111;

void setup() {
  pinMode (firing_pin,OUTPUT); 
  Serial.begin(9600);       //Start serial com with the BT module (RX and TX pins   
  lcd.init();       //Start the LC communication
  lcd.backlight();  //Turn on backlight for LCD
}

void loop() {    
               //Increase the previous time for next loop
  real_temperature = thermocouple.readCelsius();  //get the real temperature in Celsius degrees
  if (real_temperature>=setpoint)
    digitalWrite(firing_pin,LOW);
  else
    digitalWrite(firing_pin,HIGH);
  delay(3000);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Set: ");
  lcd.setCursor(5,0);
  lcd.print(setpoint);
  lcd.setCursor(0,1);
  lcd.print("Real temp: ");
  lcd.setCursor(11,1);
  lcd.print(real_temperature);

  Serial.println(real_temperature);
  if(Serial.available()>0)
   { 
      setpoint = Serial.read();
   }
}