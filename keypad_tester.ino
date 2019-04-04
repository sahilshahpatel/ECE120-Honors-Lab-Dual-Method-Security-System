#include <Keypad.h>

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
  // initialize pinModes
  for(int i = 0; i < ROWS; i++){
    pinMode(rowPins[i], INPUT);
  }
  for(int i = 0; i < COLS; i++){
    pinMode(colPins[i], INPUT);
  }
  
  // Initialize communications
  Serial.begin(9600); // Serial communication with computer
}

void loop() {
  // Read from keypad
  char key = keypad.getKey();
  if(key){
    Serial.print(key);
    Serial.println(" was pressed");
  }
}
