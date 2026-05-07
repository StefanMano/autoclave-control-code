#include "max6675.h"
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,20,4);  //sometimes the adress is not 0x27. Change to 0x3f if it dosn't work.
#include <HX711_ADC.h>
#include <EEPROM.h>

//Inputs and outputs
byte firing_pin = 11;
byte reset_pin = 2;
byte thermoDO = 9;
byte thermoCS = 10;
byte thermoCLK = 12;

float t_at_T = 0; 
byte sec = 0;
byte min = 0;

//pins:
const byte HX711_dout_1 = 3; //mcu > HX711 no 1 dout pin albverde
const byte HX711_sck_1 = 4; //mcu > HX711 no 1 sck pin albastru
const byte HX711_dout_2 = 5; //mcu > HX711 no 2 dout pin galben 
const byte HX711_sck_2 = 6; //mcu > HX711 no 2 sck pin
const byte HX711_dout_3 = 7; //mcu > HX711 no 2 dout pinrosu
const byte HX711_sck_3 = 8; //mcu > HX711 no 2 sck pin
//HX711 constructor (dout pin, sck pin)
HX711_ADC LoadCell_1(HX711_dout_1, HX711_sck_1); //HX711 1
HX711_ADC LoadCell_2(HX711_dout_2, HX711_sck_2); //HX711 2
HX711_ADC LoadCell_3(HX711_dout_3, HX711_sck_3); //HX711 3



//Start a MAX6675 communication with the selected pins
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

byte real_temperature = 0;
byte setpoint = 113;
const byte fixedsetpoint = setpoint;
float set_time_h = 6;
bool rise = true;
int EEPROM_address = 2;
int TOP = 15625;



volatile unsigned long last_interrupt_time = 0;
volatile bool reset_flag = false;

ISR(INT0_vect) {
    reset_flag = true;  // Set flag instead of writing to EEPROM
}


void increment_min(){
  min++;
  if(min % 6 == 0)
    {
      min = 0;
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

ISR(TIMER1_COMPA_vect){
  if((real_temperature>=110||t_at_T>=set_time_h)&&setpoint !=0)
  {
    
    sec++;
    if (sec%60==0){
      sec = 0;
      increment_min();
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

  TIMSK1 |= (1 << OCIE1A);

}



void loop() {    
               //Increase the previous time for next loop
  boolean newDataReady = 0;
  EEPROM.get(EEPROM_address,t_at_T);
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

  //control de temp intre setpoint si setpoint - 3 daca kukta nu e prea goala
  if ( a + b + c > -2000)
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
  else
    digitalWrite(firing_pin,LOW);
  
  //afisare LCD
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
    lcd.setCursor(0,2);
    lcd.print("ore >110C = ");
    lcd.setCursor(12,2);
    lcd.print(t_at_T);
    lcd.setCursor(0,3);
    lcd.print("Reset counter V");
  }
  else {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("S-a stricat ceva ");
    lcd.setCursor(0,1);
    lcd.print("la kukta!");
    
  }

  if((real_temperature>=110||t_at_T>=set_time_h)&&setpoint !=0)
  {
    lcd.setCursor(17,2);
    lcd.print("on");
  }
  else{
    lcd.setCursor(17,2);
    lcd.print("off");
  }
  //enable timer when temp high enough
  

  if(reset_flag == true){
    unsigned long interrupt_time = millis();
    if (interrupt_time - last_interrupt_time > 50) { // 50ms debounce time
        EEPROM.put(EEPROM_address, 0.0f);
        min = 0;
        sec = 0;
    }
    last_interrupt_time = interrupt_time;
    reset_flag = false;
  }
  //comunicare bluetooth
  Serial.println(real_temperature);
  if(Serial.available()>0)
   { 
      setpoint = Serial.read();
   }
   delay(500);

   

}

