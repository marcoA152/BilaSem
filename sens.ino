
/////////EchoDist/////////
void EchoDist() {
  digitalWrite(TRIGPIN, LOW); // Set the trigger pin to low for 2uS
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH); // Send a 10uS high to trigger ranging
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW); // Send pin low again
  double Dist1 = pulseIn(ECHOPIN, HIGH);
  Dist1 = Dist1  * 0.034 / 2;
  delay(50);// Wait 50mS before next ranging

  if (Dist1 == 0) {
    EchoDist();
  }
  else {
    Dist = Dist1;
  }
}

/////////phMeter/////////
void phMeter()
{
  const int numReadings = 40;       // smooth the result
  float total = 0;                  // the running total
  float average = 0;                // the average

  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    total += analogRead(ph_Pin);
    delay(20);
  }
  average = (total / numReadings);
  voltage = average * 5.0 / 1024;
  pHValue = 3.5 * voltage + Offset;
}


/////////TDSSens/////////
void TDSSens() {
  gravityTds.setTemperature(Temperature);  // set the temperature and execute temperature compensation
  gravityTds.update();  //sample and calculate
  tdsValue = gravityTds.getTdsValue();  // then get the value
}



/////////TempSens/////////
void TempSens() {

  sensors.requestTemperatures();
  Temperature = sensors.getTempCByIndex(0);
}



/////////TurbSens/////////
void TurbSens() {
  int turbValue = analogRead(A0);// read the input on analog pin 0:
  TurbVolt = turbValue * (5.0 / 1024.0); // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
}


void Intvalmtr() {

  Serial.print("Dist: ");
  Serial.print(Dist);
  Serial.print("   cm");

  Serial.print(", TurbV: ");
  Serial.print(TurbVolt);
  Serial.print("   V");

  Serial.print(", Temp: ");
  Serial.print(Temperature);
  Serial.print("   C");

  Serial.print(", Temp_E: ");
  Serial.print(Temp_E);
  Serial.print("   C");

  Serial.print(", pH: ");
  Serial.print(pHValue);

  Serial.print(", TDS: ");
  Serial.print(tdsValue);
  Serial.print(" ppm");

  Serial.print(", batt: ");
  Serial.print(Volt);
  Serial.println(" mV");


  delay(1000);
}
