#include <Keypad.h>
#include <EEPROM.h>
#include <SPI.h>
#include <LiquidCrystal.h>
#include <stdbool.h>

// Define pins
#define passAcceptedPin 2 // Output of comparator circuit
#define clockPin 22
#define setPin 24
#define comparePin0 23
#define comparePin1 25

// Define EEPROM addresses
#define INPUT_PASSWORD_BASE_ADDR 0
#define ACCEPTABLE_PASSWORD_BASE_ADDR 5
#define PASSWORD_LENGTH 3

// Define program variables
bool match = false;
bool resettingPassword = false;
byte currentPasswordAddr = 0;

// Set up LCD
LiquidCrystal lcd = LiquidCrystal(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13); //rs, rw, enable, d0-d7S

// Set up 4x3 Matrix Keypad
const byte ROWS = 4;
const byte COLS = 3;
// Define char values (as byte) for each item
const byte keys[ROWS][COLS] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
const byte rowPins[ROWS] = {53, 51, 49, 47};
const byte colPins[COLS] = {52, 50, 48};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

void setup() {
  // intialiaze pinModes
  pinMode(passAcceptedPin, INPUT);
  for(int i = 0; i < ROWS; i++){
    pinMode(rowPins[i], INPUT);
  }
  for(int i = 0; i < COLS; i++){
    pinMode(colPins[i], INPUT);
  }
  for(int i = 3; i<=13; i++){
    pinMode(i, INPUT);
  }
  pinMode(clockPin, OUTPUT);
  pinMode(setPin, OUTPUT);
  pinMode(comparePin0, OUTPUT);
  pinMode(comparePin1, OUTPUT);
  pinMode(passAcceptedPin, INPUT);

  // Initialize communications
  Serial.begin(9600); // Serial communication with computer
  lcd.begin(16, 2);

  int pass[] = {0, 1, 2};
  setAcceptedPassword(pass);

  // Set initial variable states
  reset();

  Serial.println("Setup Complete!");
}

void loop() {  
  // Read from keypad
  char key = keypad.getKey();
  if (key){
    Serial.println(key);
    // Check if # was pressed
    if (key == '#'){
      lcd.clear();

      bool correct = comparePasswords(false); // Use hardware logic
      // If comparator has accepted the password...
      if(resettingPassword){
        byte inputPassword[PASSWORD_LENGTH];
        for(int i = 0; i < PASSWORD_LENGTH; i++){
          inputPassword[i] = EEPROM.read(INPUT_PASSWORD_BASE_ADDR + i);
        }
        setAcceptedPassword(inputPassword);
        resettingPassword = false;

        lcd.clear();
        lcdWriteSecondLine("Passcode Changed");
        delay(1000);
        reset();
      }
      else{
        if(correct){
          granted(1000);
        }
        else{
          denied(1000);
        }
      }
      
      currentPasswordAddr = 0;
    }
    // Check if * was pressed
    else if (key == '*'){
      lcd.clear();

      bool correct = comparePasswords(false); // Use hardware logic
      // If comparator has accepted the password...
      if(correct){
        resettingPassword = true;
        
        lcd.clear();
        lcdWriteSecondLine("Enter New Code");
      }
      else{
        resettingPassword = false;
        denied(1000);
      }
      
      currentPasswordAddr = 0;
    }
    else{
      Serial.print("User entered: ");
      Serial.println(key);
      
      // Record the pressed button into EEPROM if not over max length
      if(currentPasswordAddr < INPUT_PASSWORD_BASE_ADDR + PASSWORD_LENGTH){        
        EEPROM.write(INPUT_PASSWORD_BASE_ADDR + currentPasswordAddr, (byte)charToInt(key));
        currentPasswordAddr += 1;
  
        // Output to LCD
        lcd.write(key);
      }
    }
  }
}

// Helper methods to define other behaviors
void reset(){
  lcd.clear();
  lcdWriteSecondLine("Enter Passcode");
}

void granted(int setDelay){
  lcdWriteSecondLine("Access Granted");
  delay(setDelay);
  reset();
}

void denied(int setDelay){
  lcdWriteSecondLine("Access Denied");
  delay(setDelay);
  reset();
}

void lcdWriteSecondLine(String s){
  lcd.setCursor(0, 1);
  lcd.print(s);
  lcd.setCursor(0, 0);
}

void setAcceptedPassword(int pass[]){
  for(int i = 0; i<PASSWORD_LENGTH; i++){
    EEPROM.write(ACCEPTABLE_PASSWORD_BASE_ADDR + i, (byte)((char)pass[i]));
  }
}

void setAcceptedPassword(byte pass[]){
  for(int i = 0; i<PASSWORD_LENGTH; i++){
    EEPROM.write(ACCEPTABLE_PASSWORD_BASE_ADDR + i, pass[i]);
  }
}

void outputToComparator(byte pass[], int passLength){
  // Set comparison to true initially
  digitalWrite(setPin, HIGH);
  delay(10);
  digitalWrite(setPin, LOW);

  // Iterate over password length
  for(int i = 0; i < passLength; i++){
    // Iterate over relevant bits per byte (value will be <10, only need 4 bits)
    for(int j = 0; j < 4; j++){
      // Output passwords for comparison
      digitalWrite(comparePin0, bitRead(pass[i], j));
      digitalWrite(comparePin1, bitRead(EEPROM.read(ACCEPTABLE_PASSWORD_BASE_ADDR + i), j));

      pulseClock(10);
    }
  }
}

void pulseClock(int period){
  digitalWrite(clockPin, HIGH);
  delay(period);
  digitalWrite(clockPin, LOW);
}

bool comparePasswords(bool debug){
  Serial.println("Comparing passwords...");
  byte inputPassword[PASSWORD_LENGTH];
  
  bool debug_pass_accpt = true;
  
  // Check for equality with code
  for(int i = 0; i < PASSWORD_LENGTH; i++){
    byte input = EEPROM.read(INPUT_PASSWORD_BASE_ADDR + i);
    byte correct = EEPROM.read(ACCEPTABLE_PASSWORD_BASE_ADDR + i);
    inputPassword[i] = input;
    debug_pass_accpt = debug_pass_accpt && (input == correct);
  }

  if(debug){
    return debug_pass_accpt;
  }
  
  outputToComparator(inputPassword, PASSWORD_LENGTH);

  return(digitalRead(passAcceptedPin) == HIGH);
}

int charToInt(char c){
  int i = (int)c;
  if(i >= 48 && i <= 57){
    return i - 48;
  }
  return i;
}
