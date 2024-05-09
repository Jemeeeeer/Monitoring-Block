// Include necessary libraries
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <PZEM004Tv30.h>
#include <LiquidCrystal_I2C.h>

//Define Threshold for voltages and current
#define Batt_Disconnect_Voltage 24.5 
#define Batt_Reconnect_Voltage 27.0
#define Min_Batt_Current 0.0

//Define POWERSWITCHING Pins
#define BATTERY_PIN 25 //Change as needed.
#define MAINS_PIN 26 //Change as needed.

// Define PZEM pins and serial interface
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define PZEM_SERIAL Serial2

// Create PZEM instance
PZEM004Tv30 pzem1(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x55); //inverter 
PZEM004Tv30 pzem2(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x56); //mains
PZEM004Tv30 pzem3(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x58); //load


double VRMS = 0;
double AmpsRMS = 0;
float getVoltage();
float getCurrent(); 
float getVPP(); 

//Collect Data from Sensors
void collectdata(float battData[], float InverterData[], float MainsData[], float LoadData[]) {
  //Battery Data
  battData[0] = getVPP();
  battData[1] = getCurrent();

  //Inverter Data
  InverterData[0] = pzem1.voltage();
  InverterData[1] = pzem1.current();
  InverterData[2] = pzem1.power();
  InverterData[3] = pzem1.energy();

  //AC Main Line Data
  MainsData[0] = pzem2.voltage();
  MainsData[1] = pzem2.current();
  MainsData[2] = pzem2.power();
  MainsData[3] = pzem2.energy();

 //Connected Load Data
  LoadData[0] = pzem3.voltage();
  LoadData[1] = pzem3.current();
  LoadData[2] = pzem3.power();
  LoadData[3] = pzem3.energy();

}

//Determine Which PowerSource to use
//Currently only checking Voltage. Subject to change.
String selectpowersource(float battData[],float InvData[],float MainsData[],float LoadData[]){
  //assign data to variables
  float battvolt = battData[0];
  float battcur = battData[1];
  
  float invvolt = InvData[0];
  float invcur = InvData[1];
  float invpow = InvData[2];
  float invene= InvData[3];
  
  float mainsvolt = MainsData[0];
  float mainscur = MainsData[1];
  float mainspow = MainsData[2];
  float mainsene = MainsData[3];

  float loadvolt =LoadData[0];
  float loadcur = LoadData[1];
  float loadpow = LoadData[2];
  float loadene = LoadData[3];

  String static battstate = "connected";

  //begin decision logic
  // If battery between Reconnect/Disconnect Voltages
  if  (battvolt > Batt_Disconnect_Voltage && battvolt < Batt_Reconnect_Voltage){
    //If Battery is Connected use battery, else check Mains. 
    if (battstate == "connected"){
      return "Battery";
    }
    else if (battstate == "disconnected"){
      if (mainsvolt != 0){
        return "Mains";
      } 
    else{
        return "NoSource";
      }
    }
  }
  // Disconnect Battery 
  if (battvolt <= Batt_Disconnect_Voltage){
    battstate = "disconnected";
    if (mainsvolt != 0){
        return "Mains";
      } 
    else{
        return "NoSource";
      }
  }
  //Reconnect Battery
  if (battvolt >= Batt_Reconnect_Voltage){
    battstate = "connected";
    return "Battery";
  }
  return "Error Selecting Source"; // Return "Unknown" if none of the conditions are met
}

void SwitchPower(String selectedpowersource){
  if (selectedpowersource == "Battery"){
    digitalWrite(BATTERY_PIN, HIGH); // Turn on battery
    digitalWrite(MAINS_PIN, LOW);    // Turn off mains
  }
  else if (selectedpowersource == "Mains"){
    digitalWrite(BATTERY_PIN, LOW); // Turn off battery
    digitalWrite(MAINS_PIN, HIGH);    // Turn on mains
  }
  else {
    digitalWrite(BATTERY_PIN, LOW); // Turn off battery
    digitalWrite(MAINS_PIN, LOW);    // Turn off mains
  }
}

//LCD declaration
LiquidCrystal_I2C lcd (0x27, 16,2); 

// ACS712 Declarations
const int sensorIn = 34;      // pin where the OUT pin from sensor is connected on Arduino
int mVperAmp = 100;           // this the 5A version of the ACS712 -use 100 for 20A Module and 66 for 30A Module
int Watt = 0;
double Voltage = 0;
double Current =0;


// WiFi credentials
const char* ssid = "Jem";
const char* password = "jemerjaps";


const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;


// Create AsyncWebServer instance
AsyncWebServer server(80);

// HTML document with CSS and JS 
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>

<head>
  <title>Smart Home Monitoring</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    canvas {
      -moz-user-select: none;
      -webkit-user-select: none;
      -ms-user-select: none;
    }
  
    /* Data Table Styling */
    #dataTable {
      font-family: "Trebuchet MS", Arial, Helvetica, sans-serif;
      border-collapse: collapse;
      width: 95%;
      margin-top: 40px;
      text-align: center;
    }

    #dataTable td,
    #dataTable th {
      border: 1px solid #ddd;
      padding: 8px;
      font-size: 18px; /* Increase font size */
      font-weight: bold; /* Make it bold */
    }

    #dataTable tr:nth-child(even) {
      background-color: #659EC7;
    }

    #dataTable tr:nth-child(odd) {
      background-color: #95B9C7;
    }

    #dataTable td:hover {
      background-color: #ddd;
    }

    #dataTable th {
      padding-top: 12px;
      padding-bottom: 12px;
      text-align: center;
      background-color: #151B54;
      color: white;
    }
    .button {
      display: inline-block;
      border-radius: 4px;
      background-color: #e7e7e7;
      border: none;
      color: black;
      text-align: center;
      font-size: 15px;
      padding: 6px;
      width: 100px;
      transition: all 0.5s;
      cursor: pointer;
      margin: 30px;
      border: 1px solid black;
}

  .button-container span {
      cursor: pointer;
      display: inline-block;
      position: relative;
      transition: 0.5s;
  }

  .button-container span:after {
    content: '\00bb';
    position: absolute;
    opacity: 0;
    top: 0;
    right: -20px;
    transition: 0.5s; 
  }

  .button-container:hover span {
    padding-right: 25px;
  }

  .button-container:hover span:after {
    opacity: 1;
    right: 0;
  }
  </style>
</head>

<body style="background-color: #242424;">
  <div style="text-align:center; color: #0492C2; font-family: sans-serif; font-size:300%; font-weight: bold;">SMART HOME MONITOR</div>

  <div class="button-container">
        <button class="button" onclick="redirectToHost()"><< HOME</button>
  </div>
  
  <div style="margin-top: 20px; margin: 0 auto; width: 0 auto;">
    <table id="dataTable" style="margin: 0 auto;">
      <tr>
        <th> </th>
        <th>Mains</th>
        <th>Inverter</th>
        <th>Load</th>
        <th>Battery</th>
      </tr>
      <tr>
        <th>Voltage</th>
        <td> </td>
        <td> </td>
        <td> </td>
        <td> </td>
      </tr>
      <tr>
        <th>Current</th>
        <td> </td>
        <td> </td>
        <td> </td>
        <td> </td>
      </tr>
      <tr>
        <th>Energy</th>
        <td> </td>
        <td> </td>
        <td> </td>
        <td> </td>
      </tr>
      <tr>
        <th>Power</th>
        <td> </td>
        <td> </td>
        <td> </td>
        <td> </td>
      </tr>
    </table>
  </div>
  <br>
  <br>

  <script>
    // Function to update sensor data from ESP32
    function updateSensorData() {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function() {
            if (this.readyState == 4 && this.status == 200){
          var data = JSON.parse(this.responseText);
          // Update table data
          document.getElementById("dataTable").innerHTML = `
            <tr>
              <th> </th>
              <th>Mains</th>
              <th>Inverter</th>
              <th>Load</th>
              <th>Battery</th>
            </tr>
            <tr>
              <th>Voltage</th>
              <td>${data.inverterVoltage.toFixed(2)} V</td>
              <td>${data.mainsVoltage.toFixed(2)} V</td>
              <td>${data.loadVoltage.toFixed(2)} V</td>
              <td>${data.batteryVoltage.toFixed(2)} V</td>
            </tr>
            <tr>
              <th>Current</th>
              <td>${data.inverterCurrent.toFixed(2)} A</td>
              <td>${data.mainsCurrent.toFixed(2)} A</td>
              <td>${data.loadCurrent.toFixed(2)} A</td>
              <td>${data.batteryCurrent.toFixed(2)} A</td>
            </tr>
            <tr>
              <th>Energy</th>
              <td>${data.inverterEnergy.toFixed(2)} kWh</td>
              <td>${data.mainsEnergy.toFixed(2)} kWh</td>
              <td>${data.loadEnergy.toFixed(2)} kWh</td>
              <td></td>
            </tr>
            <tr>
              <th>Power</th>
              <td>${data.inverterPower.toFixed(2)} W</td>
              <td>${data.mainsPower.toFixed(2)} W</td>
              <td>${data.loadPower.toFixed(2)} W</td>
              <td></td>
            </tr>
          `;
            }
        };
        xhttp.open("GET", "/data", true);
        xhttp.send();
    }

    function updateDataTable(data) {
      var table = document.getElementById("dataTable");
      var row = table.insertRow(1); // Add after headings
      var cell1 = row.insertCell(0);
      var cell2 = row.insertCell(1);
      var cell3 = row.insertCell(2);
      cell1.innerHTML = getCurrentTime(); // You need to implement this function
      cell2.innerHTML = data.batteryVoltage.toFixed(2) + " V";
      cell3.innerHTML = data.batteryCurrent.toFixed(2) + " A";
    }
    
    updateSensorData();
    setInterval(updateSensorData, 1500); // Update every 1.5 seconds
  
    
    //On Page load show graphs
    window.onload = function () {
      console.log(new Date().toLocaleTimeString());
      getData();
      showGraph();
    };

    //Ajax script to get ADC voltage at every 5 Seconds 

    setInterval(function () {
      // Call a function repetitively with 5 Second interval
      getData();
    }, 5000);
    
    function redirectToHost() {
        window.location.href = "http://172.20.10.12"; // Replace with actual IP address of the host
    }
  </script>
</body>

</html>

)rawliteral";

//  ACS Function
float getVoltage()
{
  float result = 0.0;
  int readValue = 0;                // value read from the sensor
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue = analogRead(sensorIn);
       Voltage = (readValue *(5/1023.0))-3.5; //Gets V
       return Voltage;
 }
}

float getCurrent() {
  float current_voltage = getVoltage();
  float acs_current = ((current_voltage) / 0.1); //0.185V/A is the sensitivity of the ACS712
  return acs_current;
}

float getVPP()
{
float result;
int readValue; // value read from the sensor
int maxValue = 0; // store max value here
int minValue = 4096; // store min value here ESP32 ADC resolution

uint32_t start_time = millis();
while((millis()-start_time) < 1000) { // sample for 1 second
        readValue = analogRead(sensorIn);
        if (readValue > maxValue) {
            maxValue = readValue;
        }
        if (readValue < minValue) {
            minValue = readValue;
        }
    }
// Subtract min from max
 // Subtract min from max
    result = ((maxValue - minValue) * 3.3) / 4096.0; // ESP32 ADC resolution 4096

return result;
}

void setup() {
    // Initialize serial communication
    Serial.begin(115200);
    pinMode(BATTERY_PIN, OUTPUT);
    pinMode(MAINS_PIN, OUTPUT);
  
    // Connect to WiFi
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to Network.....");
        Serial.print("WiFi Status: ");
        Serial.println(WiFi.status());
        attempts++;
        if (attempts > 20) {
            Serial.println("Failed to connect to WiFi. Please check your credentials.");
            break;
        }
    }

    // Print ESP32-S local IP address
    Serial.println("WiFi Connected");
    Serial.print("Local Network Address: ");
    Serial.println(WiFi.localIP());

    // Route to get to the WebPage
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

    // Route to get sensor data
    server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
        String data = "{\"inverterVoltage\":"; if(isnan(pzem1.voltage())){data+=String(0);} else {data+=String(pzem1.voltage());} data+= ",";
        data += "\"inverterCurrent\":";   if(isnan(pzem1.current())){data+=String(0);} else {data+=String(pzem1.current());} data+= ",";
        data += "\"inverterPower\":";   if(isnan(pzem1.power())){data+=String(0);} else {data+=String(pzem1.power());} data+= ",";
        data += "\"inverterEnergy\":";   if(isnan(pzem1.energy())){data+=String(0);} else {data+=String(pzem1.energy());} data+= ",";
        data += "\"mainsVoltage\":";   if(isnan(pzem2.voltage())){data+=String(0);} else {data+=String(pzem2.voltage());} data+= ",";
        data += "\"mainsCurrent\":";   if(isnan(pzem2.current())){data+=String(0);} else {data+=String(pzem2.current());} data+= ",";
        data += "\"mainsPower\":";   if(isnan(pzem2.power())){data+=String(0);} else {data+=String(pzem2.power());} data+= ",";
        data += "\"mainsEnergy\":";   if(isnan(pzem2.energy())){data+=String(0);} else {data+=String(pzem2.energy());} data+= ",";
        data += "\"loadVoltage\":";   if(isnan(pzem3.voltage())){data+=String(0);} else {data+=String(pzem3.voltage());} data+= ",";
        data += "\"loadCurrent\":";   if(isnan(pzem3.current())){data+=String(0);} else {data+=String(pzem3.current());} data+= ",";
        data += "\"loadPower\":";   if(isnan(pzem3.power())){data+=String(0);} else {data+=String(pzem3.power());} data+= ",";
        data += "\"loadEnergy\":";   if(isnan(pzem3.energy())){data+=String(0);} else {data+=String(pzem3.energy());} data+= ",";
        data += "\"batteryVoltage\":";   if(isnan(getVoltage())){data+=String(0);} else {data+=String(getVoltage());} data+= ",";
        data += "\"batteryCurrent\":";   if(isnan(getCurrent())){data+=String(0);} else {data+=String(getCurrent());} data+= "}";
        
        request->send(200, "application/json", data);
    });

    // Start server
    server.begin();

    //LCD initalization
    lcd. init ();
    // Turn on the backlight on LCD. 
    lcd. backlight ();
    lcd.print ("Battery Voltage &");
    lcd. setCursor (0, 1);
    lcd.print ("Current Monitor");
    delay(1000);
    lcd.clear();  
}

void loop() {
  //Declare Arrays to hold data
  float battData[2];
  float InvData[4];
  float MainsData[4];
  float LoadData[4];

  //Colllect Data
  collectdata(battData, InvData, MainsData, LoadData);
  //Determine PowerSource
  String selectedpowersource = selectpowersource(battData, InvData, MainsData, LoadData);
  Serial.print("Current Source:");
  Serial.println(selectedpowersource);
  //Switch to Selected Source
  SwitchPower(selectedpowersource);
  //LCD ACS712
  Serial.println (""); 
  float Voltage = getVoltage();
  float Current = getCurrent(); 
  Serial.print(Voltage);
  Serial.print(" Volts  ---  ");
  Serial.println("Batt Pin: "); Serial.print(analogRead(25));
  Serial.print("Mains Pin: "); Serial.print(analogRead(26));
  lcd. setCursor (0, 0);
  lcd.print (Voltage);
  lcd.print (" V ");
  //Here cursor is placed on first position (col: 0) of the second line (row: 1) 
  lcd. setCursor (0, 1);
  lcd.print (Current);
  lcd.print (" A ");
  delay (1000);
  Serial.println("");
  
Voltage = getVPP();
VRMS = (Voltage/2.0) *0.707; //root 2 is 0.707
AmpsRMS = ((VRMS * 1000)/mVperAmp)-0.3; //0.3 is the error I got for my sensor
Serial.print("Voltage: ");
Serial.print(Voltage);
Serial.print("V ");
Serial.print("Current: ");
Serial.print(AmpsRMS);
Serial.print("A ");
Watt = (AmpsRMS*240/1.2);
// note: 1.2 is my own empirically established calibration factor
// as the voltage measured at D34 depends on the length of the OUT-to-D34 wire
// 240 is the main AC power voltage – this parameter changes locally
Serial.print("Power: ");
Serial.print(Watt);
Serial.println("W");
}
