#include <Keypad.h>

#include "GetKeypad.h"

const byte ROWS = 4; //four rows
const byte COLS = 4; //three columns
char keys[ROWS][COLS] = {
{'1','2','3','A'},
{'4','5','6','B'},
{'7','8','9','C'},
{'*','0','#','D'}
};
byte rowPins[ROWS] = {11, 10, 9, 8}; //connect to the row pinouts of the kpd
byte colPins[COLS] = {7, 6, 5, 4}; //connect to the column pinouts of the kpd

Keypad kpd = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );

byte keypad_buffer_index;
char keypad_buffer[5];

void initKeypad() {
  keypad_buffer_index = 0;
}

bool getKeypad() {

  // Fills kpd.key[ ] array with up-to 10 active keys.
  // Returns true if there are ANY active keys.
  if (kpd.getKeys())
  {
    for (int i=0; i<LIST_MAX; i++)   // Scan the whole key list.
    {
      if ( kpd.key[i].stateChanged )   // Only find keys that have changed state.
      {
        switch (kpd.key[i].kstate) {  // Report active key state : IDLE, PRESSED, HOLD, or RELEASED
          case PRESSED:
            break;
          case HOLD:
            break;
          case RELEASED:
            keypad_buffer[keypad_buffer_index++] = kpd.key[i].kchar;
            if( keypad_buffer_index >=4 ) {
              keypad_buffer[4] = 0;
              keypad_buffer_index = 0;
              return true;
            }
            break;
          case IDLE:
            break;
        }
      }
    }
  }
  return false;
}

char *getKeypadData() {
  return keypad_buffer;
}