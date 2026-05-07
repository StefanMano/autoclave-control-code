#include "max6675.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);  //sometimes the adress is not 0x27. Change to 0x3f if it dosn't work.

#include <EEPROM.h>

//Inputs and outputs
byte firing_pin = 11;
byte reset_pin = 2;
byte thermoDO = 9;
byte thermoCS = 10;
byte thermoCLK = 12;
byte calibrare = 0;
float t_at_T = 0;
float t_off = 0; 
byte sec_on = 0;
byte sec_off = 0;
byte min_on = 0;
byte min_off = 0;

//pins:



//Start a MAX6675 communication with the selected pins
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

byte real_temperature = 0;
byte setpoint = 113;
const byte fixedsetpoint = setpoint;
float set_time_h = 6;
bool rise = true;
int EEPROM_address = 2;
int EEPROM_address2 = EEPROM_address + 8;
int TOP = 15625;



volatile unsigned long last_interrupt_time = 0;
volatile bool reset_flag = false;

ISR(INT0_vect) {
    reset_flag = true;  // Set flag instead of writing to EEPROM
}


void increment_min_on(){
  min_on++;
  if(min_on % 6 == 0)
    {
      min_on = 0;
      t_at_T += 0.1;
      EEPROM.put(EEPROM_address, t_at_T);
      
      if (t_at_T >= set_time_h)
      {
        setpoint = fixedsetpoint - (int)((t_at_T-set_time_h)*10);
        if(setpoint <=100)
          setpoint  = 0;
        
      }
    }
}
void increment_min_off(){
  min_off++;
  if(min_off % 6 == 0)
    {
      min_off = 0;
      t_off += 0.1;
      EEPROM.put(EEPROM_address2, t_off);
    }
}
ISR(TIMER1_COMPA_vect){
  if((real_temperature>=110||t_at_T>=set_time_h)&&setpoint !=0)
  {
    
    sec_on++;
    if (sec_on%60==0){
      sec_on = 0;
      increment_min_on();
    }
  }
  else if(setpoint!=0){
    sec_off++;
    if (sec_off%60==0){
      sec_off = 0;
      increment_min_off();
    }
  }
  }



void setup() {
 


  EICRA |= (1<<ISC01) | (0<<ISC00);
  EIMSK |= (1<<INT0);

  //set timer1 interrupt at 1Hz
  TCCR1A = 0;// set entire TCCR1A register to 0
  TCCR1B = 0;// same for TCCR1B
  TCNT1  = 0;//initialize counter value to 0
  // set compare match register for 1hz increments
  OCR1A = TOP-1;// = (16*10^6) / (1*1024) - 1 (must be <65536)
  // turn on CTC mode
  TCCR1B |= (1 << WGM12);
  // Set CS10 and CS12 bits for 1024 prescaler
  TCCR1B |= (1 << CS12) | (1 << CS10);  
  // disable until needed timer compare interrupt
  
  SREG |= (1 << SREG_I) ;
  pinMode (firing_pin,OUTPUT);
  pinMode (reset_pin, INPUT_PULLUP); 

  Serial.begin(9600); 
  Serial.print("on") ;     //Start serial com with the BT module (RX and TX pins   
  lcd.init();       //Start the LC communication
  lcd.backlight();  //Turn on backlight for LCD

  TIMSK1 |= (1 << OCIE1A);

}



void loop() {    
               //Increase the previous time for next loop
  boolean newDataReady = 0;
  EEPROM.get(EEPROM_address,t_at_T);
  EEPROM.get(EEPROM_address2,t_off);
  float a,b,c;
  // check for new data/start next conversion:
  
  
  real_temperature = thermocouple.readCelsius()+calibrare;  //get the real temperature in Celsius degrees

  //control de temp intre setpoint si setpoint - 3 daca kukta nu e prea goala
 
    if (rise == true)
      {
        if (real_temperature < setpoint)
          digitalWrite(firing_pin,HIGH);
        else
          {
            digitalWrite(firing_pin,LOW);
            rise = false;
          }
      }
    else
    {
      if(real_temperature>(setpoint - 3))
        digitalWrite(firing_pin,LOW);
      else
      {
        digitalWrite(firing_pin,HIGH);
        rise = true;
      }
    }
  
  
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Temp : ");
    lcd.setCursor(7,0);
    lcd.print(real_temperature);
    lcd.setCursor(10,0);
    lcd.print("/");
    lcd.setCursor(11,0);
    lcd.print(setpoint);
    lcd.setCursor(14,0);
    lcd.print("°C");
    lcd.setCursor(0,1);
    lcd.print("Timp heat/steril = ");
    lcd.setCursor(0,2);
    lcd.print(t_off);
    lcd.setCursor(4,2);
    lcd.print("/");
    lcd.setCursor(5,2);
    lcd.print(t_at_T);
    lcd.setCursor(0,3);
    lcd.print("Reset counter V");
  
  

  if((real_temperature>=110||t_at_T>=set_time_h)&&setpoint !=0)
  {
    lcd.setCursor(10,2);
    lcd.print("sterilizat");
  }
  else if(setpoint!=0){
    lcd.setCursor(10,2);
    lcd.print("incalzit");
  }
  else{
    lcd.setCursor(13,2);
    lcd.print("gata");
  }
  //enable timer when temp high enough
  

  if(reset_flag == true){
    
    
    int del = 0;
    while (reset_flag==true){
        delay(50);
        if(digitalRead(reset_pin)==0)
        {
          del+=50;
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Se reseteaza la 1000");
          
          lcd.setCursor(0,1);
          lcd.print(del);
        }
        else 
          reset_flag = false;
        if(del>=1000){
          EEPROM.put(EEPROM_address, 0.0f);
           min_on = 0;
          sec_on = 0;
          reset_flag = false;
          EEPROM.put(EEPROM_address2, 0.0f);
           min_off = 0;
          sec_off = 0;
        }

    }
    
  }
  //comunicare bluetooth
  Serial.println(real_temperature);
  if(Serial.available()>0)
   { 
      setpoint = Serial.read();
   }
   delay(500);

   

}
