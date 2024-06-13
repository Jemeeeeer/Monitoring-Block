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
PZEM004Tv30 pzem1(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x55); //inverter 
PZEM004Tv30 pzem2(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x56); //mains
PZEM004Tv30 pzem3(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x58); //load

// Declare global variables
int laststate_batt = LOW;
int laststate_mains = LOW;
String battData[1];
float InvData[4];
float MainsData[4];
float LoadData[4];
//FUNCTIONS-----------------------------------------------------------------------------------------------------------

//Collect Data from Sensors
void collectdata(String &battData, float InverterData[], float MainsData[], float LoadData[]) {
  //Battery Data
  battData = SwitchPower(laststate_batt, laststate_mains, MainsData);

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

String SwitchPower(int laststate_batt, int laststate_mains, float MainsData[]){
  float mainsvolt = MainsData[0];
  laststate_batt = digitalRead(BATTERY_PIN); //state of batt
  laststate_mains = digitalRead(MAINS_PIN); //state of mains
  int batt_disconnect = (analogRead(BATT_DISCONNECT_PIN) > 2048) ? HIGH : LOW; //state of disconnect voltage
  int batt_reconnect = (analogRead(BATT_RECONNECT_PIN) > 2048) ? HIGH : LOW; //state of reconnect voltage
  String chargebatt;
  
  Serial.print("24.5 PIN: ");
  Serial.println(batt_disconnect);
  Serial.print("27 PIN: ");
  Serial.println(batt_reconnect);

  //Control Logic
  if(batt_reconnect==1){ //max threshold on
      if(laststate_batt == 0){ //last state of batt is on
        chargebatt = "Battery Available (Discharging)";
        //do nothing
      }else if (laststate_mains == 0){ // last state of main is on
        chargebatt = "Battery Available (Charging)";
        digitalWrite(BATTERY_PIN, LOW); // batt on
        digitalWrite(MAINS_PIN, HIGH); // mains off
        delay(100);
        digitalWrite(BATTERY_PIN, HIGH); // toggle
      }
    }else if (batt_disconnect == 1 && batt_reconnect ==0){ //max threshold off and min threshold on
      if(laststate_batt == 0 && laststate_mains ==0){ 
        chargebatt = "No Supply";
      }else if(laststate_batt == 0){ // last state of batt is on
        chargebatt = "Battery Available (Discharging)";
        //do nothing
      }else if(laststate_mains == 0){ // last state of mains is on
        chargebatt = "Battery Available (Charging)";
        //do nothing
      }
    }else if(mainsvolt != 0){
      chargebatt = "Battery Not Available (Charging)";
      digitalWrite(BATTERY_PIN, HIGH); // batt off
      digitalWrite(MAINS_PIN, LOW); // mains on
      delay(100);
      digitalWrite(MAINS_PIN, HIGH); // toggle
    }else{
      chargebatt = "No Supply";
    }
    return chargebatt;
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
              <td>${data.batteryAvailability.toFixed(2)} V</td>
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
    // Initialize serial communication
    Serial.begin(115200);
    pinMode(BATTERY_PIN, OUTPUT);
    pinMode(MAINS_PIN, OUTPUT);
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
        data += "\"batteryAvailability\":";   data+=SwitchPower(laststate_batt, laststate_mains, MainsData); data+= "}";
        
        request->send(200, "application/json", data);
    });

    // Start server
    server.begin();
}

void loop() {
  //Declare Arrays to hold data
  String battData[1];
  float InvData[4];
  float MainsData[4];
  float LoadData[4];
  
  //Colllect Data
  collectdata(battData[0], InvData, MainsData, LoadData);
  
  //Determine Power Source  
  SwitchPower(laststate_batt, laststate_mains, MainsData);

  //For monitoring through Serial Monitor
  String SelectedPowerSource = SwitchPower(laststate_batt, laststate_mains, MainsData);
  Serial.print("Selected Power Source:");
  Serial.println(SelectedPowerSource);
  Serial.print("Battery Last State: ");
  Serial.println(digitalRead(BATTERY_PIN));
  Serial.print("Mains Last State: ");
  Serial.println(digitalRead(MAINS_PIN));
  
  int batt_disconnect = (analogRead(BATT_DISCONNECT_PIN) > 2048) ? HIGH : LOW; //state of disconnect voltage
  int batt_reconnect = (analogRead(BATT_RECONNECT_PIN) > 2048) ? HIGH : LOW; //state of reconnect voltage
  
  Serial.print("Battery Disconnect Output (24.5): ");
  Serial.println(batt_disconnect);
  Serial.print("Battery Reconnect Output (27): ");
  Serial.println(batt_reconnect);
  
  delay (1000);
  Serial.println("");
  
 
}
