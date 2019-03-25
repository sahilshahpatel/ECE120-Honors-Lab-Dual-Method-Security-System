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
#define clockPin 10
#define inputPassPin 11
#define accptPassPin 12
#define passAcceptedPin 19 // Output of comparator circuit
#define passComparisonDone 20 // Async 'done' signal of comparator circuit

// Define EEPROM addresses
#define INPUT_PASSWORD_BASE_ADDR 0
#define ACCEPTABLE_PASSWORD_BASE_ADDR 5
#define PASSWORD_LENGTH 4

// Define program variables
boolean match = false;
byte currentPasswordAddr = 0;
const int clockDelay = 10;

// Set up MFRC522 RFID instance
#define SS_PIN 40
#define RST_PIN 5
MFRC522 mfrc522(SS_PIN, RST_PIN);

// Set up LCD
LiquidCrystal lcd = LiquidCrystal(1, 2, 3, 4, 5, 6/*TODO: set LCD pins*/);

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
Adafruit_Keypad keypad = Adafruit_Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  // intialiaze pinModes
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(inputPassPin, OUTPUT);
  pinMode(accptPassPin, OUTPUT);
  pinMode(passAcceptedPin, INPUT);
  
  // Set initial variable states
  reset();

  // Initialize communications
  Serial.begin(9600); // Serial communication with computer
  SPI.begin(); // for MFRC522
  mfrc522.PCD_Init();
  lcd.begin(16, 2);
  keypad.begin();

  // Set RFID read range to max
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);
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
        byte inputPassword[PASSWORD_LENGTH];
        int clockDelay = 100;
        for(int i = 0; i < PASSWORD_LENGTH; i++){
          inputPassword[i] = EEPROM.read(INPUT_PASSWORD_BASE_ADDR + i);
        }
        outputToComparator(inputPassword, PASSWORD_LENGTH, clockDelay);
        // If comparator has accepted the password...
        while(digitalRead(passComparisonDone) == LOW){
            // Wait for passComparisonDone to be HIGH
        }
        if(digitalRead(passAcceptedPin) == HIGH){
          granted(10);
        }
        else{
          denied(10);
        }
        
        currentPasswordAddr = 0;
      }
      // Check if * was pressed
      else if ((int)e.bit.KEY == keys[ROWS][0]){
        /** TODO: let user reset their password
         *  Have this go through the same process as if they pressed #,
         *  but this time if the password is accepted, prompt them to enter a new
         *  password within X seconds that will replace their old one.
         */
      }
      else{
        // Record the pressed button into EEPROM
        if(currentPasswordAddr < INPUT_PASSWORD_BASE_ADDR + PASSWORD_LENGTH){        
          EEPROM.write(INPUT_PASSWORD_BASE_ADDR + currentPasswordAddr, (int)e.bit.KEY);
          currentPasswordAddr += 1;
        }
      }
    }
    else if(e.bit.EVENT == KEY_JUST_RELEASED){
      Serial.println(" released");
    }
  }

  // Read pass
  
  delay(10);
}

// Helper methods to define other behaviors
void reset(){
  // Set LEDs to standby
  digitalWrite(redLed, LED_OFF);
  digitalWrite(greenLed, LED_OFF);
  digitalWrite(blueLed, LED_ON);

  // TODO: clear LCD screen
  // TODO: set comparator done signal LOW
}

void granted(uint16_t setDelay){
  digitalWrite(redLed, LED_OFF);
  digitalWrite(greenLed, LED_ON);
  digitalWrite(blueLed, LED_OFF);
  delay(setDelay);
  reset();
}

void denied(uint16_t setDelay){
  digitalWrite(redLed, LED_ON);
  digitalWrite(greenLed, LED_OFF);
  digitalWrite(blueLed, LED_OFF);
  delay(setDelay);
  reset();
}

void outputToComparator(byte pass[], int passLength, int clockDelay){
  // Iterate over password length
  for(int i = 0; i < passLength; i++){
    // Iterate over bits per byte
    for(int j = 0; j < 8; j++){
      // Output passwords for comparison
      digitalWrite(inputPassPin, bitRead(pass[i], j));
      digitalWrite(accptPassPin, bitRead(pass[i], j));

      // Output clock cycle with specified period
      digitalWrite(clockPin, HIGH);
      delay(clockDelay/2);
      digitalWrite(clockPin, LOW);
      delay(clockDelay/2);
    }
  }
}
