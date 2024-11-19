#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

// =========================
// Wi-Fi Configuration
// =========================
const char* ssid = "iPhone de Gianna"; 
const char* password = "carlosviotti"; 

// =========================
// Web Server Setup
// =========================
ESP8266WebServer server(80); // Create a web server object that listens on port 80

// =========================
// Pin Definitions
// =========================

// L298N Motor Driver Pins
// Motor A
const int motorA_IN1 = D7;
const int motorA_IN2 = D6;
const int motorA_ENA = D8;

// Motor B
const int motorB_IN3 = D5;
const int motorB_IN4 = D4;
const int motorB_ENB = D3;

// Micro Servo Pin
const int servoPin = D2;

// =========================
// State Variables
// =========================
bool motorsRunning = false; // Tracks whether motors are currently running
Servo myServo; // Servo object
int servoPosition = 90; // Initial position (middle)

bool obstacleDetected = false; // Tracks obstacle status

// Variables for servo scanning
unsigned long previousServoMillis = 0;
const long servoInterval = 10; // Interval between servo movements in milliseconds
bool servoMovingForward = true;

// Variables for reversing
unsigned long reverseStartTime = 0;
bool reversing = false;

// Current movement direction
String currentDirection = "stop";

// =========================
// Function: Move Forward
// =========================
void moveForward() {
  digitalWrite(motorA_IN1, HIGH);
  digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH);
  digitalWrite(motorB_IN4, LOW);
  
  // Set motor speed to full 
  analogWrite(motorA_ENA, 1023); // Full speed for Motor A
  analogWrite(motorB_ENB, 1023); // Full speed for Motor B
  
  motorsRunning = true;
  
  Serial.println("Motors moving forward.");
}

// =========================
// Function: Move Backward
// =========================
void moveBackward() {
  digitalWrite(motorA_IN1, LOW);
  digitalWrite(motorA_IN2, HIGH);
  digitalWrite(motorB_IN3, LOW);
  digitalWrite(motorB_IN4, HIGH);
  
  // Set motor speed to full 
  analogWrite(motorA_ENA, 1023); // Full speed for Motor A
  analogWrite(motorB_ENB, 1023); // Full speed for Motor B
  
  motorsRunning = true;
  
  Serial.println("Motors moving backward.");
}

// =========================
// Function: Turn Left
// =========================
void turnLeft() {
  // Stop left motors, move right motors forward
  digitalWrite(motorA_IN1, LOW);
  digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, HIGH);
  digitalWrite(motorB_IN4, LOW);
  
  analogWrite(motorA_ENA, 0);    // Stop Motor A
  analogWrite(motorB_ENB, 1023); // Full speed for Motor B
  
  motorsRunning = true;
  
  Serial.println("Turning left.");
}

// =========================
// Function: Turn Right
// =========================
void turnRight() {
  // Move left motors forward, stop right motors
  digitalWrite(motorA_IN1, HIGH);
  digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, LOW);
  digitalWrite(motorB_IN4, LOW);
  
  analogWrite(motorA_ENA, 1023); // Full speed for Motor A
  analogWrite(motorB_ENB, 0);    // Stop Motor B
  
  motorsRunning = true;
  
  Serial.println("Turning right.");
}

// =========================
// Function: Stop Motors
// =========================
void stopMotors() {
  digitalWrite(motorA_IN1, LOW);
  digitalWrite(motorA_IN2, LOW);
  digitalWrite(motorB_IN3, LOW);
  digitalWrite(motorB_IN4, LOW);
  
  analogWrite(motorA_ENA, 0); // Stop Motor A
  analogWrite(motorB_ENB, 0); // Stop Motor B
  
  motorsRunning = false;
  
  Serial.println("Motors stopped.");
}
// =========================
// Function: Rotate Servo Automatically
// =========================
void autoRotateServo() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousServoMillis >= servoInterval) {
    previousServoMillis = currentMillis;
    
    if (servoMovingForward) {
      servoPosition += 1; // Increment position
      if (servoPosition >= 180) {
        servoPosition = 180;
        servoMovingForward = false;
      }
    } else {
      servoPosition -= 1; // Decrement position
      if (servoPosition <= 0) {
        servoPosition = 0;
        servoMovingForward = true;
      }
    }
    
    myServo.write(servoPosition);
    // Serial.print("Servo position: ");
    // Serial.println(servoPosition);
  }
}


// =========================
// Function: Handle Root URL
// =========================
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><title>ESP8266 Car Control</title>";
  
  // Meta tags for responsiveness
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  
  // CSS Styles for responsiveness and aesthetics
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; margin: 0; padding: 0; background-color: #f0f0f0; }";
  html += ".container { display: flex; flex-direction: column; justify-content: center; align-items: center; min-height: 100vh; padding: 20px; }";
  html += ".button-row { display: flex; flex-wrap: wrap; justify-content: center; gap: 10px; margin-bottom: 20px; }";
  html += "button { font-size: 1em; padding: 15px 30px; border: none; border-radius: 5px; cursor: pointer; transition: background-color 0.3s; }";
  html += "button:active { background-color: #ddd; }";
  html += ".forward { background-color: #4CAF50; color: white; }";
  html += ".stop { background-color: #f44336; color: white; }";
  html += ".left { background-color: #2196F3; color: white; }";
  html += ".right { background-color: #FF9800; color: white; }";
  html += "@media (max-width: 600px) {";
  html += "  button { width: 100%; padding: 15px 0; }";
  html += "  .button-row { flex-direction: column; }";
  html += "}";
  html += "</style>";
  
  // JavaScript for handling press-and-hold
  html += "<script>";
  html += "function sendCommand(command) {";
  html += "  fetch('/' + command);";
  html += "}";
  
  // Handle mouse and touch events for each button
  html += "window.onload = function() {";
  
  // Forward Button
  html += "  var forwardBtn = document.getElementById('forwardBtn');";
  html += "  forwardBtn.addEventListener('mousedown', function() { sendCommand('forward/start'); });";
  html += "  forwardBtn.addEventListener('mouseup', function() { sendCommand('forward/stop'); });";
  html += "  forwardBtn.addEventListener('mouseleave', function() { sendCommand('forward/stop'); });";
  html += "  forwardBtn.addEventListener('touchstart', function(e) { e.preventDefault(); sendCommand('forward/start'); });";
  html += "  forwardBtn.addEventListener('touchend', function() { sendCommand('forward/stop'); });";
  
  // Backward Button
  html += "  var backwardBtn = document.getElementById('backwardBtn');";
  html += "  backwardBtn.addEventListener('mousedown', function() { sendCommand('backward/start'); });";
  html += "  backwardBtn.addEventListener('mouseup', function() { sendCommand('backward/stop'); });";
  html += "  backwardBtn.addEventListener('mouseleave', function() { sendCommand('backward/stop'); });";
  html += "  backwardBtn.addEventListener('touchstart', function(e) { e.preventDefault(); sendCommand('backward/start'); });";
  html += "  backwardBtn.addEventListener('touchend', function() { sendCommand('backward/stop'); });";
  
  // Left Button
  html += "  var leftBtn = document.getElementById('leftBtn');";
  html += "  leftBtn.addEventListener('mousedown', function() { sendCommand('left/start'); });";
  html += "  leftBtn.addEventListener('mouseup', function() { sendCommand('left/stop'); });";
  html += "  leftBtn.addEventListener('mouseleave', function() { sendCommand('left/stop'); });";
  html += "  leftBtn.addEventListener('touchstart', function(e) { e.preventDefault(); sendCommand('left/start'); });";
  html += "  leftBtn.addEventListener('touchend', function() { sendCommand('left/stop'); });";
  
  // Right Button
  html += "  var rightBtn = document.getElementById('rightBtn');";
  html += "  rightBtn.addEventListener('mousedown', function() { sendCommand('right/start'); });";
  html += "  rightBtn.addEventListener('mouseup', function() { sendCommand('right/stop'); });";
  html += "  rightBtn.addEventListener('mouseleave', function() { sendCommand('right/stop'); });";
  html += "  rightBtn.addEventListener('touchstart', function(e) { e.preventDefault(); sendCommand('right/start'); });";
  html += "  rightBtn.addEventListener('touchend', function() { sendCommand('right/stop'); });";
  
  html += "}";
  html += "</script>";
  
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>ESP8266 Car Control</h1>";
    
  // Motor Controls
  html += "<div class='button-row'>";
  html += "<button id='forwardBtn' class='forward'>Forward</button>";
  html += "<button id='backwardBtn' class='stop'>Backward</button>";
  html += "</div>";
  
  // Directional Controls
  html += "<div class='button-row'>";
  html += "<button id='leftBtn' class='left'>Turn Left</button>";
  html += "<button id='rightBtn' class='right'>Turn Right</button>";
  html += "</div>";
  
  html += "</div>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

// =========================
// Function: Handle Start Commands
// =========================
void handleStart(const String& direction) {
  if (direction == "forward") {
    moveForward();
  } else if (direction == "backward") {
    moveBackward();
  } else if (direction == "left") {
    turnLeft();
  } else if (direction == "right") {
    turnRight();
  }
  currentDirection = direction;
  server.send(200, "text/plain", "OK");
}

// =========================
// Function: Handle Stop Commands
// =========================
void handleStop(const String& direction) {
  if (currentDirection == direction) {
    stopMotors();
    currentDirection = "stop";
  }
  server.send(200, "text/plain", "OK");
}

// =========================
// Route Handlers for Start and Stop
// =========================
void handleCommand() {
  String path = server.uri();
  // Expected format: /direction/action (e.g., /forward/start)
  int firstSlash = path.indexOf('/');
  int secondSlash = path.indexOf('/', firstSlash + 1);
  
  if (firstSlash != -1 && secondSlash != -1) {
    String direction = path.substring(firstSlash + 1, secondSlash);
    String action = path.substring(secondSlash + 1);
    
    if (action == "start") {
      handleStart(direction);
    } else if (action == "stop") {
      handleStop(direction);
    } else {
      server.send(404, "text/plain", "Not Found");
    }
  } else {
    server.send(404, "text/plain", "Not Found");
  }
}

// =========================
// Function: Setup
// =========================
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  Serial.println();
  Serial.println("ESP8266 Car Control Starting...");

  // Initialize Pins
  pinMode(motorA_IN1, OUTPUT);
  pinMode(motorA_IN2, OUTPUT);
  pinMode(motorA_ENA, OUTPUT);
  
  pinMode(motorB_IN3, OUTPUT);
  pinMode(motorB_IN4, OUTPUT);
  pinMode(motorB_ENB, OUTPUT);
  
  // Initialize Servo
  myServo.attach(servoPin);
  myServo.write(servoPosition); // Initialize servo to center position
  
  // Initialize Motors to Stopped State
  stopMotors();
  
  // Connect to Wi-Fi
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(ssid);
  
  WiFi.mode(WIFI_STA); // Set Wi-Fi to station mode
  WiFi.begin(ssid, password);
  
  // Wait until connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  // Connected
  Serial.println("");
  Serial.println("Wi-Fi connected successfully!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Define Web Server Routes
  server.on("/", handleRoot);
  
  // Dynamic routing for /direction/action
  server.onNotFound([](){
    handleCommand();
  });
  
  // Start Web Server
  server.begin();
  Serial.println("Web server started.");
}

// =========================
// Function: Loop
// =========================
void loop() {
  server.handleClient(); // Handle incoming client requests

  // Automatic Servo Scanning (if needed)
  autoRotateServo(); // Uncomment if servo functionality is desired

  delay(10); // Short delay to prevent excessive CPU usage
}
