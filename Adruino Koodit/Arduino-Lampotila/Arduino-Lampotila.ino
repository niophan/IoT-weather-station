float vRef = 5.0;
const int analogPin = A0;

void setup() {
  Serial.begin(9600);

}

void loop() {
  int adcValue = analogRead(analogPin);
  float voltage = (adcValue / 1023.0)*vRef;


  Serial.println("ADC value: ");
  Serial.println(adcValue);
  Serial.println(", Voltage value: ");
  Serial.println(voltage);
  

  delay(1000);

}
