// --------------------------------------
// MiniPirate
//
//   A human readable serial protocol
//   for basic I/O tasks.
//
// Heavily oriented at "Bus Pirate"
// http://dangerousprototypes.com/docs/Bus_Pirate_menu_options_guide
//
// Pin Layout:
// - A4: SDAp
// - A5: SCL
//
// Add command line UI and Port manipulations by O. Chatelain
// Including parts of I2C Scanner adapted by Arduino.cc user Krodal
// Including parts of Bus Pirate library by Ian Lesnet
//
// April 2014
// Using Arduino 1.0.1
//

#include <ctype.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Servo.h>
#include "pins_arduino.h"
#include "baseIO.h"
#include "modeBase.h"
#include "modeI2C.h"

long readAVR_VCC (long voltage_reference = 1125300);
long readAVRInternalTemp();
int freeRam();
float VCC;

Servo     servo;
ModeI2C   modeI2C;

enum mpModes { mNONE = 'n', mI2C = 'i', mSPI = 's', mMEMORY = 'm', mEEPROM = 'e', mFLASH = 'f' };
mpModes mpMode;

char* printMode () {
  switch(mpMode)  {
    case mNONE:   return "";
    case mI2C:    return "I2C";
    case mSPI:    return "SPI";
    case mMEMORY: return "Memory";
    case mEEPROM: return "EEPROM";
    case mFLASH:  return "Flash";
  }
}

void mpHelp() {

  Serial.println("LIST OF SUPPORTED COMMANDS");
  Serial.println("==========================");
  Serial.println("h/? - Show this help");
  
  //
  // Arduino port manipulations
  //
  Serial.println("p - Show all port values & directions");
  Serial.println("q - Show all port values & directions (quick)");

  Serial.println(". - Show port value & direction");
  Serial.println("< - Set a port as INPUT");
  Serial.println("> - Set a port as OUTPUT");

  Serial.println("/ - Set a port to HIGH (clock up)");
  Serial.println("\\ - Set a port to LOW (clock down)");
  Serial.println("^ - Set a port LOW-HIGH-LOW (one clock)");

  // Serial.println("b - Show bar graph of analog input");
  Serial.println("g - Set analog (pwm) value");

  Serial.println("s - Set servo value");

  //
  // I2C communication
  //
  // tbd: Serial.println("mi - Scan i2c device addresses");
  Serial.println("i - Scan i2c device addresses");
  Serial.println("# - Set i2c device active x ");
  Serial.println("r # - Read i2c n bytes from active device");
  Serial.println("w # # # - Write i2c bytes to active device");


  //
  // tbd: add SPI communication
  //
  // Serial.println("ms - spi enabled");
  // Serial.println("r # - spi read n bytes from active device");
  // Serial.println("w # # # - spi write bytes to active device");

  //
  // tbd: add LCD communication
  //
  // Serial.println("ml - LCD enabled");
  // Serial.println("r # - LCD read n bytes from active device");
  // Serial.println("w # # # - LCD write bytes to active device");

  //
  // tbd: add Memory access
  //
  // Serial.println("mm - Memory access enabled");
  // Serial.println("# - Set memory position to");
  // Serial.println("r # - Read n bytes from memory");
  // Serial.println("w # # # - Write bytes to memory");
  // Serial.println("| # # # - Or bit mask");
  // Serial.println("& # # # - And bit mask");

  //
  // tbd: add EEPROM access
  //
  // Serial.println("me - EEPROM access enabled");
  // Serial.println("# - Set EEPROM position to");
  // Serial.println("r # - Read n bytes from EEPROM");
  // Serial.println("w # # # - Write bytes to EEPROM");

  //
  // tbd: add FLASH access
  //
  // Serial.println("mf - Flash access enabled");
  // Serial.println("# - Set flash position to");
  // Serial.println("r # - Read n bytes from flash");
  // Serial.println("w # # # - Write bytes to flash");

  //
  // Storing a config to recover after power-up
  //
  Serial.println("x - save current config to eeprom");
  Serial.println("y - load last config from eeprom");
  Serial.println("z - set all ports to input and low");

  Serial.println("v - Show AVR VCC reading");
  Serial.println("t - Show AVR internal temperature reading");
  Serial.println("f - Show free memory");
  Serial.println("u - Show system uptime (or clock)");
  Serial.println("e - Erase EEPROM");
  Serial.println("* - Reboot");

}

void setPin(int pin, int value) {

  pinMode(pin, OUTPUT);
  digitalWrite(pin, value);
  Serial.println("");
  Serial.print("New value on pin ");
  printPin(pin);
  if(pin < 10) Serial.print(' ');
  Serial.print(": ");
  printHighLow(value);
  Serial.println("");
}

void setup()
{

//  mode = mpModes('n');
  mpMode = mNONE;

  modeI2C.setup();

  Serial.begin(9600);
  Serial.println("ArduPirate: v0.11");

  // Run initial scan
  Serial.println("");
  VCC = readAVR_VCC()/1000.0f;
  if (VCC < 0.0f) VCC = 5.0f;

  mpHelp();
}

void loop()
{
  char c;
  Serial.println("");
  Serial.print(printMode());
  Serial.print(mpMode);
/*  
  Serial.print("I2C");
  if(i2c_address_active >= 0) {
    printStrDec("[",   i2c_address_active);
    printStrHex(" - ", getActiveAddress());
    Serial.print("] ");
  }
*/  
  Serial.print("> ");
  Serial.flush();
  c = pollLowSerial();

  switch (c) {
    case '?':
    case 'h':
       Serial.println("");
       mpHelp();
    break;
	case '*':
		{
			Serial.println("");
			Serial.println("Rebooting...");
			Serial.println("");
			delay(1000);
			void(* resetFunc) (void) = 0; //declare reset function @ address 0
			resetFunc();
		}
		break;
	case 'u':
		{
			Serial.println("");
			unsigned long now = millis();
			Serial.print (now/1000.0f);
			Serial.println (" seconds");
		}
		break;
	case 't':
		{
		Serial.println("");
		int t = readAVRInternalTemp();
		if (t < 0) 	{
			Serial.println ("Not supported on this chip");
			}
		else {
			Serial.print (t/1000.0f);
			Serial.println ("'C");
			}
		}
		break;	
	case 'v':
		{
		Serial.println("");
		VCC = readAVR_VCC()/1000.0;
		if (VCC < 0.0f) 	{
			Serial.println ("Not supported on this chip");
			VCC=5.0f;
			}
		else {
			Serial.print (VCC);
			Serial.println (" Volts");
			}
		}
		break;	
	case 'f':
		Serial.println("");
		Serial.print (freeRam());
		Serial.println (" bytes free");
		
		Serial.print ("EEPROM is ");
		Serial.print (E2END);
		Serial.println (" bytes");
		break;    
	case 'e':

		Serial.println("");
		Serial.print ("Erasing ");
		Serial.print (E2END);
		Serial.println (" bytes....this may take a minute...");
		for (int i=0;i<E2END;i++)
			EEPROM.write(i,0);
		Serial.println("done");

		break;   
	case 'm':
     {
      char d = pollLowSerial();
      Serial.println("");
      switch (d) {
        
        case 'i':
          // Enable I2C
          mpMode = mI2C;
          modeI2C.init();
        break;

        case 's':
          // Enable SPI
        break;

        case 'l':
          // Enable LCD
        break;
        
        case 'm':
          // Enable Memory
        break;

        case 'e':
          // Enable EEPROM
        break;

        case 'f':
          // Enable FLASH
        break;
      }
     }
     break;
    case 'g':
     {
       int pin_nbre = pollPin();
       pollBlanks();
       if(pin_nbre >= 0 && isNumberPeek()) {
		   if (digitalPinHasPWM(pin_nbre))  {
			   int value = pollInt();
				analogWrite(pin_nbre, value);
				Serial.println("");
				Serial.print("New analog value on pin ");
				printPin(pin_nbre);
				printStrDec(": ", value);
				Serial.println();
			   }
		   else {
			   Serial.println("");
			   Serial.print("Pin ");
			   printPin(pin_nbre);
			   Serial.print(" does not support PWM output");
			   Serial.println();
				}
		   }
     }
    break;

    case 's':
     {
       int pin = pollPin();
       pollBlanks();
       if(pin >= 0 && isNumberPeek()) {
           int value = pollInt();
           
           servo.attach(pin);
           servo.write(value);
           
           Serial.println("");
           Serial.print("New servo value on pin ");
           printPin(pin);
           printStrDec(": ", value);
           Serial.println();
           
           // Keep the position until next input
           pollPeek();
           servo.attach(pin);
       }
     }
    break;

    case '/':
     {
      int pin = pollPin();
      if(pin >= 0) {
        setPin(pin,1);
      }
     }
    break;

    case '\\':
     {
      int pin = pollPin();
      if(pin >= 0) {
        setPin(pin, 0);
      }
     }
    break;

    case '^':
     {
      int pin = pollPin();
      if(pin >= 0) {
        setPin(pin, 0);
        setPin(pin, 1);
        setPin(pin, 0);
      }
     }
    break;

    case '<':
     {
      Serial.println();
      int pin = pollPin();
      if(pin >= 0) {
        pinMode(pin, INPUT);
        Serial.println();
        Serial.print("Pin ");
        printPin(pin);
        Serial.println(" is now INPUT");
       }
      }
    break;

    case '>':
     {
      Serial.println();
      int pin = pollPin();
      if(pin >= 0) {
        pinMode(pin, OUTPUT);
        Serial.println();
        Serial.print("Pin ");
        printPin(pin);
        Serial.println(" is now OUTPUT");
       }    
      }
     break;
    
    case 'p':
       Serial.println();
       printPorts();
    break;
	case 'q':
		Serial.println();
		printPortsQuick();
		break;

    case 'i':
       Serial.println();
       mpMode = mI2C;
       modeI2C.init();
    break;

    case '1': case '2': case '3': case '4': case '5':
    case '6': case '7': case '8': case '9': case '0':
      modeI2C.select(c);
    break;

    case 'r':
       pollBlanks();
       if(isNumberPeek()) {
         int read_nbre = pollInt();       
         modeI2C.read(read_nbre);
       }
    break;

    case 'w':
      modeI2C.write(); 
    break;
    
    case 'x':
     {
       // Write all directions to EEPROM
       // Write all digital values to EEPROM
       // Write all pwm values to EEPROM
       for(int i = 0; i < NUM_ANALOG_INPUTS+A0; i++) {
         int pin_mode  = *portModeRegister(digitalPinToPort(i)) & digitalPinToBitMask(i);// != 0;
         int pin_value = digitalRead(i);
		 if (2*i+1 < E2END) { 
			 EEPROM.write(2*i,   pin_mode);
			 EEPROM.write(2*i+1, pin_value);
			 }
       }
     }
     Serial.println();
     Serial.print("Saved digital pins to EEPROM");
    break;
    
    case 'y':
     {
       // Read all directions to EEPROM
       // Read all digital values to EEPROM
       // Read all pwm values to EEPROM
       for(int i = 0; i < NUM_ANALOG_INPUTS+A0; i++) {
		   if (2*i+1 < E2END) { 
			   int pin_mode  = EEPROM.read(2*i);
			   int pin_value = EEPROM.read(2*i + 1);
			   pinMode(i, pin_mode);
			   //if(pin_mode != 0) {
			   digitalWrite(i, pin_value);
			   //}
			   }
       }
     }
     Serial.println();
     Serial.println("Loaded digital pins from EEPROM");
     printPorts();
     break;
     
   case 'z':
     Serial.println();
     Serial.print("Reset ...");
     for(int i = 0; i < NUM_ANALOG_INPUTS+A0; i++) {
       pinMode(i, INPUT);
       digitalWrite(i, LOW);
     }
     break;
  }
}

long readAVR_VCC(long voltage_reference)
	{
	// Read 1.1V reference against AVcc
	// set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
	ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
	ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
	ADMUX = _BV(MUX3) | _BV(MUX2);
#else
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

	delay(2); // Wait for Vref to settle
	ADCSRA |= _BV(ADSC); // Start conversion
	while (bit_is_set(ADCSRA,ADSC)); // measuring

	uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
	uint8_t high = ADCH; // unlocks both

	long result = (high<<8) | low;

	result = voltage_reference / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
	return result; // Vcc in millivolts
	}


long readAVRInternalTemp()
	{
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
	return -1;
#endif
	long result; // Read temperature sensor against 1.1V reference
	ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);
	delay(20); // Wait for Vref to settle - 2 was inadequate
	ADCSRA |= _BV(ADSC); // Convert
	while (bit_is_set(ADCSRA,ADSC));
	result = ADCL;
	result |= ADCH<<8;
	result = (result - 125) * 1075;
	return result;
	}

int freeRam()
	{
	extern int __heap_start, *__brkval;
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
	}