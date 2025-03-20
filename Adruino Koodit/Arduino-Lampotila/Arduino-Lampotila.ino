#include <LiquidCrystal.h>

const float vRef = 5.0;
const float analogPin = A0;


// int d7 = 2, d6 = 3, d5 = 4, d4 = 5, enable = 11, rs = 12;
// LiquidCrystal lcd(rs, enable, d4, d5, d6, d7);

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);
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


void setup() {
  // Serial.begin(9600);
  lcd.createChar(3, heart);
  lcd.begin(19, 4);
  // lcd.write(byte(0));
  lcd.print(" Me \x03 sulari ");
  lcd.setCursor(1, 1);
  lcd.print(" Team ENER-gy ");
  lcd.setCursor(2, 2);
  lcd.print(" Eino, Nio, Eemeli ");
  delay(2500);
  lcd.clear();

}

void loop() {
   int x = 0, y = 0;
   const float raw = analogRead(analogPin);
   const float voltage = rawToVoltage(vRef, raw);
   const float temp = voltageToTampere(voltage);
   
   // Tulosta säädata
   lcd.print("ADC value: " + String(raw));
   delay(2500);
   lcd.clear();

   lcd.setCursor(0, 0);
   lcd.print("Voltage value: " + String(voltage));
   lcd.setCursor(0, 1);
   lcd.print(" voltages.");
   delay(2500);
   lcd.clear();

   lcd.setCursor(0, 0);
   lcd.print("Temp value: " + String(temp));
   lcd.setCursor(0, 1);
   lcd.print(" celcius degrees.");
   delay(2500);
   lcd.clear();
   
   // Tulosta A->Z 
    while(vRef) {
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
    }
    

  // Serial.println("-----------------------------");
  // Serial.println("ADC value: ");
  // Serial.println(raw);
  // Serial.println("Voltage value: ");
  // Serial.println(String(voltage) + " voltages");
  // Serial.println("Temperature value: ");
  // Serial.println(String(temp) + " Celcius degrees");
  

  delay(3000);

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
