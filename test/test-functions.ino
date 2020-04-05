void setStubData(){
  day_val = 3;
  mnth = 4;
  year_val = 2020;
  dayOfWeek = 4;
  
  dispTemp = 25;
  dispHum = 30;
  dispCO2 = 300;
  dispPres = 10;
  dispRain = 1;
}

void testScreen(){
  lcd.setCursor(0,0);
  lcd.print("Hello, world!");  
}

void testTime2(){
  Serial.println("Year:");
  Serial.println(year_val);

  Serial.println("Month:");
  Serial.println(mnth);

  Serial.println("Day:");
  Serial.println(day_val);

  Serial.println("dayOfTheWeek:");
  Serial.println(dayOfWeek);
  
  Serial.println("Hours:");
  Serial.println(hrs);
  
  Serial.println("Minutes:");
  Serial.println(mins);
  
  Serial.println("Seconds:");
  Serial.println(secs);
}

int testLedState = 1;
void testSensorButton(){
  button.tick();
  
  if (button.isClick()) {
    if(testLedState==1){
      testLedState = 2;
    } else {
      testLedState = 1;
    }
    setLED(testLedState);
    Serial.println("Button click");
  }
  if (button.isHolded()) {    
    setLED(3);
    Serial.println("Button hold");
  }
}

int testLedStatePhoto = 1;
void testPhotoResistor(){
  int photoResistorValue = analogRead(PHOTO);
  if(photoResistorValue!=0){
    Serial.println(photoResistorValue);  
  }
  if (photoResistorValue < BRIGHT_THRESHOLD) {   // если темно
    //Serial.println("dark!");
    testLedStatePhoto = 1;
  } else {
    //Serial.println("light");
    testLedStatePhoto = 2;
  }
  setLED(testLedStatePhoto);
}

void testLed(){
  setLED(random(1, 3));
}

void testBmeSensor2(){
  Serial.println("temp:");
  Serial.println(dispTemp);
  Serial.println("humidity:");
  Serial.println(dispHum);
  Serial.println("amtosphere pressure:");
  Serial.println(dispPres);
}

void testCO2Sensor2(){
  Serial.println("CO2:");
  Serial.println(dispCO2);
}

void testScreenData(){
  analogWrite(BACKLIGHT, LCD_BRIGHT_MIN); // set low to not bother
  
  drawClock(10,33,0,0,1);
  setStubData();
  drawData();
  drawSensors();
}
