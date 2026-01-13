#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPS++.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <RTClib.h>
#include <GyverINA.h>
#include <WiFi.h>
#include <WebServer.h>

// ==================== KONFIGURASI PIN ====================
#define OLED_SDA 8
#define OLED_SCL 9
#define GPS_RX 20
#define GPS_TX 21
#define ONE_WIRE_BUS 2
#define TTP223_PIN 10
#define BUZZER_PIN 3

// ==================== KONFIGURASI WiFi ====================
const char* ssid = "Smart_dash";
const char* password = "12345678";
const int wifiChannel = 6; // Gunakan channel 1, 6, atau 11 yang tidak overlapping

// ==================== INISIALISASI MODUL ====================
Adafruit_SSD1306 display(128, 64, &Wire, -1);
TinyGPSPlus gps;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
RTC_DS1307 rtc;
INA226 ina;
WebServer server(80);

// ==================== VARIABEL GLOBAL ====================
int currentMode = 0;
const int TOTAL_MODES = 9;
bool wifiActive = false;

// Sensor variables
float currentSpeed = 0;
float currentVoltage = 0;
float currentTemp = 0;
bool tempSensorFound = false;
bool gpsConnected = false;

// Stats variables
float maxSpeed = 0;
float totalSpeed = 0;
int speedReadings = 0;
float maxVoltage = 0;
float minVoltage = 100;
float maxTemp = -100;
float minTemp = 200;
float totalTemp = 0;
int tempReadings = 0;

// Setting variables
int blinkSpeed = 2000;
int buzzerVolume = 50;
int moveSpeed = 2;

// Button variables
unsigned long lastButtonPress = 0;
bool buttonPressed = false;

// Timing variables
unsigned long lastSensorUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastStatsUpdate = 0;

// ==================== FUNGSI BANTUAN ====================
void centerText(const String &text, int y, int textSize = 1) {
  display.setTextSize(textSize);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int x = (128 - w) / 2;
  display.setCursor(x, y);
  display.print(text);
}

void beep(int count) {
  for(int i = 0; i < count; i++) {
    analogWrite(BUZZER_PIN, buzzerVolume);
    delay(80);
    analogWrite(BUZZER_PIN, 0);
    if(i < count-1) delay(80);
  }
}

// ==================== WiFi & WEB SERVER ====================
void setupWiFi() {
  Serial.println("📡 Setting up WiFi Access Point...");
  
  // Konfigurasi WiFi yang lebih optimal
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  
  // Setup WiFi dengan parameter optimal
  bool result = WiFi.softAP(ssid, password, wifiChannel, 0, 4);
  // Parameter: (ssid, password, channel, hidden, max_connection)
  
  if(!result) {
    Serial.println("❌ WiFi AP failed!");
    return;
  }
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("📍 AP IP address: ");
  Serial.println(IP);
  Serial.print("📶 WiFi Channel: ");
  Serial.println(wifiChannel);
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/mode", handleMode);
  server.on("/data", handleData);
  server.on("/buzzer", handleBuzzer);
  server.on("/settime", handleSetTime);
  server.on("/setdate", handleSetDate);
  server.on("/setvolume", handleSetVolume);
  server.on("/setblink", handleSetBlink);
  server.on("/setmove", handleSetMove);
  server.on("/getsettings", handleGetSettings);
  
  server.begin();
  Serial.println("✅ HTTP server started");
  wifiActive = true;
}

void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Car Dashboard Control</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * {
            box-sizing: border-box;
            margin: 0;
            padding: 0;
        }
        
        body { 
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
            background: linear-gradient(135deg, #1a1a2e 0%, #16213e 100%); 
            color: #ffffff; 
            margin: 0; 
            padding: 20px;
            min-height: 100vh;
        }
        
        .container { 
            max-width: 450px; 
            margin: 0 auto; 
        }
        
        .header {
            text-align: center;
            margin-bottom: 20px;
            padding: 15px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.2);
        }
        
        .header h1 {
            font-size: 24px;
            margin-bottom: 5px;
            background: linear-gradient(45deg, #4CAF50, #2196F3);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
        }
        
        .card { 
            background: rgba(255, 255, 255, 0.1); 
            padding: 20px; 
            margin: 15px 0; 
            border-radius: 15px;
            text-align: center;
            backdrop-filter: blur(10px);
            border: 1px solid rgba(255, 255, 255, 0.2);
            transition: transform 0.3s ease, box-shadow 0.3s ease;
        }
        
        .card:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 25px rgba(0, 0, 0, 0.3);
        }
        
        .btn { 
            background: linear-gradient(45deg, #4CAF50, #45a049); 
            border: none; 
            color: white; 
            padding: 12px 16px; 
            margin: 6px; 
            border-radius: 8px; 
            cursor: pointer;
            font-size: 14px;
            font-weight: 600;
            transition: all 0.3s ease;
            box-shadow: 0 4px 15px rgba(76, 175, 80, 0.3);
        }
        
        .btn:hover { 
            transform: translateY(-2px);
            box-shadow: 0 6px 20px rgba(76, 175, 80, 0.4);
        }
        
        .btn:active {
            transform: translateY(0);
        }
        
        .btn-buzzer { 
            background: linear-gradient(45deg, #f44336, #da190b);
            box-shadow: 0 4px 15px rgba(244, 67, 54, 0.3);
        }
        
        .btn-buzzer:hover { 
            box-shadow: 0 6px 20px rgba(244, 67, 54, 0.4);
        }
        
        .btn-save { 
            background: linear-gradient(45deg, #2196F3, #0b7dda);
            box-shadow: 0 4px 15px rgba(33, 150, 243, 0.3);
        }
        
        .btn-save:hover { 
            box-shadow: 0 6px 20px rgba(33, 150, 243, 0.4);
        }
        
        .status { 
            background: rgba(0, 0, 0, 0.3); 
            padding: 12px; 
            border-radius: 10px; 
            margin: 10px 0;
            font-size: 14px;
            border-left: 4px solid #4CAF50;
        }
        
        .mode-btn { 
            width: 85px; 
            height: 65px; 
            font-size: 12px;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            gap: 5px;
        }
        
        .row { 
            display: flex; 
            justify-content: center; 
            flex-wrap: wrap; 
            gap: 8px;
            margin: 10px 0;
        }
        
        .setting-group { 
            margin: 15px 0; 
            text-align: left;
        }
        
        .setting-label { 
            display: block; 
            margin: 8px 0 5px 0; 
            font-weight: 600;
            color: #e0e0e0;
        }
        
        .setting-input { 
            width: 100%; 
            padding: 10px; 
            margin: 5px 0; 
            border-radius: 8px; 
            border: 1px solid rgba(255, 255, 255, 0.3);
            background: rgba(0, 0, 0, 0.3);
            color: white;
            font-size: 14px;
        }
        
        .setting-input:focus {
            outline: none;
            border-color: #2196F3;
            box-shadow: 0 0 0 2px rgba(33, 150, 243, 0.2);
        }
        
        .setting-value { 
            background: rgba(255, 255, 255, 0.2); 
            padding: 6px 12px; 
            border-radius: 6px; 
            margin: 0 5px;
            font-weight: 600;
        }
        
        .sensor-data {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 10px;
            margin-top: 10px;
        }
        
        .sensor-item {
            background: rgba(255, 255, 255, 0.1);
            padding: 12px;
            border-radius: 10px;
            text-align: center;
        }
        
        .sensor-value {
            font-size: 18px;
            font-weight: bold;
            margin: 5px 0;
            color: #4CAF50;
        }
        
        .sensor-label {
            font-size: 12px;
            color: #b0b0b0;
        }
        
        @keyframes fadeIn {
            from { opacity: 0; transform: translateY(10px); }
            to { opacity: 1; transform: translateY(0); }
        }
        
        .pulse {
            animation: pulse 2s infinite;
        }
        
        @keyframes pulse {
            0% { opacity: 1; }
            50% { opacity: 0.7; }
            100% { opacity: 1; }
        }
        
        /* Responsive design */
        @media (max-width: 480px) {
            .container {
                max-width: 100%;
                padding: 10px;
            }
            
            .card {
                padding: 15px;
                margin: 10px 0;
            }
            
            .mode-btn {
                width: 75px;
                height: 60px;
                font-size: 11px;
            }
            
            .sensor-data {
                grid-template-columns: 1fr;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🚗 CAR DASHBOARD CONTROL</h1>
            <div class="status" id="status">✅ Connected to Car Dashboard</div>
        </div>

        <div class="card">
            <h3 style="margin-bottom: 15px;">📱 DISPLAY MODES</h3>
            <div class="row">
                <button class="btn mode-btn" onclick="setMode(0)">
                    <span>👀</span>Eyes
                </button>
                <button class="btn mode-btn" onclick="setMode(1)">
                    <span>🚀</span>Speed
                </button>
                <button class="btn mode-btn" onclick="setMode(3)">
                    <span>🔋</span>Voltage
                </button>
            </div>
            <div class="row">
                <button class="btn mode-btn" onclick="setMode(5)">
                    <span>🌡️</span>Temp
                </button>
                <button class="btn mode-btn" onclick="setMode(7)">
                    <span>⏰</span>Clock
                </button>
                <button class="btn mode-btn" onclick="setMode(8)">
                    <span>📊</span>All Data
                </button>
            </div>

            <div style="margin-top: 20px;">
                <button class="btn btn-buzzer" onclick="testBuzzer()">
                    <span>🔊</span> Test Buzzer
                </button>
            </div>
        </div>

        <div class="card">
            <h3 style="margin-bottom: 20px;">⚙️ SYSTEM SETTINGS</h3>
            
            <div class="setting-group">
                <label class="setting-label">🕐 Set Device Time</label>
                <input type="time" id="timeInput" class="setting-input">
                <button class="btn btn-save" onclick="setTime()" style="width: 100%; margin-top: 10px;">
                    <span>💾</span> Set Time
                </button>
            </div>

            <div class="setting-group">
                <label class="setting-label">📅 Set Device Date</label>
                <input type="date" id="dateInput" class="setting-input">
                <button class="btn btn-save" onclick="setDate()" style="width: 100%; margin-top: 10px;">
                    <span>💾</span> Set Date
                </button>
            </div>

            <div class="setting-group">
                <label class="setting-label">🔊 Buzzer Volume: <span id="volumeValue" class="setting-value">50</span>%</label>
                <input type="range" id="volumeSlider" min="0" max="100" value="50" class="setting-input" oninput="updateVolume(this.value)">
                <button class="btn btn-save" onclick="setVolume()" style="width: 100%; margin-top: 5px;">
                    <span>💾</span> Set Volume
                </button>
            </div>

            <div class="setting-group">
                <label class="setting-label">👀 Eye Blink Speed: <span id="blinkValue" class="setting-value">2000</span>ms</label>
                <input type="range" id="blinkSlider" min="500" max="5000" step="100" value="2000" class="setting-input" oninput="updateBlink(this.value)">
                <button class="btn btn-save" onclick="setBlink()" style="width: 100%; margin-top: 5px;">
                    <span>💾</span> Set Blink Speed
                </button>
            </div>

            <div class="setting-group">
                <label class="setting-label">👁️ Eye Move Speed: <span id="moveValue" class="setting-value">2</span></label>
                <input type="range" id="moveSlider" min="1" max="5" value="2" class="setting-input" oninput="updateMove(this.value)">
                <button class="btn btn-save" onclick="setMove()" style="width: 100%; margin-top: 5px;">
                    <span>💾</span> Set Move Speed
                </button>
            </div>

            <div class="setting-group">
                <h4 style="margin: 20px 0 10px 0; text-align: center;">Current Settings</h4>
                <div id="currentSettings" class="status">Loading settings...</div>
            </div>
        </div>

        <div class="card">
            <h3 style="margin-bottom: 15px;">📊 LIVE SENSOR DATA</h3>
            <div class="sensor-data" id="sensorData">
                <div class="sensor-item">
                    <div class="sensor-label">🚀 SPEED</div>
                    <div class="sensor-value" id="speedValue">-- km/h</div>
                </div>
                <div class="sensor-item">
                    <div class="sensor-label">🔋 VOLTAGE</div>
                    <div class="sensor-value" id="voltageValue">-- V</div>
                </div>
                <div class="sensor-item">
                    <div class="sensor-label">🌡️ TEMPERATURE</div>
                    <div class="sensor-value" id="tempValue">-- C</div>
                </div>
                <div class="sensor-item">
                    <div class="sensor-label">🛰️ GPS STATUS</div>
                    <div class="sensor-value" id="gpsValue">--</div>
                </div>
            </div>
        </div>
    </div>

    <script>
        let currentVolume = 50;
        let currentBlink = 2000;
        let currentMove = 2;

        function setMode(mode) {
            fetch('/mode?value=' + mode)
                .then(response => response.text())
                .then(data => {
                    updateStatus('Mode changed to: ' + data);
                    event.target.classList.add('pulse');
                    setTimeout(() => event.target.classList.remove('pulse'), 1000);
                })
                .catch(err => updateStatus('Error: ' + err));
        }

        function testBuzzer() {
            fetch('/buzzer')
                .then(response => response.text())
                .then(data => {
                    updateStatus('Buzzer: ' + data);
                    event.target.classList.add('pulse');
                    setTimeout(() => event.target.classList.remove('pulse'), 1000);
                })
                .catch(err => updateStatus('Error: ' + err));
        }

        function setTime() {
            const timeValue = document.getElementById('timeInput').value;
            if (timeValue) {
                fetch('/settime?value=' + timeValue)
                    .then(response => response.text())
                    .then(data => {
                        updateStatus('Time set to: ' + data);
                        document.getElementById('timeInput').value = '';
                    })
                    .catch(err => updateStatus('Error: ' + err));
            } else {
                updateStatus('Please select a time first');
            }
        }

        function setDate() {
            const dateValue = document.getElementById('dateInput').value;
            if (dateValue) {
                fetch('/setdate?value=' + dateValue)
                    .then(response => response.text())
                    .then(data => {
                        updateStatus('Date set to: ' + data);
                        document.getElementById('dateInput').value = '';
                    })
                    .catch(err => updateStatus('Error: ' + err));
            } else {
                updateStatus('Please select a date first');
            }
        }

        function updateVolume(value) {
            currentVolume = value;
            document.getElementById('volumeValue').textContent = value;
        }

        function setVolume() {
            fetch('/setvolume?value=' + currentVolume)
                .then(response => response.text())
                .then(data => {
                    updateStatus('Volume set to: ' + data + '%');
                    event.target.classList.add('pulse');
                    setTimeout(() => event.target.classList.remove('pulse'), 1000);
                })
                .catch(err => updateStatus('Error: ' + err));
        }

        function updateBlink(value) {
            currentBlink = value;
            document.getElementById('blinkValue').textContent = value;
        }

        function setBlink() {
            fetch('/setblink?value=' + currentBlink)
                .then(response => response.text())
                .then(data => {
                    updateStatus('Blink speed set to: ' + data);
                    event.target.classList.add('pulse');
                    setTimeout(() => event.target.classList.remove('pulse'), 1000);
                })
                .catch(err => updateStatus('Error: ' + err));
        }

        function updateMove(value) {
            currentMove = value;
            document.getElementById('moveValue').textContent = value;
        }

        function setMove() {
            fetch('/setmove?value=' + currentMove)
                .then(response => response.text())
                .then(data => {
                    updateStatus('Move speed set to: ' + data);
                    event.target.classList.add('pulse');
                    setTimeout(() => event.target.classList.remove('pulse'), 1000);
                })
                .catch(err => updateStatus('Error: ' + err));
        }

        function updateStatus(message) {
            const statusElement = document.getElementById('status');
            statusElement.innerHTML = message;
            statusElement.style.borderLeftColor = '#4CAF50';
            setTimeout(() => {
                statusElement.style.borderLeftColor = '#4CAF50';
            }, 2000);
        }

        function updateSensorData() {
            fetch('/data')
                .then(response => response.text())
                .then(data => {
                    const sensors = data.split('|');
                    const sensorObj = {};
                    
                    sensors.forEach(sensor => {
                        const parts = sensor.split(':');
                        if (parts.length >= 2) {
                            const key = parts[0].trim();
                            const value = parts.slice(1).join(':').trim();
                            sensorObj[key] = value;
                        }
                    });
                    
                    if (sensorObj.Speed) {
                        document.getElementById('speedValue').textContent = sensorObj.Speed;
                    }
                    if (sensorObj.Voltage) {
                        document.getElementById('voltageValue').textContent = sensorObj.Voltage;
                    }
                    if (sensorObj.Temp) {
                        document.getElementById('tempValue').textContent = sensorObj.Temp;
                    }
                    if (sensorObj.GPS) {
                        const gpsElement = document.getElementById('gpsValue');
                        gpsElement.textContent = sensorObj.GPS;
                        gpsElement.style.color = sensorObj.GPS.includes('Connected') ? '#4CAF50' : '#f44336';
                    }
                    
                    document.querySelectorAll('.sensor-value').forEach(element => {
                        element.classList.add('pulse');
                        setTimeout(() => element.classList.remove('pulse'), 1000);
                    });
                })
                .catch(err => {
                    console.error('Error updating sensor data:', err);
                    document.getElementById('speedValue').textContent = 'Error';
                    document.getElementById('voltageValue').textContent = 'Error';
                    document.getElementById('tempValue').textContent = 'Error';
                    document.getElementById('gpsValue').textContent = 'Error';
                });
        }

        function updateSettings() {
            fetch('/getsettings')
                .then(response => response.json())
                .then(data => {
                    const settingsHtml = `
                        <div style="text-align: left; line-height: 1.8;">
                            <div>🔊 Volume: <strong>${data.volume}%</strong></div>
                            <div>👀 Blink: <strong>${data.blink}ms</strong></div>
                            <div>👁️ Move: <strong>${data.move}</strong></div>
                        </div>
                    `;
                    document.getElementById('currentSettings').innerHTML = settingsHtml;
                })
                .catch(err => {
                    document.getElementById('currentSettings').innerHTML = 'Error loading settings';
                });
        }

        setInterval(updateSensorData, 2000);
        setInterval(updateSettings, 3000);
        
        updateSensorData();
        updateSettings();
        
        fetch('/getsettings')
            .then(response => response.json())
            .then(data => {
                currentVolume = data.volume;
                currentBlink = data.blink;
                currentMove = data.move;
                
                document.getElementById('volumeSlider').value = currentVolume;
                document.getElementById('volumeValue').textContent = currentVolume;
                document.getElementById('blinkSlider').value = currentBlink;
                document.getElementById('blinkValue').textContent = currentBlink;
                document.getElementById('moveSlider').value = currentMove;
                document.getElementById('moveValue').textContent = currentMove;
            })
            .catch(err => console.error('Error loading settings:', err));

        document.addEventListener('DOMContentLoaded', function() {
            document.body.style.opacity = '0';
            document.body.style.transition = 'opacity 0.5s ease';
            setTimeout(() => {
                document.body.style.opacity = '1';
            }, 100);
        });
    </script>
</body>
</html>
)rawliteral";
  server.send(200, "text/html", html);
}

void handleMode() {
  if (server.hasArg("value")) {
    currentMode = server.arg("value").toInt();
    if (currentMode >= TOTAL_MODES) currentMode = 0;
    String modeNames[] = {"Eyes", "Speed", "Speed Stats", "Voltage", "Voltage Stats", 
                         "Temperature", "Temp Stats", "Clock", "All Sensors"};
    server.send(200, "text/plain", modeNames[currentMode]);
    beep(1);
  }
}

void handleData() {
  String data = "";
  data += "Speed:" + String(currentSpeed, 1) + " km/h|";
  data += "Voltage:" + String(currentVoltage, 1) + " V|";
  data += "Temp:" + String(currentTemp, 1) + " C|";
  data += "GPS:" + String(gpsConnected ? "Connected" : "No Signal") + "|";
  data += "Mode:" + String(currentMode);
  
  server.send(200, "text/plain", data);
}

void handleBuzzer() {
  beep(3);
  server.send(200, "text/plain", "Beeped 3 times");
}

void handleSetTime() {
  if (server.hasArg("value")) {
    String timeStr = server.arg("value");
    int hour = timeStr.substring(0, 2).toInt();
    int minute = timeStr.substring(3, 5).toInt();
    
    DateTime now = rtc.now();
    rtc.adjust(DateTime(now.year(), now.month(), now.day(), hour, minute, 0));
    
    server.send(200, "text/plain", String(hour) + ":" + (minute < 10 ? "0" : "") + String(minute));
    beep(1);
  }
}

void handleSetDate() {
  if (server.hasArg("value")) {
    String dateStr = server.arg("value");
    int year = dateStr.substring(0, 4).toInt();
    int month = dateStr.substring(5, 7).toInt();
    int day = dateStr.substring(8, 10).toInt();
    
    DateTime now = rtc.now();
    rtc.adjust(DateTime(year, month, day, now.hour(), now.minute(), now.second()));
    
    server.send(200, "text/plain", String(day) + "/" + String(month) + "/" + String(year));
    beep(1);
  }
}

void handleSetVolume() {
  if (server.hasArg("value")) {
    buzzerVolume = server.arg("value").toInt();
    server.send(200, "text/plain", String(buzzerVolume));
    beep(1);
  }
}

void handleSetBlink() {
  if (server.hasArg("value")) {
    blinkSpeed = server.arg("value").toInt();
    server.send(200, "text/plain", String(blinkSpeed) + "ms");
    beep(1);
  }
}

void handleSetMove() {
  if (server.hasArg("value")) {
    moveSpeed = server.arg("value").toInt();
    server.send(200, "text/plain", String(moveSpeed));
    beep(1);
  }
}

void handleGetSettings() {
  String json = "{";
  json += "\"volume\":" + String(buzzerVolume) + ",";
  json += "\"blink\":" + String(blinkSpeed) + ",";
  json += "\"move\":" + String(moveSpeed);
  json += "}";
  
  server.send(200, "application/json", json);
}

// ==================== ANIMASI MATA & DISPLAY ====================
int leftEyeX = 40;
int rightEyeX = 80;
int eyeY = 18;
int eyeWidth = 25;
int eyeHeight = 30;
int targetLeftEyeX = leftEyeX;
int targetRightEyeX = rightEyeX;
int blinkState = 0;
unsigned long lastBlinkTime = 0;
unsigned long moveTime = 0;

void updateEyes() {
  unsigned long currentTime = millis();

  if (currentTime - lastBlinkTime > blinkSpeed && blinkState == 0) {
    blinkState = 1;
    lastBlinkTime = currentTime;
  } 
  else if (currentTime - lastBlinkTime > 150 && blinkState == 1) {
    blinkState = 0;
    lastBlinkTime = currentTime;
  }

  if (currentTime - moveTime > random(2000, 5000) && blinkState == 0) {
    int eyeMovement = random(0, 3);
    if (eyeMovement == 1) {
      targetLeftEyeX = 30;
      targetRightEyeX = 70;
    } else if (eyeMovement == 2) {
      targetLeftEyeX = 50;
      targetRightEyeX = 90;
    } else {
      targetLeftEyeX = 40;
      targetRightEyeX = 80;
    }
    moveTime = currentTime;
  }

  if (leftEyeX != targetLeftEyeX) {
    leftEyeX += (targetLeftEyeX - leftEyeX) / moveSpeed;
  }
  if (rightEyeX != targetRightEyeX) {
    rightEyeX += (targetRightEyeX - rightEyeX) / moveSpeed;
  }
}

// ==================== FUNGSI DISPLAY STATS ====================
void displaySpeedStats() {
  // Judul "SPEED STATS"
  display.setTextSize(1);
  centerText("SPEED STATS", 5, 1);
  
  // Update stats
  if (currentSpeed > maxSpeed) maxSpeed = currentSpeed;
  if (gpsConnected && gps.speed.isValid()) {
    totalSpeed += currentSpeed;
    speedReadings++;
  }
  
  float avgSpeed = (speedReadings > 0) ? totalSpeed / speedReadings : 0;
  
  // Current speed - size 1
  String currentText = "NOW: " + String(currentSpeed, 1) + " km/h";
  centerText(currentText, 18, 1);
  
  // Stats detail - semua size 1
  String maxText = "MAX: " + String(maxSpeed, 1) + " km/h";
  String avgText = "AVG: " + String(avgSpeed, 1) + " km/h";
  
  centerText(maxText, 30, 1);
  centerText(avgText, 40, 1);
  
  if (speedReadings > 0) {
    centerText("SAMPLES: " + String(speedReadings), 50, 1);
  }
}

void displayVoltageStats() {
  // Judul "VOLTAGE STATS"
  display.setTextSize(1);
  centerText("VOLTAGE STATS", 5, 1);
  
  // Update stats
  if (currentVoltage > maxVoltage) maxVoltage = currentVoltage;
  if (currentVoltage > 0 && currentVoltage < minVoltage) minVoltage = currentVoltage;
  
  // Current voltage - size 1
  String currentText = "NOW: " + String(currentVoltage, 1) + " V";
  centerText(currentText, 18, 1);
  
  // Stats detail - semua size 1
  String maxText = "MAX: " + String(maxVoltage, 1) + " V";
  String minText = "MIN: " + String(minVoltage, 1) + " V";
  
  centerText(maxText, 30, 1);
  centerText(minText, 40, 1);
  centerText("STABLE POWER", 50, 1);
}

void displayTempStats() {
  // Judul "TEMP STATS"
  display.setTextSize(1);
  centerText("TEMP STATS", 5, 1);
  
  // Update stats
  if (tempSensorFound && currentTemp > -100) {
    if (currentTemp > maxTemp) maxTemp = currentTemp;
    if (currentTemp < minTemp) minTemp = currentTemp;
    totalTemp += currentTemp;
    tempReadings++;
  }
  
  float avgTemp = (tempReadings > 0) ? totalTemp / tempReadings : 0;
  
  // Gunakan huruf 'C' saja tanpa simbol derajat
  String currentText = "CURRENT: " + String(currentTemp, 1) + "C";
  centerText(currentText, 18, 1);
  
  // Stats detail
  String maxText = "MAXIMUM: " + String(maxTemp, 1) + "C";
  String minText = "MINIMUM: " + String(minTemp, 1) + "C";
  String avgText = "AVERAGE: " + String(avgTemp, 1) + "C";
  
  centerText(maxText, 30, 1);
  centerText(minText, 40, 1);
  centerText(avgText, 50, 1);
}

// ==================== FUNGSI DISPLAY UTAMA ====================
void displaySpeed() {
  // Judul "SPEED" kecil di atas
  display.setTextSize(1);
  centerText("SPEED", 5, 1);
  
  // Angka kecepatan besar - segment style
  display.setTextSize(4);
  String speedText = !gpsConnected ? "---" : (gps.speed.isValid() ? String(currentSpeed, 1) : "0.0");
  
  // Hitung posisi tengah
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(speedText, 0, 0, &x1, &y1, &w, &h);
  int xSpeed = (128 - w) / 2;
  
  // Tampilkan angka segment style
  display.setCursor(xSpeed, 20);
  display.print(speedText);
  
  // Satuan "km/h" kecil di bawah
  display.setTextSize(1);
  String unitText = "km/h";
  display.getTextBounds(unitText, 0, 0, &x1, &y1, &w, &h);
  int xUnit = (128 - w) / 2;
  display.setCursor(xUnit, 52);
  display.print(unitText);
}

void displayVoltage() {
  // Judul "VOLTAGE"
  display.setTextSize(1);
  centerText("VOLTAGE", 5, 1);
  
  // Angka voltage besar - segment style
  display.setTextSize(4);
  String voltageText = currentVoltage > 0 ? String(currentVoltage, 1) : "--.-";
  
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(voltageText, 0, 0, &x1, &y1, &w, &h);
  int xVolt = (128 - w) / 2;
  
  display.setCursor(xVolt, 20);
  display.print(voltageText);
  
  // Satuan
  display.setTextSize(1);
  centerText("VOLTS", 52, 1);
}

void displayTemperature() {
  // Judul "TEMPERATURE"
  display.setTextSize(1);
  centerText("TEMPERATURE", 5, 1);
  
  // Angka temperature besar
  display.setTextSize(4);
  String tempText = !tempSensorFound ? "--.-" : String(currentTemp, 1);
  
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(tempText, 0, 0, &x1, &y1, &w, &h);
  int xTemp = (128 - w) / 2;
  
  display.setCursor(xTemp, 20);
  display.print(tempText);
  
  // Satuan sederhana
  display.setTextSize(1);
  String statusText = !tempSensorFound ? "NO SENSOR" : "DEGREES C";
  centerText(statusText, 52, 1);
}

void displayClock() {
  // Judul "CLOCK"
  display.setTextSize(1);
  centerText("CLOCK", 5, 1);
  
  // Waktu dengan efek blink colon - segment style
  static bool colonVisible = true;
  static unsigned long lastColonUpdate = 0;
  if (millis() - lastColonUpdate > 500) {
    colonVisible = !colonVisible;
    lastColonUpdate = millis();
  }
  
  DateTime now = rtc.now();
  display.setTextSize(3);
  String timeText = (now.hour() < 10 ? "0" : "") + String(now.hour()) + 
                   (colonVisible ? ":" : " ") + 
                   (now.minute() < 10 ? "0" : "") + String(now.minute());
  
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(timeText, 0, 0, &x1, &y1, &w, &h);
  int xTime = (128 - w) / 2;
  
  display.setCursor(xTime, 22);
  display.print(timeText);
  
  // Tanggal kecil di bawah
  display.setTextSize(1);
  String dateText = String(now.day()) + "/" + String(now.month()) + "/" + String(now.year());
  centerText(dateText, 52, 1);
}

void displayAllSensors() {
  // Judul "ALL SENSORS"
  display.setTextSize(1);
  centerText("ALL SENSORS", 5, 1);
  
  display.setTextSize(1);
  
  // Data sensors
  String speedText = "SPEED: " + (!gpsConnected ? "NO GPS" : String(currentSpeed, 1) + " km/h");
  String voltageText = "VOLTAGE: " + (currentVoltage > 0 ? String(currentVoltage, 1) + " V" : "ERROR");
  String tempText = "TEMP: " + (!tempSensorFound ? "NO SENSOR" : String(currentTemp, 1) + " C");
  
  // Tampilkan dengan spacing yang rapi
  centerText(speedText, 18, 1);
  centerText(voltageText, 28, 1);
  centerText(tempText, 38, 1);
  
  // Waktu dan GPS status
  DateTime now = rtc.now();
  String timeText = "TIME: " + String(now.hour()) + ":" + (now.minute() < 10 ? "0" : "") + String(now.minute());
  String gpsText = "GPS: " + String(gpsConnected ? "CONNECTED" : "NO SIGNAL");
  
  centerText(timeText, 48, 1);
  centerText(gpsText, 58, 1);
}

void displayEyes() {
  updateEyes();
  
  if (blinkState == 0) {
    display.fillRoundRect(leftEyeX, eyeY, eyeWidth, eyeHeight, 5, SSD1306_WHITE);
    display.fillRoundRect(rightEyeX, eyeY, eyeWidth, eyeHeight, 5, SSD1306_WHITE);
  } else {
    display.fillRect(leftEyeX, eyeY + eyeHeight / 2 - 2, eyeWidth, 4, SSD1306_WHITE);
    display.fillRect(rightEyeX, eyeY + eyeHeight / 2 - 2, eyeWidth, 4, SSD1306_WHITE);
  }
}

void updateDisplay() {
  display.clearDisplay();
  
  switch(currentMode) {
    case 0: 
      displayEyes(); 
      break;
    case 1: 
      displaySpeed(); 
      break;
    case 2: 
      displaySpeedStats(); 
      break;
    case 3: 
      displayVoltage(); 
      break;
    case 4: 
      displayVoltageStats(); 
      break;
    case 5: 
      displayTemperature(); 
      break;
    case 6: 
      displayTempStats(); 
      break;
    case 7: 
      displayClock(); 
      break;
    case 8: 
      displayAllSensors(); 
      break;
    default: 
      displaySpeed(); 
      break;
  }
  
  display.display();
}

// ==================== SETUP & MAIN LOOP ====================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=======================================");
  Serial.println("        DASHBOARD KENDARAAN PRO");
  Serial.println("=======================================");
  
  // Initialize hardware
  Wire.begin(OLED_SDA, OLED_SCL);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED initialization failed!");
    while(1) delay(1000);
  }
  
  // Initialize sensors
  sensors.begin();
  tempSensorFound = (sensors.getDeviceCount() > 0);
  rtc.begin();
  ina.begin(0.1);
  Serial1.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  
  pinMode(TTP223_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Setup WiFi
  setupWiFi();
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Start dengan mode 0
  currentMode = 0;
  beep(2);
  
  Serial.println("✅ SYSTEM READY!");
  Serial.println("📱 Connect to WiFi: " + String(ssid));
  Serial.println("🌐 Open browser to: " + WiFi.softAPIP().toString());
  
  // Initialize timing variables
  lastSensorUpdate = millis();
  lastDisplayUpdate = millis();
  lastStatsUpdate = millis();
}

void handleButton() {
  static bool lastButtonState = HIGH;
  bool currentButtonState = digitalRead(TTP223_PIN);
  
  // Deteksi rising edge (tombol dilepas setelah ditekan)
  if (lastButtonState == LOW && currentButtonState == HIGH) {
    if (millis() - lastButtonPress > 300) { // Debounce 300ms
      currentMode = (currentMode + 1) % TOTAL_MODES;
      beep(1);
      lastButtonPress = millis();
      updateDisplay(); // Force update display immediately
    }
  }
  
  lastButtonState = currentButtonState;
}

void updateSensors() {
  // Baca data GPS
  while (Serial1.available() > 0) {
    gps.encode(Serial1.read());
  }
  
  // Update GPS connection status setiap 5 detik
  if (millis() - lastStatsUpdate > 5000) {
    gpsConnected = (gps.satellites.isValid() && gps.satellites.value() > 0);
    lastStatsUpdate = millis();
  }
  
  // Update speed
  if (gpsConnected && gps.speed.isValid()) {
    currentSpeed = gps.speed.kmph();
  } else {
    currentSpeed = 0.0;
  }
  
  // Update temperature
  if (tempSensorFound) {
    sensors.requestTemperatures();
    currentTemp = sensors.getTempCByIndex(0);
    if (currentTemp == DEVICE_DISCONNECTED_C) currentTemp = -100;
  } else {
    currentTemp = -100;
  }
  
  // Update voltage
  currentVoltage = ina.getVoltage();
  
  // Debug output
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 5000) {
    Serial.println("=== SENSOR DATA ===");
    Serial.print("Speed: "); Serial.println(currentSpeed);
    Serial.print("Voltage: "); Serial.println(currentVoltage);
    Serial.print("Temp: "); Serial.println(currentTemp);
    Serial.print("GPS: "); Serial.println(gpsConnected ? "Connected" : "No Signal");
    Serial.println("===================");
    lastDebug = millis();
  }
}

void loop() {
  // Handle web server requests (NON-BLOCKING)
  server.handleClient();
  
  // Handle button (NON-BLOCKING)
  handleButton();
  
  // Update sensors setiap 1 detik
  if (millis() - lastSensorUpdate > 1000) {
    updateSensors();
    lastSensorUpdate = millis();
  }
  
  // Update display setiap 200ms (5 FPS)
  if (millis() - lastDisplayUpdate > 200) {
    updateDisplay();
    lastDisplayUpdate = millis();
  }
  
  // Small delay to prevent watchdog timer issues
  delay(10);
}
