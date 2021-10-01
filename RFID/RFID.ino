

/*
   --------------------------------------------------------------------------------------------------------------------
   Example sketch/program showing how to read data from a PICC to serial.
   --------------------------------------------------------------------------------------------------------------------
   This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid

   Example sketch/program showing how to read data from a PICC (that is: a RFID Tag or Card) using a MFRC522 based RFID
   Reader on the Arduino SPI interface.

   When the Arduino and the MFRC522 module are connected (see the pin layout below), load this sketch into Arduino IDE
   then verify/compile and upload it. To see the output: use Tools, Serial Monitor of the IDE (hit Ctrl+Shft+M). When
   you present a PICC (that is: a RFID Tag or Card) at reading distance of the MFRC522 Reader/PCD, the serial output
   will show the ID/UID, type and any data blocks it can read. Note: you may see "Timeout in communication" messages
   when removing the PICC from reading distance too early.

   If your reader supports it, this sketch/program will read all the PICCs presented (that is: multiple tag reading).
   So if you stack two or more PICCs on top of each other and present them to the reader, it will first output all
   details of the first and then the next PICC. Note that this may take some time as all data blocks are dumped, so
   keep the PICCs at reading distance until complete.

   @license Released into the public domain.

   Typical pin layout used:
   -----------------------------------------------------------------------------------------
               MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
               Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
   Signal      Pin          Pin           Pin       Pin        Pin              Pin
   -----------------------------------------------------------------------------------------
   RST/Reset   RST          9             10         D9         RESET/ICSP-5     RST
   SPI SS      SDA(SS)      10            53        D10        10               10
   SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
   SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
   SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15


     The circuit:
   LCD RS pin to digital pin 12
   LCD Enable pin to digital pin 11
   LCD D4 pin to digital pin 5
   LCD D5 pin to digital pin 4
   LCD D6 pin to digital pin 3
   LCD D7 pin to digital pin 2
   LCD R/W pin to ground
   LCD VSS pin to ground
   LCD VCC pin to 5V
   10K resistor:
   ends to +5V and ground (+ve to LCD A(with 10k resistor) & -ve LCD K)
   wiper (poteniometer) to LCD VO pin (pin 3)

*/

#include <IRremote.h>
int receiver = A0;
IRrecv irrecv(receiver);     // create instance of 'irrecv'
decode_results results;      // create instance of 'decode_results'

#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal.h>
#include <Keypad.h>

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {26, 28, 30, 32}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {34, 36, 38, 40}; //connect to the column pinouts of the keypad
//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);

#define green           22
#define red             25
#define buzzer          6

#define RST_PIN         10          // Configurable, see typical pin layout above
#define SS_PIN          53         // Configurable, see typical pin layout above

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance

int wrongCounter = 0;
String approvedConfirmation;
String epass;
char hashtag;

void setup() {
  pinMode(green, OUTPUT);
  pinMode(red, OUTPUT);
  pinMode(buzzer, OUTPUT);

  lcd.begin(16, 2);
  ClearPrint("Approach card or", "enter pin");

  Serial.println("IR Receiver Button Decode");
  irrecv.enableIRIn(); // Start the receive

  Serial.begin(9600);		// Initialize serial communications with the PC
  while (!Serial);		// Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();			// Init SPI bus
  mfrc522.PCD_Init();		// Init MFRC522
  delay(4);				// Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();	// Show details of PCD - MFRC522 Card Reader details

}

void loop() {
    if (irrecv.decode(&results)) // have we received an IR signal?
  {
    translateIR();
    irrecv.resume(); // receive the next value
  }
  //resets approved Confirmation Value from the previus run else it will stay approved even if the card is wrong
  approvedConfirmation = "";

  //bans user after 3 unsuccessful tries; only RFID cards specified below can unban
  if (wrongCounter > 2) {
    ClearPrint("Banned", "");
    while (wrongCounter > 2) {
      if ( ! mfrc522.PICC_IsNewCardPresent()) {
        return;
      }
      if ( ! mfrc522.PICC_ReadCardSerial()) {
        return;
      }
      //RFID cards that can unban
      VerifyUFIDCard(" A0 3B 99 2B", "Christian");
    }
  }

  //unlock using pin
  KeypadProcess();
  //intiated if user entered pinn and pressed confirmation key '#'
  if (hashtag == '#' && epass != "") {
    //add approved pin codes here: format VerifyPinCode("correct pin code","holder of this pin code");
    VerifyPinCode("1234", "Somebody");
    VerifyPinCode("2002", "Person");
    //takes consequences if entered pin is wrong
    if (approvedConfirmation != "approved") {
      epass = "";
      declined();
    }
  }

  //unlock using RFID
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  //add approved RFID cards here: format VerifyUFIDCard("space followed by card's UFID code","holder of this UFID card");
  VerifyUFIDCard(" A0 3B 99 2B", "Christian");
  //VerifyUFIDCard(" B9 7C AE 00", "Victor");
  if (approvedConfirmation != "approved") {
    declined();
  }
}

void approved(String Name) {
  digitalWrite(green, HIGH);
  analogWrite(buzzer, 255);
  approvedConfirmation = "approved";
  epass = "";
  wrongCounter = 0;
  lcd.clear();
  lcd.print("ACCESS GRANTED");
  lcd.setCursor(0, 1);
  lcd.print(Name);
  delay(300);
  digitalWrite(green, LOW);
  analogWrite(buzzer, 0);
  delay(1500);
  ClearPrint("Approach card or", "enter pin");
}
void declined() {
  approvedConfirmation = "declined";
  epass = "";
  lcd.clear();
  lcd.print("ACCESS DENIED");
  xBeeps(10, 150, 255);
  wrongCounter = wrongCounter + 1;
  ClearPrint("Approach card or", "enter pin");
}
void ClearPrint(String text, String textTwo) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(text);
  lcd.setCursor(0, 1);
  lcd.print(textTwo);
}

void VerifyUFIDCard(String correctUCode, String holderName) {
  Serial.print("UID tag :");
  String content = " ";
  byte letter;
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    //Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    //Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }
  Serial.println();
  //Serial.print("Message : ");
  content.toUpperCase();
  Serial.println(content.substring(1));
  if (content.substring(1) == correctUCode) {
    approved(holderName);
  }
}

void KeypadProcess() {
  char customKey = customKeypad.getKey();
  String star;
  if (customKey) {
    Serial.println(customKey);
    if (customKey != '*') {
      xBeeps(3, 50, 200);
    }
  }
  if (customKey == '*') {
    Serial.println("code input");
    beep();
    lcd.clear();
    lcd.print("pin: ");
    while (1 > 0) {
      char customKey = customKeypad.getKey();
      if (customKey == '#') {
        hashtag = customKey;
        break;
      }
      if (customKey == 'D') {
        Serial.println("Clear");
        ClearPrint("Pin Cleared", "");
        epass = "";
        beep();
        delay(1500);
        break;
      }
      if (customKey) {
        epass = epass + customKey;
        Serial.println(epass);
        lcd.clear();
        lcd.print("pin: ");
        star = star + "*";
        lcd.print(star);
      }
    }
    ClearPrint("Approach card or", "enter pin");
  }

}

void VerifyPinCode(String correctPin, String holderName) {
  if (epass == correctPin) {
    approved(holderName);
  }
}

//beeps x times and light turns red with specific delay time with buzzer Power
void xBeeps(int numBeeps, int delayTime, int bPower) {
  for (int i = 0; i < numBeeps; i++) {
    digitalWrite(red, HIGH);
    analogWrite(buzzer, bPower);
    delay(delayTime);
    analogWrite(buzzer, 0);
    digitalWrite(red, LOW);
    delay(delayTime);
  }
}

//beeps once
void beep() {
  analogWrite(buzzer, 200);
  delay(50);
  analogWrite(buzzer, 0);
}

void translateIR() // takes action based on IR code received
// describing Remote IR codes
{

  switch (results.value) {
    case 0xFFA25D: approved(""); break;
    case 0xFFE21D: approved(""); break;
    case 0xFF629D: approved(""); break;
    case 0xFF22DD: approved("");    break;
    case 0xFF02FD: approved("");    break;
    case 0xFFC23D: approved("");   break;
    case 0xFFE01F: approved("");    break;
    case 0xFFA857: approved("");    break;
    case 0xFF906F: approved("");    break;
    case 0xFF9867: approved("");    break;
    case 0xFFB04F: approved("");    break;
    case 0xFF6897: approved("");    break;
    case 0xFF30CF: approved("");    break;
    case 0xFF18E7: approved("");    break;
    case 0xFF7A85: approved("");    break;
    case 0xFF10EF: approved("");    break;
    case 0xFF38C7: approved("");    break;
    case 0xFF5AA5: approved("");    break;
    case 0xFF42BD: approved("");    break;
    case 0xFF4AB5: approved("");    break;
    case 0xFF52AD: approved("");    break;
    case 0xFFFFFFFF: approved(""); break;

    default:
      Serial.println(" other button   ");

  }// End Case

  delay(500); // Do not get immediate repeat
}
int buttonState = 0;
