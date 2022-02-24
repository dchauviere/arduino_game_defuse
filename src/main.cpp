#include <Arduino.h>
#include <GetKeypad.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


// CONFIG
#define KEYCODE "739C"
#define PTM_MIN 30
#define PTM_MAX 40
#define START_COUNTER_MILLIS 60*60*1000

int melodieSuccess[] = {262, 196, 196, 220, 196, 247, 262};
int dureeNoteSuccess[] = {4,8,8,4,4,4,4,4 };
int melodieSizeSuccess = 7;
int melodieFailed[] = {262, 196, 196, 220, 196, 247, 262};
int dureeNoteFailed[] = {4,8,8,4,4,4,4,4 };
int melodieSizeFailed = 7;

#define KEY_PIN 13
#define WIRE_PIN 12
#define LED_PIN 3
#define BUZZER_PIN 2

// OLED
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int led_status;

unsigned long previousMillis  = 0;
unsigned long currentMillis   = 0;
unsigned long interval = 1000;
long counterMillis = (long) START_COUNTER_MILLIS;

#define DEFUSE_CODE    0
#define DEFUSE_KEY     1
#define DEFUSE_WIRE    2
#define DEFUSE_PTM     3

bool defuse_status[4];
bool end_of_game;

void setup() {
  Serial.begin(9600);
  end_of_game = false;
  led_status = LOW;

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  display.display();
  delay(2000); // Pause for 2 seconds
  // Clear the buffer
  display.clearDisplay();

  initKeypad();
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(KEY_PIN, INPUT);
  pinMode(WIRE_PIN, INPUT);

  defuse_status[DEFUSE_CODE] = false;
  defuse_status[DEFUSE_KEY] = false;
  defuse_status[DEFUSE_WIRE] = false;
  defuse_status[DEFUSE_PTM] = false;
}

bool checkDefuseCode() {
  char *keyCodeValue;
  // Keypad
  if ( getKeypad() ) {
    keyCodeValue = getKeypadData();
    Serial.print("Code entered : ");
    Serial.println(keyCodeValue);
    if(strcmp(keyCodeValue, KEYCODE) == 0) {
      return true;
    }
  }
  return false;
}

bool checkDefuseKey() {
  if ( digitalRead(KEY_PIN) == 1 ) {
    return true;
  }
  return false;
}

bool checkDefuseWire() {
  if ( digitalRead(WIRE_PIN) == 0 ) {
    return true;
  }
  return false;  
}

bool checkDefusePTM() {
  int ptm_value = map(analogRead(A0), 0, 1023, 0, 255);

  if ( ptm_value >= PTM_MIN && ptm_value <= PTM_MAX ) {
    return true;
  }
  return false;
}

void playSound(int *melodie, int *dureeNote, int size) {
  int i;
  for (i = 0; i < size; i++){
    tone(9, melodie[i], 1000/dureeNote[i]);
    delay(1300/dureeNote[i]);
    noTone(9);
  }
}

bool checkDefuse() {
  Serial.print("Code status : ");
  Serial.println(defuse_status[DEFUSE_CODE]);

  Serial.print("Key status : ");
  Serial.println(defuse_status[DEFUSE_KEY]);

  Serial.print("Wire status : ");
  Serial.println(defuse_status[DEFUSE_WIRE]);

  Serial.print("Potentiometer : ");
  Serial.println(defuse_status[DEFUSE_PTM]);
  Serial.println("");

  return defuse_status[DEFUSE_CODE] &&
    defuse_status[DEFUSE_KEY] &&
    defuse_status[DEFUSE_WIRE] &&
    defuse_status[DEFUSE_PTM];
}

void blinkLed() {
  if ( led_status == LOW ) {
    led_status = HIGH;
  } else {
    led_status = LOW;
  }
  digitalWrite(LED_PIN, led_status);
}

void loop() {
  if ( end_of_game ) {
    return;
  }

  currentMillis = millis();

  if ( (currentMillis - previousMillis >= interval ) == true ) {
    // led blink
    blinkLed();

    counterMillis -= currentMillis - previousMillis;
    Serial.print("Counter (s) : ");
    Serial.println(counterMillis/1000/60);

    if (checkDefuse()) {
      // OLED display success game
      Serial.println("End of game : Success !");
      playSound(melodieSuccess, dureeNoteSuccess, melodieSizeSuccess);
      end_of_game = true;
      return;
    }

    if (counterMillis <= 0) {
      // OLED display failed game
      Serial.println("End of game : Failed !");
      playSound(melodieFailed, dureeNoteFailed, melodieSizeFailed);
      end_of_game = true;
      return;
    }

    // OLED display current

    previousMillis = currentMillis;
  }

  // Keypad Code
  if ( ! defuse_status[DEFUSE_CODE]) {
    defuse_status[DEFUSE_CODE] = checkDefuseCode();
  }

  // Key
  if ( ! defuse_status[DEFUSE_KEY]) {
    defuse_status[DEFUSE_KEY] = checkDefuseKey();
  }

  // Wire
  if ( ! defuse_status[DEFUSE_WIRE]) {
    defuse_status[DEFUSE_WIRE] = checkDefuseWire();
  }

  // Potentiometer
  if ( ! defuse_status[DEFUSE_PTM]) {
    defuse_status[DEFUSE_PTM] = checkDefusePTM();
  }

}