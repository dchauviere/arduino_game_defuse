#include <Wire.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>

// RFID
//  Badge 1 : E9 61 37 C1
//  Badge 2 : B9 82 57 C1
//  Badge 3 : E9 B2 1B B9
//  Badge 4 : 79 13 2E C2
#define SS_PIN 43
#define RST_PIN 49
MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key; 

LiquidCrystal_I2C lcd(0x27, 20, 4);

byte badges[4][4] = {
  {0xE9, 0x61, 0x37, 0xC1},
  {0xB9, 0x82, 0x57, 0xC1},
  {0xE9, 0xB2, 0x1B, 0xB9},
  {0x79, 0x13, 0x2E, 0xC2}
};

bool indices_state[4] = {false, false, false, false};
char photos[4][2] = {
  {'A', 'B'},
  {'C', 'D'},
  {'E', 'F'},
  {'G', 'H'}
};

int localisation[4][2] = {
  {26,28},
  {32,38},
  {34,24},
  {30,36}
};

String indices_position[4] = {"R4", "W2", "M7", "W15"};

unsigned long indicesMillis   = 0;
unsigned long indicesCurrentMillis;
unsigned long indicesPreviousMillis;
unsigned long indicesInterval = 10000;


void indices_init() {
  rfid.PCD_Init(); // Init MFRC522 
  lcd.init();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}
 
byte getBadge() {
  int badge_found = 0;

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return 0;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return 0;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return 0;
  }
   
  Serial.print(F("The NUID tag is: "));
  printHex(rfid.uid.uidByte, rfid.uid.size);
  Serial.println();

  if (rfid.uid.size != 4) return 0;
  for (int badge=0; badge<4; badge++) {
      if (
        badges[badge][0] == rfid.uid.uidByte[0] &&
        badges[badge][1] == rfid.uid.uidByte[1] &&
        badges[badge][2] == rfid.uid.uidByte[2] &&
        badges[badge][3] == rfid.uid.uidByte[3]
        ) {
          badge_found = badge+1;
          Serial.print("Found badge : ");
          Serial.println(badge_found);
          break;
        }
  }

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();

  return badge_found;
}

void indices() {
  indicesCurrentMillis = millis();

  int badge = getBadge();
  
  if (badge == 0) {
    if ( (indicesCurrentMillis - indicesPreviousMillis >= indicesInterval ) == true ) {
      lcd.clear();
      lcd.noBacklight();
    }
    return;
  }

  int loc_result = 0;
  
  for (int loc=0; loc<2; loc++) {
    if (localisation[badge-1][loc] == 0) {
      loc_result+=1;
    }else{
      loc_result+= digitalRead(localisation[badge-1][loc]);
    }
  }
  if (loc_result == 2) {
    indices_state[badge-1]=true;
  }

  if (indices_state[badge-1]) {
    lcd.clear();
    lcd.backlight();
    lcd.setCursor(0,0);
	  lcd.print("l'indice se trouve");
    lcd.setCursor(0,1);
	  lcd.print("dans la case : ");
    lcd.setCursor(9,2);
	  lcd.print(indices_position[badge-1]);
  }else{
    lcd.clear();
    lcd.backlight();
    lcd.setCursor(0,0);
	  lcd.print("localiser sur la");
    lcd.setCursor(0,1);
	  lcd.print("carte les photos : ");
    lcd.setCursor(0,2);
	  lcd.print(photos[badge-1][0]);
    lcd.setCursor(2,2);
	  lcd.print(photos[badge-1][1]);
  }
  indicesPreviousMillis=indicesCurrentMillis;
}