#include <PZEM004Tv30.h>
#include <LiquidCrystal_I2C.h>

//acs712 sensor
const int sensorIn = 34;      
int mVperAmp = 100;           
int Watt = 0;
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;

//pzem sensor
PZEM004Tv30 pzem1(Serial, 3, 1);
PZEM004Tv30 pzem2(Serial, 9, 10);

//LCD
LiquidCrystal_I2C lcd (0x27, 16,2);

void setup() {
  Serial.begin(115200);
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
  float voltage1 = pzem1.voltage();
  if(!isnan(voltage1)){
    Serial.print("Voltage: "); 
    Serial.print(voltage1); 
    Serial.println("V");
  } else {
    Serial.println("Error reading voltage");
  }
  float current1 = pzem1.current();
  if(!isnan(current1)){
    Serial.print("Current: "); 
    Serial.print(current1); 
    Serial.println("A");
  } else {
    Serial.println("Error reading current");
  }

  float power1 = pzem1.power();
  if(!isnan(power1)){
    Serial.print("Power: "); 
    Serial.print(power1); 
    Serial.println("W");
  } else {
    Serial.println("Error reading power");
  }
  
  float energy1 = pzem1.energy();
  if(!isnan(energy1)){
    Serial.print("Energy: "); 
    Serial.print(energy1,3); 
    Serial.println("kWh");
  } else {
    Serial.println("Error reading energy");
  }

  float frequency1 = pzem1.frequency();
   if(!isnan(frequency1)){
    Serial.print("Frequency: "); 
    Serial.print(frequency1, 1); 
    Serial.println("Hz");
  } else {
    Serial.println("Error reading frequency");
  }
  delay(1);
  float pf1 = pzem1.pf();
   if(!isnan(pf1)){
    Serial.print("PF: "); 
    Serial.println(pf1);
  } else {
    Serial.println("Error reading power factor");
  }
  
  Serial.println();
  delay(2000);

//2nd pzem sensor
  float voltage2 = pzem2.voltage();
  if(!isnan(voltage2)){
    Serial.print("Voltage: "); 
    Serial.print(voltage2); 
    Serial.println("V");
  } else {
    Serial.println("Error reading voltage");
  }
  float current2 = pzem2.current();
  if(!isnan(current2)){
    Serial.print("Current: "); 
    Serial.print(current2); 
    Serial.println("A");
  } else {
    Serial.println("Error reading current");
  }

  float power2 = pzem2.power();
  if(!isnan(power2)){
    Serial.print("Power: "); 
    Serial.print(power2); 
    Serial.println("W");
  } else {
    Serial.println("Error reading power");
  }
  
  float energy2 = pzem2.energy();
  if(!isnan(energy2)){
    Serial.print("Energy: "); 
    Serial.print(energy2,3); 
    Serial.println("kWh");
  } else {
    Serial.println("Error reading energy");
  }

  float frequency2 = pzem1.frequency();
   if(!isnan(frequency2)){
    Serial.print("Frequency: "); 
    Serial.print(frequency2, 1); 
    Serial.println("Hz");
  } else {
    Serial.println("Error reading frequency");
  }
  
  float pf2 = pzem1.pf();
   if(!isnan(pf2)){
    Serial.print("PF: "); 
    Serial.println(pf2);
  } else {
    Serial.println("Error reading power factor");
  }
  
  Serial.println();
  delay(2000);

//acs712 sensor
  Serial.println (""); 
  Voltage = getVPP();
  VRMS = (Voltage/2.0) *0.707;   //root 2 is 0.707
  AmpsRMS = ((VRMS * 1000)/mVperAmp)-0.1; //0.3 is the error I got for my sensor
 
  Serial.print(AmpsRMS);
  Serial.print(" Amps RMS  ---  ");
  Watt = ((AmpsRMS*240/1.2)+1);
  // note: 1.2 is my own empirically established calibration factor
  // as the voltage measured at D34 depends on the length of the OUT-to-D34 wire
  // 240 is the main AC power voltage – this parameter changes locally
  Serial.print(VRMS);
  Serial.println(" Voltage");
  lcd.setCursor (0, 0);
  lcd.print (AmpsRMS);
  lcd.print (" Amps ");
  //Here cursor is placed on first position (col: 0) of the second line (row: 1) 
  lcd.setCursor (0, 1);
  lcd.print (VRMS);
  lcd.print (" V ");
  delay (2000);
}

// ***** function calls ******
float getVPP()
{
  float result;
  int readValue;                // value read from the sensor
  int maxValue = 0;             // store max value here
  int minValue = 4096;          // store min value here ESP32 ADC resolution
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue = analogRead(sensorIn);
       // see if you have a new maxValue
       if (readValue > maxValue) 
       {
           /*record the maximum sensor value*/
           maxValue = readValue;
       }
       if (readValue < minValue) 
       {
           /*record the minimum sensor value*/
           minValue = readValue;
       }
   }
   
   // Subtract min from max
   result = ((maxValue - minValue) * 3.3)/4096.0; //ESP32 ADC resolution 4096
      
   return result;
 }
