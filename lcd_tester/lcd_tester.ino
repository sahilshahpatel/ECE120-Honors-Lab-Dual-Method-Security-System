#include <Keypad.h>
#include <LiquidCrystal.h>

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
const byte rowPins[ROWS] = {53, 51, 49, 47};
const byte colPins[COLS] = {52, 50, 48};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Set up LCD Display
LiquidCrystal lcd = LiquidCrystal(3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13); //rs, rw, enable, d0-d7S

void setup() {
  // initialize pinModes
  for(int i = 0; i < ROWS; i++){
    pinMode(rowPins[i], INPUT);
  }
  for(int i = 0; i < COLS; i++){
    pinMode(colPins[i], INPUT);
  }
  for(int i = 3; i <=13; i++){
    pinMode(i, INPUT);
  }
  
  // Initialize communications
  Serial.begin(9600); // Serial communication with computer

  // Start LCD
  lcd.begin(16, 2); // 16x2 Display
}

void loop() {
  // Read from keypad
  char key = keypad.getKey();
  if(key){
    Serial.print(key);
    Serial.println(" was pressed");
    
    lcd.write(key);
    if(key == '#' || key == '*'){
      lcd.clear();
      lcd.print("Reseting...");
      delay(500); // 0.5 seconds
      lcd.clear();
    }
  }
}
