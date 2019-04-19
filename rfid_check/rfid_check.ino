#include <MFRC522.h>
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
#define PASSWORD_LENGTH 4
#define ACCEPTABLE_PASSWORD_BASE_ADDR INPUT_PASSWORD_BASE_ADDR + PASSWORD_LENGTH

#define INPUT_RFID_BASE_ADDR ACCEPTABLE_PASSWORD_BASE_ADDR + PASSWORD_LENGTH
#define RFID_LENGTH 4
#define ACCEPTABLE_RFID_BASE_ADDR INPUT_RFID_BASE_ADDR + RFID_LENGTH

// Define program variables
bool match = false;
bool resettingPassword = false;
byte currentPasswordAddr = 0;
bool successRead = false;

bool firstCard = true;

const int lockDelay = 2000; // Lock stays open for 2 seconds
const int denyDelay = 1000; // User must wait 1 seconds between attempts

// Set up MFRC522 RFID instance
#define SS_PIN 53
#define RST_PIN 48
MFRC522 mfrc522(SS_PIN, RST_PIN);

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
const byte rowPins[ROWS] = {33, 31, 29, 27};
const byte colPins[COLS] = {30, 28, 26};
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
  SPI.begin(); // for MFRC522
  mfrc522.PCD_Init();
  lcd.begin(16, 2);

  // Set RFID read range to max
  mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

  int pass[PASSWORD_LENGTH];
  for(int i = 0; i<PASSWORD_LENGTH; i++){
    pass[i] = i;
  }
  setAcceptedPassword(pass);

  // Set initial variable states
  reset();

  Serial.println("Setup Complete!");
}

void loop() {
  // Check RFID
  if (!successRead){
    // Read RFID if a card is near
    successRead = getID();
  }
  else{
    Serial.print("RFID: ");
    for(int i = 0; i < 4; i++){
      Serial.print(EEPROM.read(INPUT_RFID_BASE_ADDR + i), HEX);
    }
    Serial.println();

    if(!firstCard){
      // Check to see if accepted RFID     
      bool correct = compareRFID();
      if(correct){
        granted();
      }
      else{
        denied();
      }
    }
    else{
      // Set first RFID to the accepted
      Serial.println("First Card... setting as accepted");
      byte accepted[RFID_LENGTH];
      for(int i = 0; i < RFID_LENGTH; i++){
        accepted[i] = EEPROM.read(INPUT_RFID_BASE_ADDR + i);
      }
      
      setAcceptedRFID(accepted);
      
      Serial.print("Accepted RFID: ");
      for(int i = 0; i < RFID_LENGTH; i++){
        Serial.print(EEPROM.read(ACCEPTABLE_RFID_BASE_ADDR + i), HEX);
      }
      Serial.println();
      
      firstCard = false;
    }
    
    successRead = false; // Reset and prepare for next card
  }
  // Read from keypad
  char key = keypad.getKey();
  if (key){
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
        delay(lockDelay);
        reset();
      }
      else{
        if(correct){
          granted();
        }
        else{
          denied();
        }
      }
    }
    // Check if * was pressed
    else if (key == '*'){
      lcd.clear();

      bool correct = comparePasswords(false); // Use hardware logic
      // If comparator has accepted the password...
      if(correct){
        resettingPassword = true;

        reset();
        lcdWriteSecondLine("Enter New Code");
      }
      else{
        resettingPassword = false;
        denied();
      }
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
  currentPasswordAddr = 0;
  lcd.clear();
  lcdWriteSecondLine("Enter Passcode");
}

void granted(){
  Serial.println("*** Access Granted ***");
  lcd.clear();
  lcdWriteSecondLine("Access Granted");
  delay(lockDelay);
  reset();
}

void denied(){
  Serial.println("*** Access Denied ***");
  lcd.clear();
  lcdWriteSecondLine("Access Denied");
  delay(denyDelay);
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

void setAcceptedRFID(byte pass[]){
  for(int i = 0; i < RFID_LENGTH; i++){
    EEPROM.write(ACCEPTABLE_RFID_BASE_ADDR + i, pass[i]);
  }
}

void outputToComparator(byte pass[], int passLength, bool rfid){
  // Set comparison to true initially
  digitalWrite(setPin, HIGH);
  delay(10);
  digitalWrite(setPin, LOW);

  // Iterate over password length
  for(int i = 0; i < passLength; i++){
    // Iterate over relevant bits per byte
    for(int j = 0; j < 8; j++){
      // Output passwords for comparison
      digitalWrite(comparePin0, bitRead(pass[i], j));
      if(rfid){
        digitalWrite(comparePin1, bitRead(EEPROM.read(ACCEPTABLE_RFID_BASE_ADDR + i), j));
      }
      else{
        digitalWrite(comparePin1, bitRead(EEPROM.read(ACCEPTABLE_PASSWORD_BASE_ADDR + i), j));
      }
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
  
  outputToComparator(inputPassword, PASSWORD_LENGTH, false);

  return(digitalRead(passAcceptedPin) == HIGH);
}

bool compareRFID(){
  byte inputRFID[RFID_LENGTH];

  for(int i = 0; i<RFID_LENGTH; i++){
    byte input = EEPROM.read(INPUT_RFID_BASE_ADDR + i);
    inputRFID[i] = input;
  }

  outputToComparator(inputRFID, RFID_LENGTH, true);

  return(digitalRead(passAcceptedPin) == HIGH);
}

int charToInt(char c){
  int i = (int)c;
  if(i >= 48 && i <= 57){
    return i - 48;
  }
  return i;
}

// From GitHub example
bool getID() {
   // Getting ready for Reading PICCs
  if ( ! mfrc522.PICC_IsNewCardPresent()) { // If a new PICC placed to RFID reader continue
    //Serial.println("No new RFID card");
    return false;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {   // Since a PICC placed get Serial and continue
    Serial.println("Unknown Error in RFID");
    return false;
  }
  // There are Mifare PICCs which have 4 byte or 7 byte UID care if you use 7 byte PICC
  // I think we should assume every PICC as they have 4 byte UID
  // Until we support 7 byte PICCs
  for ( int i = 0; i < RFID_LENGTH; i++) {
    EEPROM.write(INPUT_RFID_BASE_ADDR + i, mfrc522.uid.uidByte[i]);
  }
  mfrc522.PICC_HaltA(); // Stop reading
  return true;
}
