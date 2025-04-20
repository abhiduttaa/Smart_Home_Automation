#include <ESP8266mDNS.h>
#include <LEAmDNS.h>
#include <LEAmDNS_Priv.h>
#include <LEAmDNS_lwIPdefs.h>

#include <ESP8266LLMNR.h>
#include<Arduino.h>
#include<SinricPro.h>
#include<SinricProSwitch.h>
//pin configuration
//3,12,13,14,15 for relay module
//2 for dht11
//16 for pir
//4,5 for display

#include <dummy.h>

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1306.h>
#include<DHT.h>

// EEPROM addresses
#define MODE_ADDR 0      // Address to store the mode (1 = AP, 2 = Client)
#define SSID_ADDR 1      // Address to store the SSID
#define PASSWORD_ADDR 33 // Address to store the Password
#define EEPROM_SIZE 65   // Enough to store mode, SSID, and password

#define ENABLE_DEBUG
#ifdef ENABLE_DEBUG
   #define DEBUG_ESP_PORT Serial
   #define NODEBUG_WEBSOCKETS
   #define NDEBUG
#endif 
#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32) || defined(ARDUINO_ARCH_RP2040)
  #include <WiFi.h>
#endif
#define WIFI_SSID         "test-wifi"
#define WIFI_PASS         "12345678#@"
#define APP_KEY           "***"//your sinricpro api key
#define APP_SECRET        "***"//your secret key
#define SWITCH_ID_1       "***"//your switch id
#define SWITCH_ID_2       "***"//same
#define SWITCH_ID_3       "***"//same
#define RELAYPIN_1 3      //define relay pin

#define RELAYPIN_2 12
#define RELAYPIN_3 14


#define BAUD_RATE         9600                // Change baudrate to your need



ESP8266WebServer server(80);
DHT dht(2,DHT11);
//Adafruit_SSD1306 display(128, 64, &Wire, -1);

String mode = "AP"; // Default mode
String ssid = "";
String password = "";

const char *devList[5]={"light1","light2","fan1","fan2","fan3"};
int devStates[5]={0,0,0,0,0};
const int devPins[5]={3,12,13,14,15};//for now 3,12,13 work on Sinric Pro
// in ap mode all the pins work
const int devNum=5;
int flag=0;

// Function to save settings in EEPROM
void saveSettings() {
    EEPROM.begin(EEPROM_SIZE);
    EEPROM.write(MODE_ADDR, (mode == "AP") ? 1 : 2);
    for (int i = 0; i < 32; i++) {
        EEPROM.write(SSID_ADDR + i, i < ssid.length() ? ssid[i] : 0);
        EEPROM.write(PASSWORD_ADDR + i, i < password.length() ? password[i] : 0);
    }
    EEPROM.commit();
    EEPROM.end();
}

// Function to load settings from EEPROM
void loadSettings() {
    EEPROM.begin(EEPROM_SIZE);
    int storedMode = EEPROM.read(MODE_ADDR                                                      
                                                      
                                                      
                                                      
);
    mode = (storedMode == 1) ? "AP" : "Client";

    ssid = "";
    password = "";
    for (int i = 0; i < 32; i++) {
        char c = EEPROM.read(SSID_ADDR + i);
        if (c != 0) ssid += c;
    }
    for (int i = 0; i < 32; i++) {
        char c = EEPROM.read(PASSWORD_ADDR + i);
        if (c != 0) password += c;
    }
    EEPROM.end();
}
//sinricpro
bool onPowerState1(const String &deviceId, bool &state) {
 Serial.printf("Device 1 turned %s", state?"on":"off");
 digitalWrite(RELAYPIN_1, state ? LOW:HIGH);
 return true; // request handled properly
}

bool onPowerState2(const String &deviceId, bool &state) {
 Serial.printf("Device 2 turned %s", state?"on":"off");
 digitalWrite(RELAYPIN_2, state ? LOW:HIGH);
 return true; // request handled properly
}

bool onPowerState3(const String &deviceId, bool &state) {
 Serial.printf("Device 3 turned %s", state?"on":"off");
 digitalWrite(RELAYPIN_3, state ? LOW:HIGH);
 return true; // request handled properly
}

void setupSinricPro() {
  // add devices and callbacks to SinricPro
  pinMode(RELAYPIN_1, OUTPUT);
  pinMode(RELAYPIN_2, OUTPUT);
  pinMode(RELAYPIN_3, OUTPUT);
  digitalWrite(RELAYPIN_1,HIGH);
  digitalWrite(RELAYPIN_2,HIGH);
  digitalWrite(RELAYPIN_3,HIGH);
  SinricProSwitch& mySwitch1 = SinricPro[SWITCH_ID_1];
  mySwitch1.onPowerState(onPowerState1);
  SinricProSwitch& mySwitch2 = SinricPro[SWITCH_ID_2];
  mySwitch2.onPowerState(onPowerState2);
  SinricProSwitch& mySwitch3 = SinricPro[SWITCH_ID_3];
  mySwitch3.onPowerState(onPowerState3);
  // setup SinricPro
  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.restoreDeviceStates(true); // Uncomment to restore the last known state from the server.
   
  SinricPro.begin(APP_KEY, APP_SECRET);
}

// Function to handle mode selection
void handleModeSelection() {
    if (server.hasArg("mode")) {
        mode = server.arg("mode");
        if (mode == "Client" && server.hasArg("ssid") && server.hasArg("password")) {
            ssid = server.arg("ssid");
            password = server.arg("password");

            saveSettings();
            //char tr1=WiFi.localIP();
            server.send(200, "text/plain", "Switching to Client mode...");
            delay(1000);
            ESP.restart();
        } else if (mode == "AP") {
            saveSettings();
            server.send(200, "text/plain", "Staying in AP mode...");
            delay(1000);
            ESP.restart();
            //server.send(200,)
        } else {
            server.send(400, "text/plain", "Invalid input!");
        }
    } else {
        server.send(400,"text/plain","Mode not specified!");
    }
}

// Function to start AP mode
void startAPMode() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP_AP_Mode", "password123");
    /*display.clearDisplay();  // Clear the display buffer
    display.setTextColor(SSD1306_WHITE);  // Set text color to white
    display.setTextSize(2);  // Set text size
    display.setCursor(0, 0);  // Set the cursor to the top left corner
    display.println("AP Mode");  // Print mode info on the display
    //display.setTextSize(2);  // Set smaller text size for more information
    //display.setCursor(0, 30);  // Set cursor position below the title
    //display.println("IP: ");
    display.print(WiFi.softAPIP());  // Print the AP IP address on the display
    display.display();  //*/
    Serial.println("Started AP Mode");
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());
    server.on("/",sendMainUI);
    server.on("/selMode", HTTP_GET, sendModeSel);
    server.on("/seldMode",HTTP_GET,handleModeSelection);
    server.on("/devs",HTTP_GET,handleDevs);
    server.on("/thsensor",sendTH);
    server.on("/pirsnsr",pirSen);
    server.on("/devState",sendDev);
    server.begin();
}

// Function to start Client mode
void startClientMode() {
    WiFi.mode(WIFI_STA);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID,WIFI_PASS);

    Serial.print("Connecting to Wi-Fi");
    unsigned long startAttemptTime = millis();

    // Wait for connection with a timeout
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 20000) {
        delay(1000);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to Wi-Fi");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
       /* display.clearDisplay();  // Clear the display buffer
    display.setTextColor(SSD1306_WHITE);  // Set text color to white
    display.setTextSize(2);  // Set text size
    display.setCursor(0, 0);  // Set the cursor to the top left corner
    display.println("Client Mode");  // Print mode info on the display
    //display.setTextSize(2);  // Set smaller text size for more information
    //display.setCursor(0, 30);  // Set cursor position below the title
    //display.print("IP: ");
    display.print(WiFi.localIP());  // Print the AP IP address on the display
    display.display();  //*/
        /*server.on("/",HTTP_GET,sendMainUI);
        server.on("/selMode", HTTP_GET, sendModeSel);
        server.on("/seldMode",HTTP_GET,handleModeSelection);
        server.on("/devs",HTTP_GET,handleDevs);
        server.on("/thsensor",sendTH);
        server.on("/pirsnsr",pirSen);
        server.on("/devState",sendDev);
        server.begin();*/
    } else {
        Serial.println("\nFailed to connect to Wi-Fi. Switching to AP mode.");
        mode = "AP";
        saveSettings();
        startAPMode();
    }
    setupSinricPro();
}

void sendDev()
{
    String ds="";
    for(int i=0;i<5;i++)
    {
        ds+=String(devList[i])+" is "+(devStates[i]?"on":"off")+"<br>";
    }
    server.send(200,"text/plain",ds);
}

void sendModeSel()
{
  String html1=R"====(
   <!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Wifi Mode</title>
    <style>
        body {
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
            background: linear-gradient(135deg, #022526, #d7cdcc);
            font-family: Arial, sans-serif;
            color: white;
        }
        .container {
            text-align: center;
            background: rgba(0, 0, 0, 0.7);
            padding: 40px;
            border-radius: 8px;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.5);
        }
        h1 {
            margin-bottom: 30px;
            font-size: 2.5em;
        }
        .wifi-symbol {
            font-size: 4em;
            margin-bottom: 20px;
        }
        .field {
            margin: 20px 0;
        }
        label {
            display: block;
            font-size: 1.2em;
            margin-bottom: 5px;
        }
        input {
            width: 80%;
            padding: 10px;
            font-size: 1em;
            border: none;
            border-radius: 4px;
        }
        .btn {
            display: inline-block;
            font-size: 1.5em;
            text-decoration: none;
            color: #fff;
            background: #4caf50;
            padding: 15px 20px;
            margin: 20px 0;
            border-radius: 4px;
            transition: background 0.3s;
            cursor: pointer;
            border: none;
        }
        .btn:hover {
            background: #45a049;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="wifi-symbol">📶</div>
        <h1>WIFI MODE</h1>
        <div class="field">
            <label for="ssid">SSID</label>
            <input type="text" id="ssid" name="ssid" placeholder="Enter Wi-Fi name">
        </div>
        <div class="field">
            <label for="password">Password</label>
            <input type="password" id="password" name="password" placeholder="Enter password">
        </div>
        <button class="btn" onclick="submitForm('Client')">start client mode</button>
        <br>
        <button class="btn" onclick="submitForm('AP')">stay in ap mode</button>
        <br>
        <button class="btn"><a href="/">Back</a></button>
    </div>
    <script>
        function submitForm(md) {
            const ssid = document.getElementById('ssid').value;
            const password = document.getElementById('password').value;
            fetch('/seldMode?mode='+md+'&ssid=' + ssid + '&password=' + password)
            	.then(response=>response.text())
            	.then(data=>{
            		console.log(data);
            		alert(data);
            		});
            //alert(`SSID: ${ssid}\nPassword: ${password}`);
        }
    </script>
</body>
</html>
  )====";
  server.send(200,"text/html",html1);
}

void sendMainUI()
{
  String html2=R"====(


<!DOCTYPE html>
<html>
<head>
    <title>Home Automation</title>
    <style>
        /* General Styling */
        body {
            margin: 0;
            padding: 0;
            font-family: Arial, sans-serif;
            background: #f4f4f9;
            color: #333;
            text-align: center;
        }



        /* Navbar */
        /* .navbar {
            background: #007BFF;
            padding: 10px 0;
            color: white;
            font-size: 1.2em;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
        } */

        /* Navbar */
.navbar {
    background: #007BFF;
    padding: 10px 0;
    color: white;
    font-size: 1.2em;
    box-shadow: 0 2px 4px rgba(0, 0, 0, 0.2);
    position: fixed;        /* Fixes it to the top */
    top: 0;                 /* Aligns to top edge */
    left: 0;
    width: 100%;            /* Full width */
    z-index: 1000;          /* Keeps it above other content */
}


        .navbar a {
            color: white;
            text-decoration: none;
            margin: 0 15px;
            font-weight: bold;
        }

        .navbar a:hover {
            text-decoration: underline;
        }

        /* Device Controls */
        .device {
            margin: 20px auto;
            padding: 15px;
            background: white;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
            max-width: 300px;
        }

        .device h2 {
            margin: 10px 0;
            color: #007BFF;
        }

        .toggle-label {
            display: inline-block;
            width: 60px;
            height: 30px;
            background: #ccc;
            border-radius: 15px;
            position: relative;
            cursor: pointer;
            transition: background 0.3s ease;
        }

        .toggle-label:before {
            content: '';
            position: absolute;
            width: 26px;
            height: 26px;
            background: white;
            border-radius: 50%;
            top: 2px;
            left: 2px;
            transition: transform 0.3s ease;
        }

        .toggle-input {
            display: none;
        }

        .toggle-input:checked + .toggle-label {
            background: #28a745;
        }

        .toggle-input:checked + .toggle-label:before {
            transform: translateX(30px);
        }

        .mode-button {
            background: #28a745;
            color: white;
            border: none;
            border-radius: 5px;
            padding: 10px 20px;
            font-size: 1em;
            cursor: pointer;
            margin-top: 20px;
            transition: background 0.3s ease;
        }

        .mode-button:hover {
            background: #218838;
        }

        /* Footer */
        /* footer {
            background: #007BFF;
            color: white;
            padding: 10px 0;
            margin-top: 20px;
            font-size: 0.9em;
            
        } */

        /* Footer - Stylish version */
footer {
    background: linear-gradient(to right, #007BFF, #00c6ff);
    color: #fff;
    padding: 15px 0;
    margin-top: 30px;
    font-size: 1em;
    font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
    text-align: center;
    letter-spacing: 0.5px;
    font-weight: 500;
    box-shadow: 0 -2px 10px rgba(0, 0, 0, 0.1);
}


        /* Status Message */
        .status-message {
            margin-top: 20px;
            font-size: 1em;
            color: #28a745;
            display: none;
        }
        /* footer p */
 


        /* contact us. */
 
.container {
    background: #ffffff;
    padding: 20px;
    border-radius: 8px;
    box-shadow: 0 0 10px rgba(0, 0, 0, 0.08);
    max-width: 400px;
    margin: 0 auto;
    text-align: left;
    border: 1px solid #007BFF;
}

.container h1 {
    font-size: 1.8em;
    color: #007BFF;
    margin-bottom: 15px;
    text-align: center;
}

.field {
    margin: 10px 0;
}

label {
    font-size: 1em;
    color: #333;
    margin-bottom: 5px;
    display: block;
}

input, textarea {
    width: 100%;
    padding: 8px;
    font-size: 0.95em;
    border-radius: 5px;
    border: 1px solid #ccc;
    box-sizing: border-box;
    margin-bottom: 10px;
    transition: border 0.3s ease;
}

input:focus, textarea:focus {
    border-color: #007BFF;
    outline: none;
}

.btn {
    display: block;
    font-size: 1em;
    color: #fff;
    background: #007BFF;
    padding: 10px;
    margin: 15px auto 0;
    border-radius: 5px;
    transition: background 0.3s;
    cursor: pointer;
    border: none;
    width: 100%;
    text-align: center;
}

.btn:hover {
    background: #0056b3;
}


    </style>
</head>
<body>
    <!-- Navbar -->
    <div class="navbar">
        <a href="#home">Home</a>
        <a href="Finaldocument.docx">Documentation</a>
        <a href="#status">Status</a>
        <a href="/selMode">Mode</a>
        <a href="#contactus">Contact</a>
    </div>

    <!-- Main Content -->
    <font size=8><p>Welcome to your device Assistant</p></font>
     <div class="sensor"><font size=5>
            <h2><b><em><u>Room Temperature and Humidity</u></em></b></h2></font>
            <b><em><font size=5 color="grey"><p id="senData">Loading...</p></font></em></b>
           <font size=4> <button class="mode-button" onclick="fetchSensorData()">Refresh Data</button></font>
        </div>
    <div id="home">
    <div class="device">
    <h2>MotionSensor</h2>
        <input type="checkbox" id="toggle4" class="toggle-input" onchange="getSen()">
        <label for="toggle4" class="toggle-label"></label>
        </div>
        <div class="device">
            <h2>Light 1</h2>
            <input type="checkbox" id="toggle1" class="toggle-input" onchange="sendRequest('light1', this.checked)">
            <label for="toggle1" class="toggle-label"></label>
        </div>

        <div class="device">
            <h2>Light 2</h2>
            <input type="checkbox" id="toggle2" class="toggle-input" onchange="sendRequest('light2', this.checked)">
            <label for="toggle2" class="toggle-label"></label>
        </div>

        <div class="device">
            <h2>Fan</h2>
            <input type="checkbox" id="toggleFan" class="toggle-input" onchange="sendRequest('fanon', this.checked)">
            <label for="toggleFan" class="toggle-label"></label>
        </div>

        <div class="device">
            <h2>Fan Speed</h2>
            <input type="range" id="fanSpeed" min="1" max="3" step="1" oninput="adjustFanSpeed(this.value)">
            <p id="speedValue">Speed: Medium</p>
        </div>
    </div>

    <!-- Status Message -->
    <div id="statusMessage" class="status-message"></div>

    <!-- Footer -->
    <footer>
       
<!-- contact us. -->
        <center>
            <div class="container" id="contactus">
            <h1>Contact Us</h1>
            <form action="https://api.web3forms.com/submit" method="POST" id="contactForm" onsubmit="return validateForm()">
                <input type="hidden" name="access_key" value="9f5089f3-b3ad-4461-bdd1-64157e938bdd">
    
                <div class="field">
                    <label for="username">Username</label>
                    <input type="text" id="username" name="username" required>
                </div>
                <div class="field">
                    <label for="email">Email</label>
                    <input type="email" id="email" name="email" required>
                </div>
                <div class="field">
                    <label for="message">Message</label>
                    <textarea id="message" name="message" rows="4" required></textarea>
                </div>
                <button class="btn" type="submit">Submit</button>
            </form>
            </div>
        </center>
        <p>&copy; 2025 Abhijit & Rohit </p>
    </footer>

    <script>

function validateForm() {
            var username = document.getElementById("username").value;
            var email = document.getElementById("email").value;
            var message = document.getElementById("message").value;

            if (username.trim() === "") {
                alert("Username is required.");
                return false;
            }
            if (email.trim() === "") {
                alert("Email is required.");
                return false;
            }
            if (!/\S+@\S+\.\S+/.test(email)) {
                alert("Please enter a valid email address.");
                return false;
            }
            if (message.trim().length < 10) {
                alert("Message must be at least 10 characters long.");
                return false;
            }
            // alert("Form submitted successfully!");
            return true;
        }

        // Send Request to ESP Server
        function sendRequest(device, state) {
            fetch(`/devs?device=${device}&state=${state ? 1 : 0}`)
                .then(response => response.text())
                .then(data => {
                    showStatusMessage(data);
                })
                .catch(error => {
                    showStatusMessage("Failed to update device!");
                });
        }

        // Adjust Fan Speed
        function adjustFanSpeed(value) {
            const speedText = { 1: "Low", 2: "Medium", 3: "High" };
            document.getElementById("speedValue").innerText = `Speed: ${speedText[value]}`;
            sendRequest('fan'+value, true);
        }


//function to print temp;humd
function fetchSensorData() {
            fetch('/thsensor')
                .then(response => response.text())
                .then(data => {
                    document.getElementById("senData").innerText = data;
                })
                .catch(error => {
                    showStatusMessage("Failed to fetch sensor data!");
                });
        }

        // Show Status Message
        function showStatusMessage(message) {
            fetch('/devState')
                .then(response=>response.text())
                .then(data=>{
                document.getElementById("statusMessage").innerText=data;
                })
        }
        
        function getSen()
        {
        	const vlu=document.getElementById("toggle4").checked;
        	if(vlu)
        	{
        	fetch(`/pirsnsr`)
        	.then(response=>response.text())
        	.then(data=>{
        		alert(data);
        		});
        	}
        }
        setInterval(getSen,8000);
        setInterval(showStatusMessage,5000);
    </script>
</body>
</html>
    

  )====";
  server.send(200,"text/html",html2);
}
//device algorithm
void handleDevs()
{
    //separate the pin number
    int k;
    if(server.hasArg("device") && server.hasArg("state"))
    {
        String device=server.arg("device");
        String state=server.arg("state");
        int st=state.toInt();

        for(int i=0;i<5;i++)
        {
            if(device==devList[i])
            {
                k=i;
                break;
            }
            else if(device=="fanon")
            {
                k=2;
                break;
            }
        }
        //light control
        if(k<2)
        {
            digitalWrite(devPins[k],(!st));
            devStates[k]=st;
        }
        //fan control
        else if(k>=2)
        {
            if(device=="fanon" && st==0)
            {
                for(int j=2;j<5;j++)
                {
                    digitalWrite(devPins[j],HIGH);
                    devStates[j]=0;
                }
                flag=0;
            }
            else if(device=="fanon" && st==1)
            {
                
                for(int j=2;j<5;j++)
                {
                    if(j!=k)
                    {
                        digitalWrite(devPins[j],HIGH);
                        devStates[j]=0;
                    }
                }
                digitalWrite(devPins[2],LOW);
                devStates[2]=1;
                flag=1;
            }
            else
            {
                if(flag==1)
                {
                    for(int j=2;j<5;j++)
                    {
                        digitalWrite(devPins[j],HIGH);
                        devStates[j]=0;
                    }
                    digitalWrite(devPins[k],(!st));
                    devStates[k]=st;
                }
                else if(flag==0)
                {
                    server.send(200,"text/plain","Fan is set to off !");
                }
            }
        }
    }
  server.send(200,"text/plain",String(devList[k])+" is active");
}
//temperature and humidity
void sendTH()
{
  float temp=dht.readTemperature();
  float hmd=dht.readHumidity();
  Serial.println(temp);
  Serial.println(hmd);
  if(isnan(temp)||isnan(hmd))
  {
    server.send(200,"text/plain","Error in sensor!");
    Serial.println("Error in sensor!");
    /*display.clearDisplay();
    display.setTextSize(2);
    display.print("DHT error!");
    display.display();*/
  }
  else
  {
    String th="Temperature: "+String(temp)+" *C || Humidity: "+String(hmd)+" %";
    server.send(200,"text/plain",th);
    Serial.println(th);
  }
  /*display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print(temp);
  display.println(" *C");
  display.print(hmd);
  display.println(" %");
  display.display();*/
}

void pirSen()
{
  if(digitalRead(16)==1)
  {
    server.send(20,"text/plain","Attention! Motion Detected!");
    /*display.clearDisplay();
    display.setCursor(0,10);
    display.setTextSize(3);
    display.print("Attention!");
    display.display();*/
  }
}

void setup() 
{
  Serial.begin(115200);
  loadSettings();
  //display initialization
  /*if(!display.begin(SSD1306_SWITCHCAPVCC,0x3C))
  {
    Serial.println("Error in display!");
  }*/
  //display.display();
  /*display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);*/
    pinMode(16,INPUT);
    if (mode == "AP") {
        startAPMode();
    } else if (mode == "Client" && !ssid.isEmpty()) {
        startClientMode();
    } else {
        // Fallback to AP mode if settings are invalid
        startAPMode();
    }
    //sendMainUI();
    for(int i=0;i<devNum;i++)
    {
      pinMode(devPins[i],OUTPUT);
      digitalWrite(devPins[i],HIGH);
    }
}

void loop() {
    server.handleClient();
   if(mode=="Client")
    {
        SinricPro.handle();
    }
}