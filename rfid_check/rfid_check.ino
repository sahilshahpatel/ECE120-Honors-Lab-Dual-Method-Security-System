#include <deprecated.h>
#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <require_cpp11.h>
#include <Adafruit_Keypad.h>
#include <EEPROM.h>
#include <SPI.h>
#include <LiquidCrystal.h>

// Assuming Common Cathode
#define LED_ON LOW
#define LED_OFF HIGH

// Define pins
#define redLed 30 // Password Denied
#define greenLed 31 // Password Accepted
#define blueLed 32 // Standby

// Define program variables
boolean match = false;
uint8_t successRead; // TODO: is this necessary?

// Set up MFRC522 RFID instance
#define SS_PIN 40
#define RST_PIN 5
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Set up LCD
LiquidCrystal lcd(/*TODO: set LCD pins*/);

// Set up 4x3 Matrix Keypad
const byte ROWS = 4;
const byte COLS = 3;
// Define byte values for each item. * = 10, # = 11
const byte keys[ROWS][COLS] = {
  {1, 2, 3},
  {4, 5, 6},
  {7, 8, 9},
  {10, 0, 11}
};
const byte rowPins[ROWS] = {10, 11, 12, 13};
const byte colPins[COLS] = {14, 15, 16};
Adafuit_Keypad keypad = Adafuit_Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  // intialiaze pinModes
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);

  // Set initial variable states
  reset();

  // Initialize communications
  Serial.begin(9600); // Serial communication with computer
  SPI.begin(); // for MFRC522
  mfrc522.PDC_Init();
  lcd.begin(16, 2);
  keypad.begin();

  // Set RFID read range to max
  mrfc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
}

void loop() {
  // Read from keypad
  keypad.tick();

  while(keypad.available()){
    keypadEvent e = keypad.read();
    Serial.print((int)e.bit.KEY);
    if(e.bit.EVENT == KEY_JUST_PRESSED){
      Serial.println(" pressed");
      // Check if # was pressed
      if ((int)e.bit.KEY == keys[ROWS][COLS]){
        // TODO: output recorded data serially
      }
      // Check if * was pressed
      else if ((int)e.bit.KEY == keys[ROWS][0]){
        // TODO: let user reset their password
      }
      else{
        // TODO: record the pressed button into EEPROM
      }
    }
    else if(e.bit.EVENT == KEY_JUST_RELEASED) Serial.println(" released");
  }

  // Read pass
  
  delay(10);
}

// Helper methods to define other behaviors
reset(){
  // Set LEDs to standby
  digitalWrite(redLed, LED_OFF);
  digitalWrite(greenLed, LED_OFF);
  digitalWrite(blueLed, LED_ON);

  // TODO: clear LCD screen
}

void granted(uint16_t setDelay){
  digitalWrite(redLed, LED_OFF);
  digitalWrite(greenLed, LED_ON);
  digitalWrite(blueLed, LED_OFF);
  delay(setDelay)
  reset();
}

void denied(uint16_t setDelay){
  digitalWrite(redLed, LED_ON);
  digitalWrite(greenLed, LED_OFF);
  digitalWrite(blueLed, LED_OFF);
  delay(setDelay);
  reset();
}

// From rfid library example:
uint8_t getID() {
   // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { // If a new PICC placed to RFID reader continue
    return 0;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   // Since a PICC placed get Serial and continue
    return 0;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  // Serial.println(F("Scanned PICC's UID:"));
  for ( uint8_t i = 0; i < 4; i++) {  //
    readCard[i] = mfrc522.uid.uidByte[i];
    Serial.print(readCard[i], HEX);
  }
  Serial.println("");
  mfrc522.PICC_HaltA(); // Stop reading
  return 1;
}
