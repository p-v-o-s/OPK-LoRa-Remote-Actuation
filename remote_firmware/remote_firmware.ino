//#include <EEPROM.h>
#include <PacketCommand.h>
#include <RHReliableDatagram.h>
#include <RH_RF95.h>
#include <SPI.h>
#include "pCmd_RHRD_RF95_module.h"

#include <SerialCommand.h>

//#include <FreqCount.h>

//uncomment for debugging messages
#define DEBUG

#ifdef DEBUG
  #define DEBUG_PORT Serial
#endif



#define LED_pin 13   /* Arduino LED on board */
//#define LED_pin 9    /* Moteino LED on board */

#define SQUAWKBOX_HAS_RGBLED

bool LED_state = LOW;



//Serial Interface for Testing over FDTI
SerialCommand sCmd_USB(Serial);

#define PACKETCOMMAND_MAX_COMMANDS 20
#define PACKETCOMMAND_INPUT_BUFFER_SIZE 32
#define PACKETCOMMAND_OUTPUT_BUFFER_SIZE 32
PacketCommand pCmd_RHRD(PACKETCOMMAND_MAX_COMMANDS,
                        PACKETCOMMAND_INPUT_BUFFER_SIZE,
                        PACKETCOMMAND_OUTPUT_BUFFER_SIZE);

#define BASESTATION_ADDRESS 1
#define DEFAULT_REMOTE_ADDRESS 2

#ifdef SQUAWKBOX_HAS_RGBLED
void rgb_led(uint8_t r, uint8_t g, uint8_t b){
  analogWrite(5 ,r);
  analogWrite(10,g);
  analogWrite(11,b);
}
#endif
//******************************************************************************
// Setup
void setup() {
  pinMode(LED_pin, OUTPUT);      // Configure the onboard LED for output
  //FreqCount.begin(1000); //this is the gateing interval/duration of measurement
  #ifdef SQUAWKBOX_HAS_RGBLED
  //setup PWM pins for RGB LED
  pinMode(5,OUTPUT);
  pinMode(10,OUTPUT);
  pinMode(11,OUTPUT);
  rgb_led(255,0,0);   //red
  delay(250);
  rgb_led(0,255,0);   //green
  delay(250);
  rgb_led(0,0,255);   //blue
  delay(250);
  rgb_led(0,255,255); //cyan
  delay(250);
  rgb_led(255,0,255); //magenta
  delay(250);
  rgb_led(255,255,0); //yellow
  delay(250);
  rgb_led(255,255,255); //white
  delay(250);
  rgb_led(0,0,0); //off
  #endif
  
  Serial.begin(115200);

  // Setup callbacks for SerialCommand commands
  //sCmd_USB.addCommand("FREQ1.READ?",   FREQ1_READ_sCmd_query_handler);       // reads input frequency on pin 5
  sCmd_USB.addCommand("DIGITAL.READ?", DIGITAL_READ_sCmd_query_handler);
  sCmd_USB.addCommand("DIGITAL.WRITE", DIGITAL_WRITE_sCmd_action_handler);
  sCmd_USB.addCommand("ANALOG.READ?",  ANALOG_READ_sCmd_query_handler);
  sCmd_USB.addCommand("ANALOG.WRITE",  ANALOG_WRITE_sCmd_action_handler);
  sCmd_USB.addCommand("LED?",          LED_sCmd_query_handler);
  sCmd_USB.addCommand("LED.ON",        LED_ON_sCmd_action_handler);          // Turns LED on
  sCmd_USB.addCommand("LED.OFF",       LED_OFF_sCmd_action_handler);         // Turns LED off
  // Serial commands for non-volatile configuration (EEPROM)
  //sCmd_USB.addCommand("EEPROM.READ?", EEPROM_READ_sCmd_query_handler);
  //sCmd_USB.addCommand("EEPROM.WRITE", EEPROM_WRITE_sCmd_action_handler);
  sCmd_USB.setDefaultHandler(UNRECOGNIZED_sCmd_error_handler);      // Handler for command that isn't matched  (says "What?")
  #ifdef DEBUG
  DEBUG_PORT.println(F("# SerialCommand Ready"));
  #endif
  
  // Setup callbacks and command for PacketCommand interface
  // Setup callbacks for PacketCommand actions and queries
  //pCmd_RHRD.addCommand((byte*) "\xFF\x11","FREQ1.READ?",    FREQ1_READ_pCmd_query_handler);
  pCmd_RHRD.addCommand((byte*) "\xFF\x21","DIGITAL.READ?",  DIGITAL_READ_pCmd_query_handler);
  pCmd_RHRD.addCommand((byte*) "\xFF\x22","DIGITAL.WRITE",  DIGITAL_WRITE_pCmd_action_handler);
  pCmd_RHRD.addCommand((byte*) "\xFF\x23","ANALOG.READ?",   ANALOG_READ_pCmd_query_handler);
  pCmd_RHRD.addCommand((byte*) "\xFF\x24","ANALOG.WRITE",   ANALOG_WRITE_pCmd_action_handler);
  pCmd_RHRD.addCommand((byte*) "\xFF\x40","LED?",        LED_pCmd_query_handler);
  pCmd_RHRD.addCommand((byte*) "\xFF\x41","LED.ON",      LED_ON_pCmd_action_handler);            // Turns LED on   ("\x41" == "A")
  pCmd_RHRD.addCommand((byte*) "\xFF\x42","LED.OFF",     LED_OFF_pCmd_action_handler);           // Turns LED off  ("
  //Setup type IDs for PacketCommand replies
  pCmd_RHRD.addCommand((byte*) "\x11","FREQ1!",          NULL);
  pCmd_RHRD.addCommand((byte*) "\x21","DIGITAL!",        NULL);
  pCmd_RHRD.addCommand((byte*) "\x23","ANALOG!",         NULL);
  pCmd_RHRD.addCommand((byte*) "\x40","LED!",            NULL);
  //pCmd.registerDefaultHandler(unrecognized);                          // Handler for command that isn't matched  (says "What?")
  pCmd_RHRD.registerRecvCallback(pCmd_RHRD_recv_callback);
  pCmd_RHRD.registerSendCallback(pCmd_RHRD_send_callback);
  
  //configure the pCmd_RHRD_RF95_module
  pCmd_RHRD_module_setup(DEFAULT_REMOTE_ADDRESS,
                         PCMD_RHRD_DEFAULT_FREQUENCY,
                         PCMD_RHRD_DEFAULT_TX_POWER,
                         1,//PCMD_RHRD_DEFAULT_NUM_RETRIES,
                         PCMD_RHRD_DEFAULT_TIMEOUT
                         );
                         
  //configure the default node address
  pCmd_RHRD.setOutputToAddress(BASESTATION_ADDRESS);
  
  //blink to signal setup complete
  digitalWrite(LED_pin, HIGH);
  delay(500);
  digitalWrite(LED_pin, LOW);     // default to LED off
  LED_state = LOW;
}

//******************************************************************************
// Main Loop

void loop() {
  int num_bytes = sCmd_USB.readSerial();      // fill the buffer
  if (num_bytes > 0){
    sCmd_USB.processCommand();  // process the command
  }

  //process incoming radio packets
  pCmd_RHRD_module_proccess_input(pCmd_RHRD);
  
  delay(10);
}

//******************************************************************************
// SerialCommand handlers

/*void FREQ1_READ_sCmd_query_handler(SerialCommand this_sCmd) {*/
/* if (FreqCount.available()) {*/
/*    unsigned long count = FreqCount.read();*/
/*    Serial.println(count);*/
/*  }*/
/*}*/

void DIGITAL_READ_sCmd_query_handler(SerialCommand this_sCmd){
  int pin;
  bool state;
  char *arg = this_sCmd.next();
  if (arg == NULL){
    Serial.print(F("### Error: DIGITAL.READ requires 1 argument (int pin)\n"));
  }
  else{
    pin = atoi(arg);
    pinMode(pin,INPUT);
    state = digitalRead(pin);
    Serial.print(state);
    Serial.print('\n');
  }
}

void DIGITAL_WRITE_sCmd_action_handler(SerialCommand this_sCmd){
  int pin;
  bool state;
  char *arg = this_sCmd.next();
  if (arg == NULL){
    Serial.print(F("### Error: DIGITAL.WRITE requires 2 arguments (int pin, byte value), none given\n"));
  }
  else{
    pin = atoi(arg);
    arg = this_sCmd.next();
    if (arg == NULL){
        Serial.print(F("### Error: DIGITAL.WRITE requires 2 arguments (int pin, byte value), 1 given\n"));
    }
    else{
        state = atoi(arg);
        digitalWrite(pin, state);
    }
  }
}

void ANALOG_READ_sCmd_query_handler(SerialCommand this_sCmd){
  int pin;
  uint16_t value;
  char *arg = this_sCmd.next();
  if (arg == NULL){
    Serial.print(F("### Error: ANALOG.READ requires 1 argument (int pin)\n"));
  }
  else{
    pin = atoi(arg);
    value = analogRead(pin);
    Serial.print(value);
    Serial.print('\n');
  }
}

void ANALOG_WRITE_sCmd_action_handler(SerialCommand this_sCmd){
  int pin;
  uint16_t value;
  char *arg = this_sCmd.next();
  if (arg == NULL){
    Serial.print(F("### Error: ANALOG.WRITE requires 2 arguments (int pin, byte value), none given\n"));
  }
  else{
    pin = atoi(arg);
    arg = this_sCmd.next();
    if (arg == NULL){
        Serial.print(F("### Error: ANALOG.WRITE requires 2 arguments (int pin, byte value), 1 given\n"));
    }
    else{
        value = atoi(arg);
        analogWrite(pin, value);
    }
  }
}

void LED_sCmd_query_handler(SerialCommand this_sCmd) {
  Serial.println(LED_state);
}

void LED_ON_sCmd_action_handler(SerialCommand this_sCmd) {
  Serial.println(F("# LED on"));
  digitalWrite(LED_pin, HIGH);
  LED_state = HIGH;
}

void LED_OFF_sCmd_action_handler(SerialCommand this_sCmd) {
  Serial.println(F("# LED off"));
  digitalWrite(LED_pin, LOW);
  LED_state = LOW;
}

void EEPROM_READ_sCmd_query_handler(SerialCommand this_sCmd){
  int addr;
  byte data;
  char *arg = this_sCmd.next();
  if (arg == NULL){
    Serial.print(F("### Error: EEPROM.READ requires 1 argument (int addr)\n"));
  }
  else{
    addr = atoi(arg);
    //data = EEPROM.read(addr);
    Serial.print(data);
    Serial.print('\n');
  }
}

void EEPROM_WRITE_sCmd_action_handler(SerialCommand this_sCmd){
  int addr;
  byte value;
  char *arg = this_sCmd.next();
  if (arg == NULL){
    Serial.print(F("### Error: EEPROM.WRITE requires 2 arguments (int addr, byte value), none given\n"));
  }
  else{
    addr = atoi(arg);
    arg = this_sCmd.next();
    if (arg == NULL){
        Serial.print(F("### Error: EEPROM.WRITE requires 2 arguments (int addr, byte value), 1 given\n"));
    }
    else{
        value = atoi(arg);
        //EEPROM.write(addr, value);
    }
  }
}

// This gets set as the default handler, and gets called when no other command matches.
void UNRECOGNIZED_sCmd_error_handler(SerialCommand this_sCmd) {
  SerialCommand::CommandInfo command = this_sCmd.getCurrentCommand();
  Serial.print(F("# Did not recognize \""));
  Serial.print(command.name);
  Serial.println(F("\" as a command."));
}
//******************************************************************************
// PacketCommand handlers
/*void FREQ1_READ_pCmd_query_handler(PacketCommand& this_pCmd) {*/
/* if (FreqCount.available()) {*/
/*    uint32_t count = FreqCount.read();*/
/*    this_pCmd.resetOutputBuffer();*/
/*    this_pCmd.setupOutputCommandByName("FREQ1!");*/
/*    this_pCmd.pack_uint32(count);*/
/*    this_pCmd.send();*/
/*  }*/
/*}*/

void DIGITAL_READ_pCmd_query_handler(PacketCommand& this_pCmd){
  #if defined(DEBUG)
  DEBUG_PORT.println(F("# DIGITAL_READ_pCmd_query_handler"));
  #endif
  uint8_t pin;
  this_pCmd.unpack_uint8(pin);
  pinMode(pin,INPUT); //change the pinmode
  delayMicroseconds(10);
  bool state = digitalRead(pin);
  //construct reply packet
  this_pCmd.resetOutputBuffer();
  this_pCmd.setupOutputCommandByName("DIGITAL!");
  this_pCmd.pack_uint8(pin);
  this_pCmd.pack_byte((byte) state);
  this_pCmd.send();
}

void DIGITAL_WRITE_pCmd_action_handler(PacketCommand& this_pCmd){
  #if defined(DEBUG)
  DEBUG_PORT.println(F("# DIGITAL_WRITE_pCmd_action_handler"));
  #endif
  uint8_t pin;
  this_pCmd.unpack_uint8(pin);
  bool state;
  this_pCmd.unpack_byte((byte&) state);
  //complete the action
  pinMode(pin,OUTPUT); //change the pinmode
  digitalWrite(pin, state);
}


void ANALOG_READ_pCmd_query_handler(PacketCommand& this_pCmd){
  #if defined(DEBUG)
  DEBUG_PORT.println(F("# ANALOG_READ_pCmd_query_handler"));
  #endif
  uint8_t pin;
  this_pCmd.unpack_uint8(pin);
  pinMode(pin,INPUT); //change the pinmode
  delayMicroseconds(10);
  uint16_t value = analogRead(pin);
  //construct reply packet
  this_pCmd.resetOutputBuffer();
  this_pCmd.setupOutputCommandByName("ANALOG!");
  this_pCmd.pack_uint8(pin);
  this_pCmd.pack_uint16(value);
  this_pCmd.send();
}

void ANALOG_WRITE_pCmd_action_handler(PacketCommand& this_pCmd){
  #if defined(DEBUG)
  DEBUG_PORT.println(F("# ANALOG_WRITE_pCmd_action_handler"));
  #endif
  uint8_t pin;
  this_pCmd.unpack_uint8(pin);
  uint16_t value;
  this_pCmd.unpack_uint16(value);
  //complete the action
  #if defined(ARDUINO_SAMD_ZERO)
  switch(pin){
    case 5:
    case 6:
    case 10:
    case 11:
    case 12:
      //8-bit PWM
      value = constrain(value,0,255);
      pinMode(pin,OUTPUT); //change the pinmode
      analogWrite(pin, (uint8_t) value);
      break;
    case 14: //==A0
      //10-bit DAC
      pinMode(pin,OUTPUT); //change the pinmode
      analogWrite(pin, (uint16_t) value);
    default:
      break;
  };
  #else
  value = constrain(value,0,255);
  pinMode(pin,OUTPUT); //change the pinmode
  analogWrite(pin, (uint8_t) value);
  #endif
}

void LED_pCmd_query_handler(PacketCommand& this_pCmd) {
  this_pCmd.resetOutputBuffer();
  this_pCmd.setupOutputCommandByName("LED!");
  this_pCmd.pack_byte((byte) LED_state);
  this_pCmd.send();
}

void LED_ON_pCmd_action_handler(PacketCommand& this_pCmd) {
  #ifdef SQUAWKBOX_HAS_RGBLED
  PacketCommand::InputProperties input_props = this_pCmd.getInputProperties();
  if(input_props.RSSI > -75){        //strong signal
    rgb_led(0,255,0); //green
  } else if(input_props.RSSI > -85){ //acceptable signal
    rgb_led(0,0,255); //blue
  } else if(input_props.RSSI > -95){ //marginal signal
    rgb_led(255,255,0); //yellow
  } else{                            //very low or no signal
    rgb_led(255,0,0); //red
  }
  #else
  digitalWrite(LED_pin,HIGH);
  #endif
  LED_state = HIGH;
}

void LED_OFF_pCmd_action_handler(PacketCommand& this_pCmd) {
  #ifdef SQUAWKBOX_HAS_RGBLED
  rgb_led(0,0,0); //off
  #else
  digitalWrite(LED_pin,LOW);
  #endif
  LED_state = LOW;
}

