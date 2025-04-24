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
float lampo, kosteus;

// Tilastot for button 3
float minAnalog = 1000.0;
float maxAnalog = -1000.0;
float minFreq = 1000.0;
float maxFreq = -1000.0;
bool showMin = true;



// ISR services
void countPulse() {
  pulseCount++;
}

void timerIsr() {
  frequency = pulseCount;
  pulseCount = 0;
  freqSignals[freqIndex] = frequency;
  freqIndex++;
  if (freqIndex >= SEKUNTI_COUNT) {
    freqIndex = 0;
    freqReady = true;
  }
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
    sprintf(msgBuf, "IOTJS={\"S_name1\":\"ENERGY_lampo\",\"S_value\":%s}", outStr1);
    sprintf(msgBuf, "IOTJS={\"S_name2\":\"ENERGY_kosteus\",\"S_value\":%s }", outStr2);

    // sprintf(msgBuf, "IOTJS={\"S_name\":\"ENERGY_lampo\",\"S_value\":%s,"
    //   "\"S_name\":\"ENERGY_kosteus\",\"S_value\":%s}",
    //   outStr1, outStr2);
    client.publish(outTopic, msgBuf); // Lähetetään viesti MQTT-brokerille
    Serial.println("Message sent to MQTT server."); Serial.println(msgBuf);// Tulostetaan viesti onnistuneesta lähettämisestä
  } else {
    Serial.println("Failed to send message: not connected to MQTT server.");
  }
}

void connect_MQTT_server() {
    Serial.println("Connecting to MQTT broker..."); // Tulostetaan vähän info-viestiä
      if (client.connect(clientId)) { // Tarkistetaan saadaanko yhteys MQTT-brokeriin
          Serial.println("Connected OK!"); // Yhdistetty onnistuneesti
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
  // float humid = 100 - ((frequency  - 6.9) / (7.9 - 6.9) * 60); // Linear interpolation
  // return ;

  float humid = (0 + (-62.2222 * (frequency/ 1000.0)) + 530.4444); // Käyrän sovittaminen pisteistöön
  if(humid > 105.0) {
    humid = 20;
  }
  return humid;
}

// UI funktiot
// Nappi 1: näytä IP-osoitetta and MQTT-tilaa
void showNetwork() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("IP: "); lcd.print(Ethernet.localIP());
  lcd.setCursor(0, 1);
  lcd.print("MQTT: "); 
  lcd.print((client.connect(clientId)) ? "Connected OOK!" : "ERROR happened");
}

// Nappi 2: näytä lämpön ja kosteuksen keskiarvoja
void showSignals(float avgTemp, float avgHumid) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("AvgT: "); lcd.print(avgTemp, 1); lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("AvgH: "); lcd.print(avgHumid, 1); lcd.print("%");
}

// Nappi 3: vaihda min/max lämpö & kosteus
void showStats() {
  lcd.clear();
  if (showMin) {
    lcd.setCursor(0, 0);
    lcd.print("Min lampo: "); lcd.print(minAnalog, 1);
    lcd.setCursor(0, 1);
    lcd.print("Min kosteus: "); lcd.print(minFreq, 1);
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Max lampo: "); lcd.print(maxAnalog, 1);
    lcd.setCursor(0, 1);
    lcd.print("Max kosteus: "); lcd.print(maxFreq, 1);
  }
  showMin = !showMin;
}

// Nappi 4: Tulosta nykyista lämpön / kosteuksen dataa
void customAction(float lampo, float kosteus) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Lampo: ");
  lcd.print(lampo, 1);
  lcd.print("C");
  lcd.setCursor(0, 1);
  lcd.print("Kosteus: ");
  lcd.print(kosteus, 1);
  lcd.print("%");
}

void handleKey(char key) {
  switch (key) {
    case '1': showNetwork(); break;
    case '2': showSignals(avgTemp, avgHumid); break;
    case '3': showStats();   break;
    case 'A': customAction(lampo, kosteus); break;
  }
}

void processLoop() {

  const int raw = analogRead(analogPin);
  const float voltage = rawToVoltage(vRef, raw);
  const float temp = voltageToTemperature(voltage);
  const float humidity = frequencyToHumidity(frequency);

  if (temp     < minAnalog) minAnalog = temp;
  if (temp     > maxAnalog) maxAnalog = temp;
  if (humidity > maxFreq  ) maxFreq   = humidity;
  if (humidity < minFreq  ) minFreq   = humidity;
  
  // Signaalin lukeminen
  analogSignals[analogIndex] = raw;
  analogIndex ++;
  if (analogIndex >= SEKUNTI_COUNT) {
    analogIndex = 0;
    analogReady = true;
  }

  lampo = temp;
  kosteus = humidity;

 float analogSum = 0, freqSum = 0;
  for (int i = 0; i < SEKUNTI_COUNT; i++) {
    analogSum += analogSignals[i];
    freqSum   += freqSignals[i];
  }
  float analogAvg  = analogSum / SEKUNTI_COUNT;
  float freqAvg    = freqSum   / SEKUNTI_COUNT;
  float voltageAvg = rawToVoltage(vRef, analogAvg);

  avgTemp  = voltageToTemperature(voltageAvg);
  avgHumid = frequencyToHumidity(freqAvg);  


   // Serial Monitor -
   Serial.println("-----------------------------");
   Serial.print("ADC: "); Serial.println(raw);
   Serial.print("Voltage: "); Serial.print(voltage); Serial.println(" V");
   Serial.print("Temp: "); Serial.print(temp); Serial.println(" C degrees");
   Serial.print("Frequency: "); Serial.print(frequency); 
   Serial.print(" Hz, Humidity: ");Serial.print(humidity); Serial.println("%");
   Serial.println("-----------------------------");

   
   // Keypad handling
   char key = keypad.getKey();
   if (key) {
   Serial.print("Key pressed: ");
   Serial.println(key);
   handleKey(key);
   }


  // 10 sekunnin jälkeen 
   if(analogReady && freqReady) {
      send_MQTT_message(avgTemp, avgHumid);

      Serial.println("=== AVERAGE OVER 10s ===");
      Serial.print("AnalogAvg: "); Serial.println(analogAvg);
      Serial.print("VolAvg: "); Serial.println(voltageAvg);
      Serial.print("TempAvg: "); Serial.println(avgTemp);
      Serial.print("FreqAvg: "); Serial.print(freqAvg); Serial.println(" Hz");
      Serial.print("HumidAvg: "); Serial.print(avgHumid); Serial.println(" %");
      Serial.println("========================");

      // RESET flags
      analogReady = freqReady =false; 

  }
  delay(1000); 
  
}

// Setup
void setup() {
  Serial.begin(9600);


  // LCD-tervetelu
  lcd.begin(20, 4);
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
  // Pidä MQTT-yhteys elossa
  client.loop();
  // Suorita koko päälogiikka omassa aliohjelmassaan
  processLoop();
}
