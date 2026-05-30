#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>   

// WiFi credentials
const char* ssid = "Nothing";
const char* password = "khuljasimsim";

WebServer server(80);

// Motor pins
#define WATER_PUMP 19
#define PUMP1 2
#define PUMP2 4

// ===== L298N MOTOR DRIVER PINS =====
#define LEFT_MOTOR_IN1 25
#define LEFT_MOTOR_IN2 33
// #define LEFT_MOTOR_ENA 14
#define RIGHT_MOTOR_IN3 27
#define RIGHT_MOTOR_IN4 26
// #define RIGHT_MOTOR_ENB 12

// ===== ULTRASONIC SENSOR PINS =====
#define TRIG_PIN 5
#define ECHO_PIN 18

// ===== SERVO MOTOR PINS =====
#define SERVO_PEST1 13    // Servo for Pesticide 1
#define SERVO_PEST2 12    // Servo for Pesticide 2
#define SERVO_WATER 14    // Servo for Water

// Motor speed variables
int leftMotorSpeed = 0;
int rightMotorSpeed = 0;
String currentMode = "stop";
String command = "";

// Servo objects
Servo servoPest1;
Servo servoPest2;
Servo servoWater;

// Ultrasonic variables
unsigned long lastUltrasonicRead = 0;
const unsigned long ultrasonicInterval = 200; // Read every 200ms
float distance = 0;
bool plantDetected = false;
unsigned long plantDetectionTime = 0;

// Servo positions
const int SERVO_CLOSED = 90;  // Changed to 90 degrees as center position
const int SERVO_OPEN = 180;
const int SERVO_SPRAY = 180;

void setup() {
  Serial.begin(115200);
  delay(100);

  Serial.println("Starting Mecanum Wheel Robot with Sensors...");

  // Configure motor pins
  pinMode(LEFT_MOTOR_IN1, OUTPUT);
  pinMode(LEFT_MOTOR_IN2, OUTPUT);
  // pinMode(LEFT_MOTOR_ENA, OUTPUT);
  pinMode(RIGHT_MOTOR_IN3, OUTPUT);
  pinMode(RIGHT_MOTOR_IN4, OUTPUT);
  // pinMode(RIGHT_MOTOR_ENB, OUTPUT);
  stopMotors();

  // Configure pump pins
  pinMode(WATER_PUMP, OUTPUT);
  pinMode(PUMP1, OUTPUT);
  pinMode(PUMP2, OUTPUT);
  digitalWrite(WATER_PUMP, LOW);
  digitalWrite(PUMP1, LOW);
  digitalWrite(PUMP2, LOW);

  // Configure ultrasonic pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  // Attach servos
  servoPest1.attach(SERVO_PEST1);
  servoPest2.attach(SERVO_PEST2);
  servoWater.attach(SERVO_WATER);
  
  // Initialize servos to closed position
  servoPest1.write(SERVO_CLOSED);
  servoPest2.write(SERVO_CLOSED);
  servoWater.write(SERVO_CLOSED);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/joy", handleJoy);
  server.on("/api/status", handleStatus);
  server.on("/api/stop", handleStop);
  server.on("/api/direction", handleDirection);
  server.on("/api/speed", handleSpeed);
  server.on("/api/ultrasonic", handleUltrasonic);
  server.on("/api/servo", handleServoControl);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
  
  // Read ultrasonic sensor periodically
  readUltrasonicSensor();
  
  // Check for serial commands
  if (Serial.available()) {
    command = Serial.readStringUntil('\n');
    command.trim();
    processSerialCommand(command);
  }
}

float readUltrasonicSensor() {
  unsigned long currentMillis = millis();
  
  // Read at intervals to avoid blocking
  if (currentMillis - lastUltrasonicRead >= ultrasonicInterval) {
    lastUltrasonicRead = currentMillis;
    
    // Send trigger pulse
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    // Measure echo pulse duration (with timeout)
    long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
    
    if (duration == 0) {
      distance = -1; // No echo received
    } else {
      // Calculate distance in cm (speed of sound: 343 m/s)
      distance = duration * 0.034 / 2;
    }
    
    // Check if plant is detected (within 30cm)
    bool wasDetected = plantDetected;
    plantDetected = (distance > 0 && distance <= 30);
    
    // If plant just detected, record time
    if (plantDetected && !wasDetected) {
      plantDetectionTime = currentMillis;
      Serial.printf("Plant detected at %.1f cm!\n", distance);
      // Optional: Send notification about plant detection
    }
    
    // Debug output
    if (plantDetected) {
      Serial.printf("Distance: %.1f cm | Plant Detected\n", distance);
    }
  }
  
  return distance;
}

void processSerialCommand(String cmd) {
  // PESTICIDE 1 with servo
  if (cmd == "PEST1") {
    Serial.println("Activating Pesticide 1 with servo");
    
    // Activate pump
    digitalWrite(PUMP1, HIGH);
    
    // Oscillate servo for 5 seconds while spraying
    unsigned long startTime = millis();
    int oscillationDelay = 250; // Adjust speed here (ms between movements)
    
    while(millis() - startTime < 2500) {
      servoPest1.write(SERVO_CLOSED - 30);
      delay(oscillationDelay);
      servoPest1.write(SERVO_CLOSED + 30);
      delay(oscillationDelay);
    }
    
    digitalWrite(PUMP1, LOW);
    servoPest1.write(SERVO_CLOSED);
    
    Serial.println("Pesticide Pump1 finished spraying");
  }
  
  // PESTICIDE 2 with servo
  else if (cmd == "PEST2") {
    Serial.println("Activating Pesticide 2 with servo");
    
    digitalWrite(PUMP2, HIGH);
    
    unsigned long startTime = millis();
    int oscillationDelay = 250;
    
    while(millis() - startTime < 2500) {
      servoPest2.write(SERVO_CLOSED - 30);
      delay(oscillationDelay);
      servoPest2.write(SERVO_CLOSED + 30);
      delay(oscillationDelay);
    }
    
    digitalWrite(PUMP2, LOW);
    servoPest2.write(SERVO_CLOSED);
    
    Serial.println("Pesticide Pump2 finished spraying");
  }
  
  // WATER with servo
  else if (cmd == "WATER") {
    Serial.println("Activating Water spray with servo");
    
    digitalWrite(WATER_PUMP, HIGH);
    
    unsigned long startTime = millis();
    int oscillationDelay = 250;
    
    while(millis() - startTime < 2500) {
      servoWater.write(SERVO_CLOSED - 30);
      delay(oscillationDelay);
      servoWater.write(SERVO_CLOSED + 30);
      delay(oscillationDelay);
    }
    
    digitalWrite(WATER_PUMP, LOW);
    servoWater.write(SERVO_CLOSED);
    
    Serial.println("Water spray finished");
  }
  
  // Test individual servos
  else if (cmd == "SERVO1_ON") {
    servoPest1.write(SERVO_OPEN);
    Serial.println("Servo 1 opened");
  }
  else if (cmd == "SERVO1_OFF") {
    servoPest1.write(SERVO_CLOSED);
    Serial.println("Servo 1 closed");
  }
  else if (cmd == "SERVO2_ON") {
    servoPest2.write(SERVO_OPEN);
    Serial.println("Servo 2 opened");
  }
  else if (cmd == "SERVO2_OFF") {
    servoPest2.write(SERVO_CLOSED);
    Serial.println("Servo 2 closed");
  }
  else if (cmd == "SERVO3_ON") {
    servoWater.write(SERVO_OPEN);
    Serial.println("Servo 3 opened");
  }
  else if (cmd == "SERVO3_OFF") {
    servoWater.write(SERVO_CLOSED);
    Serial.println("Servo 3 closed");
  }
  
  // Get ultrasonic reading
  else if (cmd == "GET_DISTANCE") {
    Serial.printf("Distance: %.1f cm | Plant Detected: %s\n", 
                  distance, plantDetected ? "YES" : "NO");
  }
  
  // Auto-spray when plant detected (if enabled)
  else if (cmd == "AUTO_SPRAY_ON") {
    // This would enable automatic spraying on plant detection
    Serial.println("Auto-spray mode enabled");
  }
  else if (cmd == "AUTO_SPRAY_OFF") {
    Serial.println("Auto-spray mode disabled");
  }
}

void setMotorSpeeds(int leftSpeed, int rightSpeed) {
  leftSpeed = constrain(leftSpeed, -255, 255);
  rightSpeed = constrain(rightSpeed, -255, 255);
  
  leftMotorSpeed = leftSpeed;
  rightMotorSpeed = rightSpeed;
  
  // Left Motor
  if (leftSpeed > 0) {
    digitalWrite(LEFT_MOTOR_IN1, HIGH);
    digitalWrite(LEFT_MOTOR_IN2, LOW);
    // analogWrite(LEFT_MOTOR_ENA, leftSpeed);
  } else if (leftSpeed < 0) {
    digitalWrite(LEFT_MOTOR_IN1, LOW);
    digitalWrite(LEFT_MOTOR_IN2, HIGH);
    // analogWrite(LEFT_MOTOR_ENA, -leftSpeed);
  } else {
    digitalWrite(LEFT_MOTOR_IN1, LOW);
    digitalWrite(LEFT_MOTOR_IN2, LOW);
    // analogWrite(LEFT_MOTOR_ENA, 0);
  }
  
  // Right Motor
  if (rightSpeed > 0) {
    digitalWrite(RIGHT_MOTOR_IN3, HIGH);
    digitalWrite(RIGHT_MOTOR_IN4, LOW);
    // analogWrite(RIGHT_MOTOR_ENB, rightSpeed);
  } else if (rightSpeed < 0) {
    digitalWrite(RIGHT_MOTOR_IN3, LOW);
    digitalWrite(RIGHT_MOTOR_IN4, HIGH);
    // analogWrite(RIGHT_MOTOR_ENB, -rightSpeed);
  } else {
    digitalWrite(RIGHT_MOTOR_IN3, LOW);
    digitalWrite(RIGHT_MOTOR_IN4, LOW);
    // analogWrite(RIGHT_MOTOR_ENB, 0);
  }
  
  Serial.printf("Left: %d | Right: %d\n", leftSpeed, rightSpeed);
}

void stopMotors() {
  setMotorSpeeds(0, 0);
  currentMode = "stop";
}

void handleJoy() {
  if (server.hasArg("x") && server.hasArg("y")) {
    float x = server.arg("x").toFloat();
    float y = server.arg("y").toFloat();
    
    float leftSpeed = (y + x);
    float rightSpeed = (y - x);
    
    leftSpeed = (leftSpeed / 100.0) * 255;
    rightSpeed = (rightSpeed / 100.0) * 255;
    
    if (abs(leftSpeed) < 10) leftSpeed = 0;
    if (abs(rightSpeed) < 10) rightSpeed = 0;
    
    setMotorSpeeds((int)leftSpeed, (int)rightSpeed);
    
    if (abs(x) < 10 && abs(y) < 10) {
      currentMode = "stop";
    } else if (abs(y) > abs(x)) {
      currentMode = (y > 0) ? "forward" : "backward";
    } else {
      currentMode = (x > 0) ? "right" : "left";
    }
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    
    Serial.printf("Joystick: x=%.1f, y=%.1f | Left: %.1f, Right: %.1f\n",
                  x, y, leftSpeed, rightSpeed);
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing parameters\"}");
  }
}

void handleDirection() {
  if (server.hasArg("dir") && server.hasArg("speed")) {
    String direction = server.arg("dir");
    int speed = server.arg("speed").toInt();
    int motorSpeed = map(speed, 0, 100, 0, 255);
    
    if (direction == "forward") {
      setMotorSpeeds(motorSpeed, motorSpeed);
      currentMode = "forward";
    } else if (direction == "backward") {
      setMotorSpeeds(-motorSpeed, -motorSpeed);
      currentMode = "backward";
    } else if (direction == "left") {
      setMotorSpeeds(-motorSpeed, motorSpeed);
      currentMode = "left";
    } else if (direction == "right") {
      setMotorSpeeds(motorSpeed, -motorSpeed);
      currentMode = "right";
    } else if (direction == "rotate-left") {
      setMotorSpeeds(-motorSpeed, motorSpeed);
      currentMode = "rotate-left";
    } else if (direction == "rotate-right") {
      setMotorSpeeds(motorSpeed, -motorSpeed);
      currentMode = "rotate-right";
    } else if (direction == "diagonal-left") {
      setMotorSpeeds(0, motorSpeed);
      currentMode = "diagonal-left";
    } else if (direction == "diagonal-right") {
      setMotorSpeeds(motorSpeed, 0);
      currentMode = "diagonal-right";
    } else if (direction == "stop") {
      stopMotors();
    }
    
    server.send(200, "application/json", "{\"status\":\"ok\",\"mode\":\"" + currentMode + "\"}");
  } else if (server.hasArg("cmd")) {
    // Handle spray commands from web interface
    String cmd = server.arg("cmd");
    processSerialCommand(cmd);
    server.send(200, "application/json", "{\"status\":\"ok\",\"command\":\"" + cmd + "\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing parameters\"}");
  }
}

void handleSpeed() {
  if (server.hasArg("speed")) {
    int speedPercent = server.arg("speed").toInt();
    int motorSpeed = map(speedPercent, 0, 100, 0, 255);
    
    if (currentMode == "forward") {
      setMotorSpeeds(motorSpeed, motorSpeed);
    } else if (currentMode == "backward") {
      setMotorSpeeds(-motorSpeed, -motorSpeed);
    } else if (currentMode == "left") {
      setMotorSpeeds(-motorSpeed, motorSpeed);
    } else if (currentMode == "right") {
      setMotorSpeeds(motorSpeed, -motorSpeed);
    } else if (currentMode == "rotate-left") {
      setMotorSpeeds(-motorSpeed, motorSpeed);
    } else if (currentMode == "rotate-right") {
      setMotorSpeeds(motorSpeed, -motorSpeed);
    }
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing speed parameter\"}");
  }
}

void handleStatus() {
  String json = "{";
  json += "\"leftSpeed\":" + String(leftMotorSpeed) + ",";
  json += "\"rightSpeed\":" + String(rightMotorSpeed) + ",";
  json += "\"mode\":\"" + currentMode + "\",";
  json += "\"distance\":" + String(distance) + ",";
  json += "\"plantDetected\":" + String(plantDetected ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handleUltrasonic() {
  String json = "{";
  json += "\"distance\":" + String(distance) + ",";
  json += "\"plantDetected\":" + String(plantDetected ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

void handleServoControl() {
  if (server.hasArg("servo") && server.hasArg("position")) {
    int servoNum = server.arg("servo").toInt();
    int position = server.arg("position").toInt();
    position = constrain(position, 0, 180);
    
    switch(servoNum) {
      case 1:
        servoPest1.write(position);
        break;
      case 2:
        servoPest2.write(position);
        break;
      case 3:
        servoWater.write(position);
        break;
      default:
        server.send(400, "application/json", "{\"error\":\"Invalid servo number\"}");
        return;
    }
    
    server.send(200, "application/json", "{\"status\":\"ok\",\"servo\":" + String(servoNum) + ",\"position\":" + String(position) + "}");
  } else if (server.hasArg("servo") && server.hasArg("action")) {
    int servoNum = server.arg("servo").toInt();
    String action = server.arg("action");
    int position = (action == "open") ? SERVO_OPEN : SERVO_CLOSED;
    
    switch(servoNum) {
      case 1:
        servoPest1.write(position);
        if (action == "spray") {
          digitalWrite(PUMP1, HIGH);
          delay(3000);
          digitalWrite(PUMP1, LOW);
          servoPest1.write(SERVO_CLOSED);
        }
        break;
      case 2:
        servoPest2.write(position);
        if (action == "spray") {
          digitalWrite(PUMP2, HIGH);
          delay(3000);
          digitalWrite(PUMP2, LOW);
          servoPest2.write(SERVO_CLOSED);
        }
        break;
      case 3:
        servoWater.write(position);
        if (action == "spray") {
          digitalWrite(WATER_PUMP, HIGH);
          delay(3000);
          digitalWrite(WATER_PUMP, LOW);
          servoWater.write(SERVO_CLOSED);
        }
        break;
      default:
        server.send(400, "application/json", "{\"error\":\"Invalid servo number\"}");
        return;
    }
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    server.send(400, "application/json", "{\"error\":\"Missing parameters\"}");
  }
}

void handleStop() {
  stopMotors();
  server.send(200, "application/json", "{\"status\":\"stopped\"}");
}

void handleRoot() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
    <title>CropCare Bot</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
        }
        
        body {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }
        
        .container {
            background: rgba(255, 255, 255, 0.95);
            border-radius: 30px;
            padding: 30px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            max-width: 500px;
            width: 100%;
            backdrop-filter: blur(10px);
        }
        
        h1 {
            text-align: center;
            color: #333;
            margin-bottom: 10px;
            font-size: 24px;
            font-weight: 600;
        }
        
        .subtitle {
            text-align: center;
            color: #666;
            margin-bottom: 25px;
            font-size: 12px;
        }
        
        .sensor-panel {
            background: linear-gradient(135deg, #48bb78, #38a169);
            border-radius: 20px;
            padding: 15px;
            margin-bottom: 20px;
            color: white;
        }
        
        .sensor-title {
            font-size: 14px;
            opacity: 0.9;
            margin-bottom: 10px;
        }
        
        .distance-display {
            font-size: 32px;
            font-weight: bold;
            text-align: center;
        }
        
        .plant-status {
            text-align: center;
            margin-top: 5px;
            font-size: 14px;
        }
        
        .plant-detected {
            animation: pulse 1s infinite;
        }
        
        @keyframes pulse {
            0% { opacity: 1; }
            50% { opacity: 0.6; }
            100% { opacity: 1; }
        }
        
        .connection-status {
            text-align: center;
            padding: 10px;
            border-radius: 20px;
            margin-bottom: 20px;
            font-weight: 500;
            transition: all 0.3s ease;
        }
        
        .connected {
            background: #c6f6d5;
            color: #22543d;
        }
        
        .disconnected {
            background: #fed7d7;
            color: #742a2a;
        }
        
        .joystick-section {
            text-align: center;
            margin: 20px 0;
        }
        
        .section-title {
            font-size: 18px;
            color: #333;
            margin-bottom: 15px;
            font-weight: 600;
        }
        
        .joystick-container {
            display: flex;
            justify-content: center;
            margin: 10px 0;
        }
        
        #joystick {
            width: 250px;
            height: 250px;
            background: linear-gradient(145deg, #f0f0f0, #d9d9d9);
            border-radius: 50%;
            position: relative;
            touch-action: none;
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
            cursor: grab;
            margin: 0 auto;
        }
        
        #joystick-handle {
            width: 90px;
            height: 90px;
            background: linear-gradient(145deg, #4a5568, #2d3748);
            border-radius: 50%;
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            pointer-events: none;
            border: 3px solid #718096;
        }
        
        .spray-buttons {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 10px;
            margin: 20px 0;
        }
        
        .spray-btn {
            padding: 15px;
            border: none;
            border-radius: 12px;
            color: white;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            transition: transform 0.2s;
        }
        
        .spray-btn:active {
            transform: scale(0.95);
        }
        
        .spray-water {
            background: linear-gradient(135deg, #4299e1, #2b6cb0);
        }
        
        .spray-pest1 {
            background: linear-gradient(135deg, #ed8936, #dd6b20);
        }
        
        .spray-pest2 {
            background: linear-gradient(135deg, #9f7aea, #805ad5);
        }
        
        .servo-controls {
            background: #f7fafc;
            border-radius: 15px;
            padding: 15px;
            margin: 15px 0;
        }
        
        .servo-buttons {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 8px;
            margin-top: 10px;
        }
        
        .servo-btn {
            padding: 10px;
            background: #e2e8f0;
            border: none;
            border-radius: 8px;
            font-size: 12px;
            cursor: pointer;
        }
        
        .direction-buttons {
            display: grid;
            grid-template-columns: repeat(3, 1fr);
            gap: 10px;
            margin: 20px 0;
        }
        
        .dir-btn {
            padding: 12px 5px;
            border: none;
            border-radius: 12px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            font-size: 12px;
            font-weight: 600;
            cursor: pointer;
        }
        
        .dir-btn.stop {
            background: linear-gradient(135deg, #f56565 0%, #c53030 100%);
        }
        
        .stop-btn {
            background: linear-gradient(135deg, #f56565, #c53030);
            color: white;
            padding: 15px;
            border: none;
            border-radius: 12px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            width: 100%;
            margin-top: 10px;
        }
        
        .info-text {
            text-align: center;
            color: #718096;
            font-size: 11px;
            margin-top: 20px;
            padding: 10px;
            background: #f7fafc;
            border-radius: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌱 CropCare Bot</h1>
        <div class="subtitle">Smart Agriculture Robot</div>
        
        <div class="sensor-panel">
            <div class="sensor-title">📡 Ultrasonic Sensor</div>
            <div id="distanceDisplay" class="distance-display">-- cm</div>
            <div id="plantStatus" class="plant-status">🔍 Scanning for plants...</div>
        </div>
        
        <div id="connectionStatus" class="connection-status connected">
            ✓ Connected to Robot
        </div>
        
        <div class="joystick-section">
            <div class="section-title">🕹️ Joystick Control</div>
            <div class="joystick-container">
                <div id="joystick">
                    <div id="joystick-handle"></div>
                </div>
            </div>
        </div>
        
        <div class="spray-buttons">
            <button class="spray-btn spray-water" onclick="sprayNow('water')">💧 WATER</button>
            <button class="spray-btn spray-pest1" onclick="sprayNow('pest1')">🐛 PEST 1</button>
            <button class="spray-btn spray-pest2" onclick="sprayNow('pest2')">🦟 PEST 2</button>
        </div>
        
        <div class="servo-controls">
            <div style="font-weight:600; margin-bottom:10px;">🎮 Manual Servo Test</div>
            <div class="servo-buttons">
                <button class="servo-btn" onclick="controlServo(1, 'open')">Servo 1 Open</button>
                <button class="servo-btn" onclick="controlServo(1, 'close')">Servo 1 Close</button>
                <button class="servo-btn" onclick="controlServo(2, 'open')">Servo 2 Open</button>
                <button class="servo-btn" onclick="controlServo(2, 'close')">Servo 2 Close</button>
                <button class="servo-btn" onclick="controlServo(3, 'open')">Servo 3 Open</button>
                <button class="servo-btn" onclick="controlServo(3, 'close')">Servo 3 Close</button>
            </div>
        </div>
        
        <div class="direction-buttons">
            <button class="dir-btn" onclick="sendDirection('forward')">↑ FWD</button>
            <button class="dir-btn" onclick="sendDirection('stop')">⏹ STOP</button>
            <button class="dir-btn" onclick="sendDirection('backward')">↓ REV</button>
            <button class="dir-btn" onclick="sendDirection('left')">← LEFT</button>
            <button class="dir-btn stop" onclick="sendDirection('stop')">⏹ STOP</button>
            <button class="dir-btn" onclick="sendDirection('right')">→ RIGHT</button>
            <button class="dir-btn" onclick="sendDirection('rotate-left')">↺ ROT L</button>
            <button class="dir-btn" onclick="sendDirection('rotate-right')">↻ ROT R</button>
            <button class="dir-btn" onclick="sendDirection('stop')">⏹ STOP</button>
        </div>
        
        <button class="stop-btn" id="emergencyStop">🚨 EMERGENCY STOP</button>
        
        <div class="info-text">
            🔋 Robot with Ultrasonic Sensor | 3 Servo Motors for Spraying
        </div>
    </div>

    <script>
        let isDragging = false;
        let currentX = 0, currentY = 0;
        const JOYSTICK_RADIUS = 90;
        
        const joystick = document.getElementById('joystick');
        const handle = document.getElementById('joystick-handle');
        
        function updateHandlePosition(x, y) {
            const posX = (x / 100) * JOYSTICK_RADIUS;
            const posY = -(y / 100) * JOYSTICK_RADIUS;
            handle.style.transform = `translate(calc(-50% + ${posX}px), calc(-50% + ${posY}px))`;
        }
        
        function handleJoystickMove(clientX, clientY) {
            const rect = joystick.getBoundingClientRect();
            const centerX = rect.left + rect.width / 2;
            const centerY = rect.top + rect.height / 2;
            
            let deltaX = clientX - centerX;
            let deltaY = centerY - clientY;
            
            const distance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);
            
            if (distance > JOYSTICK_RADIUS) {
                deltaX = (deltaX / distance) * JOYSTICK_RADIUS;
                deltaY = (deltaY / distance) * JOYSTICK_RADIUS;
            }
            
            currentX = Math.round((deltaX / JOYSTICK_RADIUS) * 100);
            currentY = Math.round((deltaY / JOYSTICK_RADIUS) * 100);
            
            if (Math.abs(currentX) < 5) currentX = 0;
            if (Math.abs(currentY) < 5) currentY = 0;
            
            updateHandlePosition(currentX, currentY);
            
            fetch(`/joy?x=${currentX}&y=${currentY}`).catch(e => console.log(e));
        }
        
        function resetJoystick() {
            currentX = 0;
            currentY = 0;
            updateHandlePosition(0, 0);
            fetch(`/joy?x=0&y=0`).catch(e => console.log(e));
        }
        
        joystick.addEventListener('mousedown', (e) => {
            isDragging = true;
            handleJoystickMove(e.clientX, e.clientY);
        });
        
        document.addEventListener('mousemove', (e) => {
            if (!isDragging) return;
            handleJoystickMove(e.clientX, e.clientY);
        });
        
        document.addEventListener('mouseup', () => {
            if (isDragging) {
                isDragging = false;
                resetJoystick();
            }
        });
        
        joystick.addEventListener('touchstart', (e) => {
            e.preventDefault();
            isDragging = true;
            const touch = e.touches[0];
            handleJoystickMove(touch.clientX, touch.clientY);
        });
        
        document.addEventListener('touchmove', (e) => {
            if (!isDragging) return;
            e.preventDefault();
            const touch = e.touches[0];
            handleJoystickMove(touch.clientX, touch.clientY);
        });
        
        document.addEventListener('touchend', () => {
            if (isDragging) {
                isDragging = false;
                resetJoystick();
            }
        });
        
        function sendDirection(dir) {
            fetch(`/api/direction?dir=${dir}&speed=50`).catch(e => console.log(e));
            if (dir === 'stop') resetJoystick();
        }
        
        function sprayNow(type) {
            let command = '';
            if (type === 'water') command = 'WATER';
            else if (type === 'pest1') command = 'PEST1';
            else if (type === 'pest2') command = 'PEST2';
            
            fetch(`/api/direction?cmd=${command}`)
                .then(() => alert(`Spraying ${type.toUpperCase()}...`))
                .catch(e => console.log(e));
        }
        
        function controlServo(servoNum, action) {
            fetch(`/api/servo?servo=${servoNum}&action=${action}`)
                .then(() => console.log(`Servo ${servoNum} ${action}`))
                .catch(e => console.log(e));
        }
        
        function fetchUltrasonic() {
            fetch('/api/ultrasonic')
                .then(response => response.json())
                .then(data => {
                    const distanceElem = document.getElementById('distanceDisplay');
                    const plantStatus = document.getElementById('plantStatus');
                    
                    if (data.distance < 0) {
                        distanceElem.textContent = 'ERR cm';
                        plantStatus.textContent = '⚠️ Sensor Error';
                    } else {
                        distanceElem.textContent = `${data.distance.toFixed(1)} cm`;
                        if (data.plantDetected) {
                            plantStatus.innerHTML = '🌿 PLANT DETECTED! 🌿';
                            plantStatus.style.color = '#fbbf24';
                            plantStatus.style.fontWeight = 'bold';
                        } else {
                            plantStatus.innerHTML = '🔍 Scanning for plants...';
                            plantStatus.style.color = 'white';
                            plantStatus.style.fontWeight = 'normal';
                        }
                    }
                })
                .catch(e => console.log(e));
        }
        
        document.getElementById('emergencyStop').addEventListener('click', () => {
            fetch('/api/stop').then(() => resetJoystick());
        });
        
        setInterval(fetchUltrasonic, 200);
        fetchUltrasonic();
    </script>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", page);
}
