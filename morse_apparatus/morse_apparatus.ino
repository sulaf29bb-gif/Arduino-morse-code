/*
  Morseapparat (Arduino Morse code device)

  - One button: a short press is a dot, a longer press is a dash.
  - Morse input is translated to text and shown on an I2C LCD (and the Serial Monitor).
  - Text typed in the Serial Monitor is played back as Morse on a buzzer + LED.

  Button press lengths (ms):
    <= 400        dot
    401  - 2000   dash
    2001 - 5000   end of letter (translate the dots/dashes entered so far)
    > 5000        translate the pending letter and show the whole word
*/

#include <LiquidCrystal_I2C.h>
#include <Wire.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int buttonPin1 = 2;   // interrupt on RISING  (button pressed)
const int buttonPin2 = 3;   // interrupt on FALLING (button released)
const int buzzPin = 8;
const int ledPin = 11;

unsigned long pressed = 0;
unsigned long released = 0;
int DebounceTime = 80;
unsigned long lastInterrupt = 0;
unsigned int HoldInTime = 0;

String morse[] = {
  ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....", "..",   // A-I
  ".---", "-.-", ".-..", "--", "-.", "---", ".--.", "--.-", ".-.", "...", "-",
  "..-", "...-", ".--", "-..-", "-.--", "--..",                    // J-Z
  ".----", "..---", "...--", "....-", ".....", "-....", "--...", "---..", "----.",
  "-----"                                                          // 1-9, then 0
};

String alfabet[] = {
  "A", "B", "C", "D", "E", "F", "G", "H",
  "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W",
  "X", "Y",
  "Z", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0"
};

String bokstav = "";  // dots/dashes of the letter being entered (one letter at a time)
String ord = "";      // letters translated so far; not shown until the whole word is done

void push() {
  if (millis() > lastInterrupt + DebounceTime) {
    pressed = millis();
    lastInterrupt = millis();
  }
}

void realese() {
  if (millis() > lastInterrupt + DebounceTime) {
    released = millis();
    lastInterrupt = millis();
  }
}

void setup() {
  pinMode(buttonPin1, INPUT);
  pinMode(buttonPin2, INPUT);
  pinMode(buzzPin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(buttonPin1), push, RISING);
  attachInterrupt(digitalPinToInterrupt(buttonPin2), realese, FALLING);
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.clear();
}

void dot() {
  tone(buzzPin, 1000);
  digitalWrite(buzzPin, HIGH);
  digitalWrite(ledPin, HIGH);
  delay(100);
  noTone(buzzPin);
  digitalWrite(ledPin, LOW);
  Serial.print(".");
  bokstav += ".";
}

void dash() {
  tone(buzzPin, 1000);
  digitalWrite(buzzPin, HIGH);
  digitalWrite(ledPin, HIGH);
  delay(500);
  noTone(buzzPin);
  digitalWrite(ledPin, LOW);
  Serial.print("-");
  bokstav += "-";
}

// Translate the entered dots/dashes (bokstav) into a letter and append it to ord.
void oversetting() {
  for (int i = 0; i < 36; i++) {  // 26 letters + 10 digits
    if (morse[i] == bokstav) {
      ord += alfabet[i];
      bokstav = "";  // reset to an empty string, otherwise every word would start with 0
      lcd.setCursor(0, 0);
    }
  }
}

// Serial Monitor text (uppercase) -> Morse on the buzzer and LED.
void oversettingTilMorse() {
  Serial.println("Oversetter...");
  static String translated = "";
  String input = Serial.readString();
  for (int i = 0; i < input.length(); i++) {
    for (int j = 0; j < 36; j++) {
      if (input[i] == alfabet[j][0]) {
        translated = translated + morse[j] + ' ';
      }
    }
  }
  for (int i = 0; i < translated.length(); i++) {
    if (translated[i] == '.') {
      dot();
    } else if (translated[i] == '-') {
      dash();
    } else if (translated[i] == ' ') {
      Serial.write(' ');
      delay(250);
    }
    delay(250);
  }
}

// Translate the pending letter, then show the whole word on the LCD and Serial Monitor.
void printing() {
  for (int i = 0; i < 36; i++) {
    if (morse[i] == bokstav) {
      ord += alfabet[i];
      bokstav = "";
      Serial.print(ord);
      Serial.println(" ");
      lcd.clear();
      lcd.print(ord);  // print the whole word, not one letter at a time
      ord = "";
    }
  }
}

void loop() {
  if (Serial.available()) {
    oversettingTilMorse();
  }

  if (released > 0) {
    HoldInTime = released - pressed;
    released = 0;
    pressed = 0;

    if (HoldInTime <= 400) {
      dot();
    } else if (HoldInTime > 400 && HoldInTime <= 2000) {
      dash();
    } else if (HoldInTime > 2000 && HoldInTime <= 5000) {
      oversetting();  // store the letter just entered
      Serial.print(" ");
    } else if (HoldInTime > 5000) {
      printing();  // show the word
    }
  }
}
