#include <LiquidCrystal.h>

const float vRef = 5.0;
const int analogPin = A0;
const int signalPin = 2;
volatile unsigned long pulseCount = 0;

// Opettajan-avun koodit
// int d7 = 2, d6 = 3, d5 = 4, d4 = 5, enable = 11, rs = 12;
// LiquidCrystal lcd(rs, enable, d4, d5, d6, d7);

// byte a_umlaut[8] = { B01110, B00000, B01110, B10001, B11111, B10001, B10001, B00000 }; // ä
// byte o_umlaut[8] = { B01110, B00000, B01110, B10001, B10001, B10001, B01110, B00000 }; // ö
// byte a_ring[8]    = { B00100, B01010, B00100, B01110, B10001, B11111, B10001, B00000 }; // å

LiquidCrystal lcd(9, 8, 7, 6, 5, 4);
byte heart[8] = {
  0B00000,
  0B01010,
  0B11111,
  0B11111,
  0B11111,
  0B01110,
  0B00100,
  0B00000,
};

void countPulse() {
  pulseCount++;
}



void setup() {
  Serial.begin(9600);
  pinMode(signalPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(signalPin), countPulse, RISING);
  lcd.createChar(3, heart);
  // lcd.createChar(0, a_umlaut);
  // lcd.createChar(1, o_umlaut);
  // lcd.createChar(2, a_ring);


  lcd.begin(20, 4);
  // lcd.write(byte(0));
  lcd.print(" Me \x03 sulari ");
  lcd.setCursor(1, 1);
  lcd.print(" Team ENER-gy ");
  lcd.setCursor(2, 2);
  lcd.print(" Eino, Nio, Eemeli ");
  delay(2500);
  lcd.clear();

  lcd.print("Humidity Sensor");
  delay(2000);
  lcd.clear();
}

void loop() {
   int x = 0, y = 0;
   const float raw = analogRead(analogPin);
   const float voltage = rawToVoltage(vRef, raw);
   const float temp = voltageToTampere(voltage);
   const float frequency = readFrequency();
   const float humidity = frequencyToHumidity(frequency);
   
   // Serial Monitor -
   Serial.println("-----------------------------");
   Serial.print("ADC: ");
   Serial.println(raw);
   Serial.print("Voltage: ");
   Serial.print(voltage);
   Serial.println(" V");
   Serial.print("Temp: ");
   Serial.print(temp);
   Serial.println(" C degrees");
   Serial.print("Frequency: ");
   Serial.print(frequency);
   Serial.print(" Hz, Humidity: ");
   Serial.print(humidity);
   Serial.println("%");
   Serial.println("-----------------------------");

   // LDC
   lcd.setCursor(0, 0);
   lcd.print("ADC:" + String(raw));
   lcd.setCursor(0, 1);
   lcd.print("Voltage:" + String(voltage) + " V");
   lcd.setCursor(0, 2);
   lcd.print("Temp:" + String(temp) + " C degree");
   
   delay(3500);
   lcd.clear();

   lcd.setCursor(0, 0);
   lcd.print("Freq: " + String(frequency) + " Hz");
   lcd.setCursor(0, 1);
   lcd.print("Humidity: " + String(humidity) + "%");

   delay(1500); 
   lcd.clear();
   
   // Tulosta A->Z 
   /* while(true) {
      for (int i = 65; i <= 90; i++) { 
        lcd.setCursor(x, y);
        lcd.write(i); 
        delay(300); 
        lcd.clear(); 
        
        // Liikutetaan kursoria ympäri näyttöä
        if (x < 19 && y == 0) x++;           // Oikealle ylhäällä
        else if (x == 19 && y < 1) y++;      // Alas oikealla
        else if (y == 1 && x > 0) x--;       // Vasemmalle alhaalla
        else if (x == 0 && y > 0) y--;       // Ylös vasemmalla
      }

      // Print custom characters Ä, Å, Ö
      byte extraChars[] = { 0, 2, 1 }; 
      for (int j = 0; j < 3; j++) {
        lcd.setCursor(x, y);
        lcd.write(extraChars[j]);
        delay(300);
        lcd.clear();

        if (x < 19 && y == 0) x++;
        else if (x == 19 && y < 1) y++;
        else if (y == 1 && x > 0) x--;
        else if (x == 0 && y > 0) y--;
    } */
}

float readFrequency() {
  pulseCount = 0;
  unsigned long startTime = millis();
  while (millis() - startTime < 1000) {
  }
  return pulseCount;
}

float frequencyToHumidity(float frequency) {
  frequency /= 1000.0;
  // float humidity = 100 - ((frequency - 6.9) / (7.9 - 6.9) * 60); // Linear interpolation
  // return humidity;
  return 0*frequency*frequency -62.2222*frequency + 530.4444; // Nonlinear (Käyräsovitus)
}

float rawToVoltage(float vRef, float raw) {
  return (raw / 1023.0)*vRef;
}

float voltageToTampere(float voltage) {
  if(voltage < 1.6) {
    return -10;
  } else if (voltage <= 2.6) {
    return 0;
  } else if (voltage <= 3.70) {
    return 10;
  } else if (voltage <= 4.6) {
    return 20;
  } else {
    return 25;
  }    
    
}
