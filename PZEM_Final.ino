#include <WebServer.h> //webserver libraries (automatic)
#include <WiFi.h> //wifi libraries (automatic)
#include <WiFiClient.h>
#include <WiFiServer.h>

#include <PZEM004Tv30.h> //pzem library
#include <LiquidCrystal_I2C.h> //LCD library

//pzem sensor pins
PZEM004Tv30 pzem1(Serial, 3, 1);
PZEM004Tv30 pzem2(Serial2, 10, 9);

//LCD
LiquidCrystal_I2C lcd (0x27, 16,2);


void setup() {
  Serial.begin(9600);
  Serial.println ("ACS712 current sensor"); 
  lcd.init();
  
  lcd.backlight();
  lcd.print ("ACS712 current");
  lcd.setCursor (0, 1);
  lcd.print ("sensor");
  delay(1000);
  lcd.clear();  
}

void loop() {
//1st pzem sensor
    Serial.print("Custom Address:");
    Serial.println(pzem1.readAddress(), HEX);
    Serial2.println(pzem2.readAddress(), HEX);


    // Read the data from the pzem1
    float voltage1 = pzem1.voltage();      float voltage2 = pzem2.voltage();
    float current1 = pzem1.current();      float current2 = pzem2.current();
    float power1 = pzem1.power();          float power2 = pzem2.power();
    float energy1 = pzem1.energy();        float energy2 = pzem2.energy();
    float frequency1 = pzem1.frequency();  float frequency2 = pzem2.frequency();
    float pf1 = pzem1.pf();                float pf2 = pzem2.pf();

    // Check if the data1 is valid
    if(isnan(voltage1)){
        Serial.println("Error reading Inverter voltage");
    } else if (isnan(current1)) {
        Serial.println("Error reading Inverter current");
    } else if (isnan(power1)) {
        Serial.println("Error reading Inverter power");
    } else if (isnan(energy1)) {
        Serial.println("Error reading Inverter energy");
    } else if (isnan(frequency1)) {
        Serial.println("Error reading Inverter frequency");
    } else if (isnan(pf1)) {
        Serial.println("Error reading Inverter power factor");
    } else {

        // Print the values to the Serial console
        Serial.print("Inverter Voltage: ");      Serial.print(voltage1);      Serial.println("V");
        Serial.print("Inverter Current: ");      Serial.print(current1);      Serial.println("A");
        Serial.print("Inverter Power: ");        Serial.print(power1);        Serial.println("W");
        Serial.print("Inverter Energy: ");       Serial.print(energy1,3);     Serial.println("kWh");
        Serial.print("Inverter Frequency: ");    Serial.print(frequency1, 1); Serial.println("Hz");
        Serial.print("Inverter PF: ");           Serial.println(pf1);

    }

     // Check if the data2 is valid
    if(isnan(voltage2)){
        Serial2.println("Error reading Load voltage");
    } else if (isnan(current2)) {
        Serial2.println("Error reading Load current");
    } else if (isnan(power2)) {
        Serial2.println("Error reading Load power");
    } else if (isnan(energy2)) {
        Serial2.println("Error reading Load energy");
    } else if (isnan(frequency2)) {
        Serial2.println("Error reading Load frequency");
    } else if (isnan(pf2)) {
        Serial2.println("Error reading Load power factor");
    } else {

        // Print the values to the Serial console
        Serial2.print("Load Voltage: ");      Serial2.print(voltage2);      Serial2.println("V");
        Serial2.print("Load Current: ");      Serial2.print(current2);      Serial2.println("A");
        Serial2.print("Load Power: ");        Serial2.print(power2);        Serial2.println("W");
        Serial2.print("Load Energy: ");       Serial2.print(energy2,3);     Serial2.println("kWh");
        Serial2.print("Load Frequency: ");    Serial2.print(frequency2, 1); Serial2.println("Hz");
        Serial2.print("Load PF: ");           Serial2.println(pf2);

    }
    Serial.println();
    delay(2000);
    
//acs712 sensor
  int adc = analogRead(A6); //gpio34
  float voltage= map(adc, 0, 1023, -5  ,5);
  float current = ((voltage-2.5)/0.185);
  Serial.print("Battery Voltage: ");
  Serial.println(voltage);
  Serial.print("Battery Current: ");
  Serial.println(current);
  delay(1000);

  lcd.setCursor (0, 0);
  lcd.print (current);
  lcd.print (" A ");
  //Here cursor is placed on first position (col: 0) of the second line (row: 1) 
  lcd.setCursor (0, 1);
  lcd.print (voltage);
  lcd.print (" V ");
  delay (1000);
}

