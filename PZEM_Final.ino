// Include necessary libraries
#include "WiFi.h"
#include "ESPAsyncWebServer.h"
#include <PZEM004Tv30.h>

//Define Threshold for voltages and current
#define Batt_Disconnect_Voltage 24.5 
#define Batt_Reconnect_Voltage 27.0

//Define Battery Threshold Voltage Pins
#define BATT_DISCONNECT_PIN 34 // for 24.5V disconnect
#define BATT_RECONNECT_PIN 35 // for 27V reconnect
  
//Define POWERSWITCHING Pins
#define BATTERY_PIN 25 //Change as needed.
#define MAINS_PIN 26 //Change as needed.

// Define PZEM pins and serial interface
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#define PZEM_SERIAL Serial2

// Create PZEM instance
PZEM004Tv30 pzem1(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x56); //inverter
PZEM004Tv30 pzem2(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x55); //mains
PZEM004Tv30 pzem3(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x58); //load

// Declare global variables
int laststate_batt = LOW;
int laststate_mains = LOW;
String battData;
float InvData[4] = {0.0, 0.0, 0.0, 0.0};
float MainsData[4] = {0.0, 0.0, 0.0, 0.0};
float LoadData[4] = {0.0, 0.0, 0.0, 0.0};

String SelectedPowerSource;
// Forward declaration (prototype) of selectpowersource function
String selectpowersource(float InvData, float MainsData, float LoadData);


//FUNCTIONS-----------------------------------------------------------------------------------------------------------

//Collect Data from Sensors
void collectdata(float InverterData[], float MainsData[], float LoadData[]) {

 //AC Main Line Data
  MainsData[0] = pzem2.voltage();
  MainsData[1] = pzem2.current();
  MainsData[2] = pzem2.power();
  MainsData[3] = pzem2.energy();
  
  //Inverter Data
  InverterData[0] = pzem1.voltage();
  InverterData[1] = pzem1.current();
  InverterData[2] = pzem1.power();
  InverterData[3] = pzem1.energy();

 //Connected Load Data
  LoadData[0] = pzem3.voltage();
  LoadData[1] = pzem3.current();
  LoadData[2] = pzem3.power();
  LoadData[3] = pzem3.energy();
}

String selectpowersource(float InvData[], float MainsData[],float LoadData[]){
  collectdata(InvData, MainsData, LoadData);
  float invvolt = InvData[0];
  float invcur = InvData[1];
  float invpow = InvData[2];
  float invene = InvData[3];

  float mainsvolt = MainsData[0];
  float mainscurt = MainsData[1];
  float mainspow = MainsData[2];
  float mainsene = MainsData[3];

  float loadvolt = LoadData[0];
  float loadcur = LoadData[1];
  float loadpow = LoadData[2];
  float loadene = LoadData[3];

  laststate_batt = digitalRead(13); //battery
  laststate_mains = digitalRead(14); //mains
  int batt_disconnect = (analogRead(BATT_DISCONNECT_PIN) > 2048) ? HIGH : LOW; //state of disconnect voltage
  int batt_reconnect = (analogRead(BATT_RECONNECT_PIN) > 2048) ? HIGH : LOW; //state of reconnect voltage
  String chargebatt;
  
  if(batt_reconnect==1){ //max threshold on
      if(laststate_batt == 0){ //last state of batt is on
        chargebatt = "Battery Available (Discharging) Using Battery";
        //do nothing
      }else if (laststate_mains == 0){ // last state of main is on
        chargebatt = "Battery Available (Charging) Using Battery";
      }
    }else if (batt_disconnect == 1 && batt_reconnect ==0){ //max threshold off and min threshold on
      if(laststate_batt == 1 && laststate_mains ==1){ 
        chargebatt = "No Supply";
      }else if(laststate_batt == 0){ // last state of batt is on
        chargebatt = "Battery Available (Discharging) Using Battery";
        //do nothing
      }else if(laststate_mains == 0){ // last state of mains is on
        chargebatt = "Battery Available (Charging) Using Mains";
        //do nothing
      }
    }else if(batt_disconnect == 0){
      if(mainsvolt != 0 && laststate_mains == 0){ //last state of mains is on
        chargebatt = "Battery Unavailable (Charging) Still using Mains";
        //do nothing
      }else if(mainsvolt != 0 && laststate_mains == 1) { //last state of mains is off
        chargebatt = "Battery Unavailable (Charging) Using Mains";
    }else{
      chargebatt = "No Supply";
    }
    }
    Serial.print("Mains Volt: ");
    Serial.println(mainsvolt);
    return chargebatt;
  
}

void SwitchPower(){
  SelectedPowerSource = selectpowersource(InvData, MainsData, LoadData);
  laststate_batt = digitalRead(13); //battery
  laststate_mains = digitalRead(14); //mains
  //Control Logic
  if(SelectedPowerSource == "Battery Available (Discharging) Using Battery"){ //max threshold on
      if(laststate_batt == 0){ //last state of batt is on
        //do nothing
      }else if (latstate_batt == 1){ // last state of batt is off
        digitalWrite(BATTERY_PIN, HIGH); // batt on
        digitalWrite(MAINS_PIN, LOW); // mains off
        delay(3000);
        digitalWrite(BATTERY_PIN, LOW); // toggle//max threshold off and min threshold on
      }
    }else if (SelectedPowerSource == "Battery Available (Charging) Using Battery"){
      if(laststate_batt == 0){
        //do nothing
      }else{
        digitalWrite(BATTERY_PIN, HIGH); // batt on
        digitalWrite(MAINS_PIN, LOW); // mains off
        delay(3000);
        digitalWrite(BATTERY_PIN, LOW); // toggle//max threshold off and min threshold on
      }
    }else if (SelectedPowerSource == "Battery Available (Charging) Using Mains"){
        //do nothing
    }else if (SelectedPowerSource == "Battery Unavailable (Charging) Using Mains"){
      if(laststate_mains == 0){
        //do nothing
      }else{
        digitalWrite(BATTERY_PIN, LOW); // batt off
        digitalWrite(MAINS_PIN, HIGH); // mains on
        delay(3000);
        digitalWrite(MAINS_PIN, LOW); // toggle//max threshold off and min threshold on
      }
    }else if (SelectedPowerSource == "Battery Unavailable (Charging) Still using Mains"){
        //do nothing
    }else if (SelectedPowerSource == "No Supply"){
      if(laststate_mains == 1 && laststate_mains ==1){
        //do nothing
      }else{
        digitalWrite(BATTERY_PIN, LOW); // batt off
        digitalWrite(MAINS_PIN, LOW); // mains on
      }
    }
}

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
        <td id="mainsVoltage"> </td>
        <td id="inverterVoltage"></td>
        <td id="loadVoltage"></td>
        <td id="batteryAvailability"></td>
      </tr>
      <tr>
        <th>Current</th>
        <td id="mainsCurrent"></td>
        <td id="inverterCurrent"></td>
        <td id="loadCurrent"></td>
        <td></td>
      </tr>
      <tr>
        <th>Energy</th>
        <td id="mainsEnergy"></td>
        <td id="inverterEnergy"></td>
        <td id="loadEnergy"></td>
        <td></td>
      </tr>
      <tr>
        <th>Power</th>
        <td id="mainsPower"></td>
        <td id="inverterPower"></td>
        <td id="loadPower"></td>
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
              <td></td>
              <td>${data.batteryAvailability}</td>
            </tr>
            <tr>
              <th>Current</th>
              <td>${data.inverterCurrent.toFixed(2)} A</td>
              <td>${data.mainsCurrent.toFixed(2)} A</td>
              <td>${data.loadCurrent.toFixed(2)} A</td>
              <td></td>
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

void setup() {
    SelectedPowerSource = selectpowersource(InvData, MainsData, LoadData);  // Assign to global variable
    // Initialize serial communication
    Serial.begin(115200);
    pinMode(BATT_DISCONNECT_PIN, INPUT);
    pinMode(BATT_RECONNECT_PIN, INPUT);
    
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
        
        data += "\"batteryAvailability\":" += SelectedPowerSource + "\"}";
        
        request->send(200, "application/json", data);
    });

    // Start server
    server.begin();
}

void loop() {
  //Declare Arrays to hold data
  float InvData[4];
  float MainsData[4];
  float LoadData[4];
  
  //Colllect Data
  collectdata(InvData, MainsData, LoadData);
  SelectedPowerSource = selectpowersource(InvData, MainsData, LoadData);
  SwitchPower();
   
  laststate_batt = digitalRead(13); //battery
  laststate_mains = digitalRead(14); //mains

  //For monitoring through Serial Monitor
  Serial.print("Selected Power Source:");
  Serial.println(SelectedPowerSource);
  Serial.print("Battery Last State: ");
  Serial.println(laststate_batt);
  Serial.print("Mains Last State: ");
  Serial.println(laststate_mains);
  
  int batt_disconnect = (analogRead(BATT_DISCONNECT_PIN) > 2048) ? HIGH : LOW; //state of disconnect voltage
  int batt_reconnect = (analogRead(BATT_RECONNECT_PIN) > 2048) ? HIGH : LOW; //state of reconnect voltage
  
  Serial.print("Battery Disconnect Output (24.5): ");
  Serial.println(batt_disconnect);
  Serial.print("Battery Reconnect Output (27): ");
  Serial.println(batt_reconnect);
  Serial.println("");
}
