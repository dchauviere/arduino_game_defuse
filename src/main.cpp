#include <Arduino.h>
#include <GetKeypad.h>

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "indices.h"

// CONFIG
#define KEYCODE "572C"
#define PTM_MIN 374 // value = 394
#define PTM_MAX 414
#define START_COUNTER_MILLIS 60*60*1000

int melodieSuccess[] = {330,392,659,523,587,784};
int dureeNoteSuccess[] = {150,150,150,150,150,150 };
int melodieSizeSuccess = 6;
int melodieFailed[] = {622,587,554};
int dureeNoteFailed[] = {300,300,300 };
int melodieSizeFailed = 3;
int melodieError[] = {100};
int dureeNoteError[] = {300};
int melodieSizeError = 1;
int melodieOk[] = {600};
int dureeNoteOk[] = {150 };
int melodieSizeOk = 1;


#define KEY_PIN 13
#define WIRE1_PIN 46 // Marron
#define WIRE2_PIN 47 // Orange
#define WIRE3_PIN 48 // Vert
#define WIRE4_PIN 45 // Bleu
#define LED_PIN 3
#define BUZZER_PIN 2
#define BUTTON_PIN 12

int wires_pin[4]={WIRE1_PIN,WIRE2_PIN,WIRE3_PIN,WIRE4_PIN};
bool badwire[4]={true,true,true,false};

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
  Serial.println("Setup start !");

  SPI.begin(); // Init SPI bus

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

  indices_init();
  
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(KEY_PIN, INPUT);
  pinMode(WIRE1_PIN, INPUT);
  pinMode(WIRE2_PIN, INPUT);
  pinMode(WIRE3_PIN, INPUT);
  pinMode(WIRE4_PIN, INPUT);

  defuse_status[DEFUSE_CODE] = false;
  defuse_status[DEFUSE_KEY] = false;
  defuse_status[DEFUSE_WIRE] = false;
  defuse_status[DEFUSE_PTM] = false;


  Serial.println("Setup done !");

}

void playSound(int *melodie, int *dureeNote, int size) {
  int i;
  for (i = 0; i < size; i++){
    tone(BUZZER_PIN, melodie[i]);
    delay(dureeNote[i]);
    noTone(BUZZER_PIN);
  }
}

bool checkDefuseCode() {
  char *keyCodeValue;
  // Keypad
  if ( getKeypad() ) {
    keyCodeValue = getKeypadData();
    Serial.print("Code entered : ");
    Serial.println(keyCodeValue);
    if(strcmp(keyCodeValue, KEYCODE) == 0) {
      playSound(melodieOk, dureeNoteOk, melodieSizeOk);
      return true;
    }else{
      playSound(melodieError, dureeNoteError, melodieSizeError);
      Serial.println("decrement counter");
      counterMillis -= (long) 60*5000;
    }
  }
  return false;
}

bool checkDefuseKey() {
  if ( digitalRead(KEY_PIN) == 1 ) {
    playSound(melodieOk, dureeNoteOk, melodieSizeOk);
    return true;
  }
  return false;
}

bool checkDefuseWire() {
  for (int w=0; w<4; w++) {
    int data = digitalRead(wires_pin[w]);
    if (data==0 && w==3) {
      playSound(melodieOk, dureeNoteOk, melodieSizeOk);
      return true;
    }
    if (data==0 && badwire[w]) {
      badwire[w]=false;
      playSound(melodieError, dureeNoteError, melodieSizeError);
      Serial.println("decrement counter");
      counterMillis -= (long) 60*5000;
    }
  }
  return false;  
}

bool checkDefusePTM() {
  int ptm_value = analogRead(A0);

  if (digitalRead(BUTTON_PIN)==0) return false; 

  Serial.print("Potentiometer : ");
  Serial.println(ptm_value);
  
  if ( ptm_value >= PTM_MIN && ptm_value <= PTM_MAX ) {
    playSound(melodieOk, dureeNoteOk, melodieSizeOk);
    return true;
  } else {
    playSound(melodieError, dureeNoteError, melodieSizeError);
    Serial.println("decrement counter");
    counterMillis -= (long) 60*5000;
  }
  return false;
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
  int status_line_x;
  int status_line_y;

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
      digitalWrite(LED_PIN, LOW);
      playSound(melodieSuccess, dureeNoteSuccess, melodieSizeSuccess);
      end_of_game = true;
      display.clearDisplay();
      display.setTextSize(3);
      display.setCursor(10,10);
      display.print("Gagne");
      display.display();
      return;
    }

    if (counterMillis <= 0) {
      // OLED display failed game
      Serial.println("End of game : Failed !");
      digitalWrite(LED_PIN, HIGH);
      playSound(melodieFailed, dureeNoteFailed, melodieSizeFailed);
      end_of_game = true;
      display.clearDisplay();
      display.setTextSize(3);
      display.setCursor(10,10);
      display.print("Perdu");
      display.display();
      return;
    }

    // OLED display current
    display.clearDisplay();
    display.setTextSize(6);
    display.setTextColor(WHITE);
    display.setCursor(10,10);
    display.print(counterMillis/1000/60);
    for(int i=0; i<4; i++) {
      if (i<2) {
        status_line_y = 20;
        status_line_x = i*20+100;
      } else {
        status_line_y = 40;
        status_line_x = (i-2)*20+100;
      }
      if ( defuse_status[i] ) {
        display.drawCircle(status_line_x, status_line_y, 8, WHITE);
      } else {
        display.fillCircle(status_line_x, status_line_y, 8, WHITE);
      }
    }
    display.display();

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

  indices();
}