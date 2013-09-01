//
//
//  ARRRduino-FM: Part 15 FM Broadcasting for the Arduino
//
//  By:   Mike Yancey
//  Date: October 7, 2008
//  Version 3.0
//  Language: Arduino 0013
//  Developed Around: October 7-12, 2008
//  Updated to Version 3.0: February 2009
//  
//  Description: Uses an Arduino or compatible to control an 
//               NS73M FM Transmitter module (available from 
//               Sparkfun) on the I2C bus.
//
//  Goals for Version 2:
//    Upgrade for Arduino 0012, which includes an LCD Library
//    Move to a single Rotary Encoder control instead of buttons.
//    Encoder will have a bump-switch to use for a On/Off-Air button.
//    Moving to an encoder will simplify packaging
//
//  Experimentally derived Band Settings for VFO ...
//		Band 0: 83.78 - 91.72
//		Band 1: 88.74 - 98.28
//		Band 2: 93.10 - 104.00
//		Band 3: 99.50 - 112.72


#include <Wire.h>
#include <EEPROM.h>
#include <LiquidCrystal.h>

#define encoderPinA 2               // up button on pin 2
#define encoderPinB 3               // down button on pin 3

volatile unsigned int encoderPos = 0;
unsigned int encoderLast = 0;


#define setButton 4                 // set button on pin 4

#define topFM  107900000            // Top of the FM Dial Range in USA
#define botFM   87500000            // Bottom of the FM Dial Range in USA
#define incrFM    200000            // FM Channel Increment in USA
// define incrFM   100000           // FM Channel Increment - certain countries.
// define incrFM    50000           // FM Channel Increment - certain countries...


long frequency = 97300000;          // the default initial frequency in Hz
long newFrequency = 0;
boolean gOnAir = false;             // Initially, NOT On The Air...

// Define the LCD
#define rs 12
#define rw 13                       // Indicate RW tied to 0.0 volts / WRITE ONLY
#define enable  11
#define d0  7
#define d1  8
#define d2  9
#define d3  10

LiquidCrystal lcd( rs, rw, enable, d0, d1, d2, d3 );


  
void setup() {
  

  //Serial.begin(9600);                 //for debugging
 /*
  // Attempt to read the last saved frequency from EEPROM
  newFrequency = loadFrequency();

  // Test if outside our FM Range...
  if ( newFrequency < botFM || newFrequency > topFM ) {
    // Sorry - haven't saved before - use the default.
    frequency = 97300000;
  }
  else {
    // We have a valid frequency!
    frequency = newFrequency;
  }  
  
*/

  Wire.begin();                       // join i2c bus as master

  transmitter_setup( frequency );
  
  //set_freq( frequency );
  //saveFrequency( frequency ); 
 
}



void loop() {


  

}



void transmitter_setup( long initFrequency )
{
  i2c_send(0x0E, B00000101); //Software reset

  i2c_send(0x01, B10110100); //Register 1: forced subcarrier, pilot tone on
	
  i2c_send(0x02, B00000011); //Register 2: Unlock detect off, 2mW Tx Power

  set_freq( initFrequency);

  //i2c_send(0x00, B10100001); //Register 0: 200mV audio input, 75us pre-emphasis on, crystal off, power on
  i2c_send(0x00, B00100001); //Register 0: 100mV audio input, 75us pre-emphasis on, crystal off, power on
  
  i2c_send(0x0E, B00000101); //Software reset
  
  i2c_send(0x06, B00011110); //Register 6: charge pumps at 320uA and 80 uA
}

void transmitter_standby( long aFrequency )
{
  //i2c_send(0x00, B10100000); //Register 0: 200mV audio input, 75us pre-emphasis on, crystal off, power OFF
  i2c_send(0x00, B00100000); //Register 0: 100mV audio input, 75us pre-emphasis on, crystal off, power OFF
  
  //displayFrequency( aFrequency );
  
  delay(100);
  gOnAir = false;
}

void set_freq( long aFrequency )
{
  int new_frequency;

  // New Range Checking... Implement the (experimentally determined) VFO bands:
  if (aFrequency < 88500000) {                       // Band 3
    i2c_send(0x08, B00011011);
    //Serial.println("Band 3");
  }  
  else if (aFrequency < 97900000) {                 // Band 2
    i2c_send(0x08, B00011010);
    //Serial.println("Band 2");
  }
  else if (aFrequency < 103000000) {                  // Band 1 
    i2c_send(0x08, B00011001);
    //Serial.println("Band 1");
  }
  else {
    // Must be OVER 103.000.000,                    // Band 0
    i2c_send(0x08, B00011000);
    //Serial.println("Band 0");
  }


  new_frequency = (aFrequency + 304000) / 8192;
  byte reg3 = new_frequency & 255;                  //extract low byte of frequency register
  byte reg4 = new_frequency >> 8;                   //extract high byte of frequency register
  i2c_send(0x03, reg3);                             //send low byte
  i2c_send(0x04, reg4);                             //send high byte
  
  // Retain old 'band set' code for reference....  
  // if (new_frequency <= 93100000) { i2c_send(0x08, B00011011); }
  // if (new_frequency <= 96900000) { i2c_send(0x08, ); }
  // if (new_frequency <= 99100000) { i2c_send(0x08, B00011001); }
  // if (new_frequency >  99100000) { i2c_send(0x08, B00011000); }
  
  i2c_send(0x0E, B00000101);                        //software reset  

  //Serial.print("Frequency changed to ");
  //Serial.println(aFrequency, DEC);

  //i2c_send(0x00, B10100001); //Register 0: 200mV audio input, 75us pre-emphasis on, crystal off, power ON
  i2c_send(0x00, B00100001); //Register 0: 100mV audio input, 75us pre-emphasis on, crystal off, power ON
  
  lcd.setCursor(0, 1);
  lcd.print( " On Air ");
  gOnAir = true;
}

void i2c_send(byte reg, byte data)
{ 
    Wire.beginTransmission(B1100111);               // transmit to device 1100111
    Wire.write(reg);                                 // sends register address
    Wire.write(data);                                // sends register data
    Wire.endTransmission();                         // stop transmitting
    delay(5);                                       // allow register to set
}

void saveFrequency ( long aFrequency ) 
{
  long memFrequency = 0;                   // For use in Read / Write to EEProm

  //Serial.print( "Saving: " );
  //Serial.println(aFrequency, DEC);

  memFrequency = aFrequency / 10000;
  EEPROM.write( 0, memFrequency / 256);   // right-most byte
  EEPROM.write( 1, memFrequency - (memFrequency / 256) * 256 );   // next to right-most byte
}

long loadFrequency ()
{
  long memFrequency = 0;                   // For use in Read / Write to EEProm

  memFrequency = EEPROM.read(0) * 256 + EEPROM.read(1);
  memFrequency *= 10000;

  //Serial.print("Retrieving: " );
  //Serial.println(memFrequency, DEC);
  return  memFrequency;
}



