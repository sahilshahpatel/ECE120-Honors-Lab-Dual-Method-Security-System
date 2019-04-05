#include <deprecated.h>
#include <MFRC522.h>
#include <MFRC522Extended.h>
#include <require_cpp11.h>
#include <Keypad.h>
#include <EEPROM.h>
#include <SPI.h>
#include <LiquidCrystal.h>
#include <stdbool.h>

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
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
const byte rowPins[ROWS] = {28, 30, 32, 34};
const byte colPins[COLS] = {22, 24, 26};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  // intialiaze pinModes
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(inputPassPin, OUTPUT);
  pinMode(accptPassPin, OUTPUT);
  pinMode(passAcceptedPin, INPUT);

  for(int i = 0; i < ROWS; i++){
    pinMode(rowPins[i], INPUT);
  }
  for(int i = 0; i < COLS; i++){
    pinMode(colPins[i], INPUT);
  }
  
  // Set initial variable states
  reset();

  // Initialize communications
  Serial.begin(9600); // Serial communication with computer
  SPI.begin(); // for MFRC522
  mfrc522.PCD_Init();
  lcd.begin(16, 2);

  // Set RFID read range to max
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  // Set accepted password to '0123'
  for(int i = 0; i<PASSWORD_LENGTH; i++){
    EEPROM.write(ACCEPTABLE_PASSWORD_BASE_ADDR + i, (byte)((char)i));
  }

  Serial.println("Setup Complete!");
}

void loop() {  
  // Read from keypad
  char key = keypad.getKey();
  if (key){
    // Check if # was pressed
    if (key == '#'){
      lcd.clear();

      bool debug_pass_accpt = true;
      Serial.println("Comparing passwords...");
      
      byte inputPassword[PASSWORD_LENGTH];

      // Check for equality with code
      for(int i = 0; i < PASSWORD_LENGTH; i++){
        byte input = EEPROM.read(INPUT_PASSWORD_BASE_ADDR + i);
        byte correct = EEPROM.read(ACCEPTABLE_PASSWORD_BASE_ADDR + i);
        inputPassword[i] = input;
        debug_pass_accpt = debug_pass_accpt && (input == correct);
      }

      if(debug_pass_accpt){
        Serial.println("***PASS ACCEPTED***");
      }
      
      outputToComparator(inputPassword, PASSWORD_LENGTH, 100);
      
      // If comparator has accepted the password...
      if(digitalRead(passAcceptedPin) == HIGH){
        granted(10);
      }
      else{
        denied(10);
      }
      
      currentPasswordAddr = 0;
    }
    // Check if * was pressed
    else if (key == '*'){
      /** TODO: let user reset their password
       *  Have this go through the same process as if they pressed #,
       *  but this time if the password is accepted, prompt them to enter a new
       *  password within X seconds that will replace their old one.
       */
    }
    else{
      // Record the pressed button into EEPROM if not over max length
      if(currentPasswordAddr < INPUT_PASSWORD_BASE_ADDR + PASSWORD_LENGTH){        
        EEPROM.write(INPUT_PASSWORD_BASE_ADDR + currentPasswordAddr, (byte)charToInt(key));
        currentPasswordAddr += 1;
  
        // Output to LCD
        lcd.write(key);
        Serial.print("User entered: ");
        Serial.println(key);
      }
    }
  }
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
      digitalWrite(accptPassPin, bitRead(EEPROM.read(ACCEPTABLE_PASSWORD_BASE_ADDR + i), j));

      // Output clock cycle with specified period
      digitalWrite(clockPin, HIGH);
      delay(clockDelay/2);
      digitalWrite(clockPin, LOW);
      delay(clockDelay/2);
    }
  }
}

int charToInt(char c){
  int i = (int)c;
  if(i >= 48 && i <= 57){
    return i - 48;
  }
  return i;
}
