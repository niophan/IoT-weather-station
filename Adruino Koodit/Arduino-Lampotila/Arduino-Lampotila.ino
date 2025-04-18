// ---- Libraries and Configuration ----
#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <LiquidCrystal.h>
#include <TimerOne.h>
#include <Keypad.h>

#define outTopic "ICT4_out_2020"
#define MAC_6   0x30

// MQTT settings
byte server[]       = {10, 6, 0, 23};    // MQTT broker IP address
unsigned int Port   = 1883;               // MQTT broker port
static uint8_t mymac[6] = {0xA8,0x61,0x0A,0xAE,0x46,MAC_6}; // Ethernet MAC address
char* clientId      = "a731fsd4";       // MQTT client identifier

EthernetClient ethClient;
PubSubClient client(server, Port, ethClient);

// LCD setup (parallel mode)
// rs=9, enable=8, d4=7, d5=6, d6=5, d7=4
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

// Keypad 4x4 on pins D2..D9
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {19};  // not used by LCD here, but D2-D9 free
byte colPins[COLS] = {15, 16, 17, 18};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Analogi- ja taajuussignaalit
const float vRef = 5.0;
const int analogPin = A0;
const int signalPin = 2;
const int SEKUNTI_COUNT = 10;

volatile unsigned long pulseCount = 0;
float analogSignals[SEKUNTI_COUNT];
float freqSignals[SEKUNTI_COUNT];
int analogIndex = 0, freqIndex = 0;
bool analogReady = false, freqReady = false;
float frequency = 0;
float avgTemp = 0;
float avgHumid = 0;
float lampo = 0;
float kosteus = 0;

// Tilastot for button 3
float minAnalog = 0, maxAnalog = 0;
unsigned long minFreq = 0, maxFreq = 0;
bool showMin = true;

// ISR services
void countPulse() {
  pulseCount++;
}

void timerIsr() {
  frequency = pulseCount;
  freqSignals[freqIndex] = frequency;
  freqIndex++;
  if (freqIndex >= SEKUNTI_COUNT) {
    freqIndex = 0;
    freqReady = true;
  }
  pulseCount = 0;
}

// MQTT ja Ethernet funktiot
void fetch_IP() {
  bool connectionSuccess = Ethernet.begin(mymac);
  if (!connectionSuccess) {
    Serial.println("Failed to access Ethernet controller");
  } else {
    Serial.println("Connected with IP: " + Ethernet.localIP()); // Onnistuessa tulostetaan IP-osoite
  }
}

void send_MQTT_message(float tempAvg, float humidAvg) {
  if (!client.connected()) connect_MQTT_server();
  if (client.connected()) {
    char msgBuf[128], outStr1[8], outStr2[8];
    // floatToString
    dtostrf(tempAvg, 4, 2, outStr1);
    dtostrf(humidAvg, 4, 2, outStr2);

    sprintf(msgBuf, "IOTJS={\"S_name\":\"ENERGY_lampo\",\"S_value\":%s,"
      "\"S_name\":\"ENERGY_ikosteus\",\"S_value\":%s}",
      outStr1, outStr2);
    client.publish(outTopic, msgBuf); // Lähetetään viesti MQTT-brokerille
    Serial.println("Message sent to MQTT server."); Serial.println(payload);// Tulostetaan viesti onnistuneesta lähettämisestä
  } else {
    Serial.println("Failed to send message: not connected to MQTT server.");
  }
}

void connect_MQTT_server() {
    Serial.println("Connecting to MQTT broker..."); // Tulostetaan vähän info-viestiä
      if (client.connect(clientId)) { // Tarkistetaan saadaanko yhteys MQTT-brokeriin
          Serial.println("Connected OK"); // Yhdistetty onnistuneesti
      } else {
          Serial.println("Connection failed."); // Yhdistäminen epäonnistui
      } 
}

// Helper-funktiot
float rawToVoltage(float vRef, int raw) {
  return (raw / 1023.0) * vRef;
}

float voltageToTemperature(float voltage) {
  if (voltage < 1.6) return -10;
  else if (voltage <= 2.6) return 0;
  else if (voltage <= 3.7) return 10;
  else if (voltage <= 4.6) return 20;
  else return 25;
}

float frequencyToHumidity(float frequency) {
  float f_khz = frequency /= 1000.0;
  // float humidity = 100 - ((frequency - 6.9) / (7.9 - 6.9) * 60); // Linear interpolation
  // return humidity;
  return 0*f_khz*f_khz -62.2222*f_khz + 530.4444; // Nonlinear (Käyräsovitus)
}

// UI funktiot
// Nappi 1: näytä IP-osoitetta and MQTT-tilaa
void showNetwork() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP: "); lcd.print(Ethernet.localIP());
  lcd.setCursor(0, 1);
  lcd.print("MQTT: "); lcd.print(client.connected() ? "Connected OK" : "ERROR happened");
}

// Nappi 2: näytä lämpön ja kosteuksen keskiarvoja
void showSignals(float tempAvg, float humidAvg) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("AvgT: "); lcd.print(tempAvg, 1); lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("AvgH: "); lcd.print(humidAvg, 1); lcd.print("%");
}

// Nappi 3: vaihda min/max lämpö & kosteus
void showStats() {
  lcd.clear();
  if (showMin) {
    lcd.setCursor(0, 0);
    lcd.print("Min lämpö: "); lcd.print(minAnalog);
    lcd.setCursor(0, 1);
    lcd.print("Min kosteus: "); lcd.print(minFreq);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Max lämpö: "); lcd.print(maxAnalog);
    lcd.setCursor(0, 1);
    lcd.print("Max kosteus: "); lcd.print(maxFreq);
  }
  showMin = !showMin;
}

// Nappi 4: Tulosta nykyista lämpön / kosteuksen dataa
void customAction(float lampo, float kosteus) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp:" + String(lampo) + " C degree");
  lcd.setCursor(0, 1);
  lcd.print("Humid: " + String(kosteus) + "%");
}

void handleKey(char key) {
  switch (key) {
    case '1': showNetwork(); break;
    case '2': showSignals(avgTemp, avgHumid); break;
    case '3': showStats();   break;
    case 'A': customAction(lampo, kosteus); break;
  }
}

// Setup
void setup() {
  Serial.begin(9600);


  // LCD-tervetelu
  lcd.begin(20, 4);
  lcd.createChar(3, heart);
  lcd.print(" Me \x03 sulari ");
  lcd.setCursor(1, 1);
  lcd.print(" Team ENER-gy ");
  lcd.setCursor(2, 2);
  lcd.print(" Eino, Nio, Eemeli ");
  delay(2000);
  lcd.clear();

  fetch_IP();
  delay(2000);

  // Signaali-ISR ja ajastin
  pinMode(signalPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(signalPin), countPulse, RISING);
  Timer1.initialize(1000000);
  Timer1.attachInterrupt(timerIsr);
}

// Main loop
void loop() {
  client.loop();
  const int raw = analogRead(analogPin);
  const float voltage = rawToVoltage(vRef, raw);
  const float temp = voltageToTemperature(voltage);
  const float humidity = frequencyToHumidity(frequency);
  
  // Signaalin lukeminen
  analogSignals[analogIndex] = raw;
  analogIndex ++;
  if (analogIndex >= SEKUNTI_COUNT) {
    analogIndex = 0;
    analogReady = true;
  }
  minAnalog = min(minAnalog, temp);
  maxAnalog = max(maxAnalog, temp);
  minFreq = min(minFreq, humidity);
  maxFreq = max(maxFreq, humidity);

   // Serial Monitor -
   Serial.println("-----------------------------");
   Serial.print("ADC: "); Serial.println(raw);
   Serial.print("Voltage: "); Serial.print(voltage); Serial.println(" V");
   Serial.print("Temp: "); Serial.print(temp); Serial.println(" C degrees");
   Serial.print("Frequency: "); Serial.print(frequency); 
   Serial.print(" Hz, Humidity: ");Serial.print(humidity); Serial.println("%");
   Serial.println("-----------------------------");
   delay(1000); 
   
  // 10 sekunnin jälkeen 
   if(analogReady && freqReady) {
    unsigned long now = millis();
      float analogSum = 0, freqSum = 0;
      for(int i = 0; i < SEKUNTI_COUNT; i++) {
        analogSum += analogSignals[i];
        freqSum += freqSignals[i];
      }

      float analogAvg = analogSum / SEKUNTI_COUNT;
      float freqAvg = freqSum / SEKUNTI_COUNT;
      float voltageAvg = rawToVoltage(vRef, analogAvg);
      float tempAvg = voltageToTemperature(voltageAvg);
      float humidAvg = frequencyToHumidity(freqAvg);

      Serial.println("=== AVERAGE OVER 10s ===");
      Serial.print("AnalogAvg: "); Serial.println(analogAvg);
      Serial.print("VolAvg: "); Serial.println(voltageAvg);
      Serial.print("TempAvg: "); Serial.println(tempAvg);
      Serial.print("FreqAvg: "); Serial.print(freqAvg); Serial.println(" Hz");
      Serial.print("HumidAvg: "); Serial.print(humidAvg); Serial.println(" %");
      Serial.println("========================");

          // store for showSignals
      avgTemp   = tempAvg;
      avgHumid  = humidAvg;
      lampo = temp;
      kosteus = humidity;

      send_MQTT_message(tempAvg, humidAvg);

      // RESET flags
      analogReady = freqReady =false; 
  }

  // Keypad handling
  char key = keypad.getKey();
  if (key) {
    Serial.print("Key pressed: ");
    Serial.println(key);
    handleKey(key);
  }

}
