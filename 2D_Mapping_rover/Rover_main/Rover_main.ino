#include <Wire.h>
#include <MPU6050.h>
#include <WiFi.h>
#include <math.h>

// Motor driver and ultrasonic sensor pins
const int in1 = 27;
const int in2 = 26;
const int in3 = 25;
const int in4 = 33;

const int trigPin = 4;
const int echoPin = 5;

// WiFi configuration
const int port = 80; 
WiFiServer server(port);

String command = "";
bool stopRover = false;
float posX = 0;  // Track the rover's position on the X-axis
float posY = 0;  // Track the rover's position on the Y-axis
int direction = 0; // 0 = North, 1 = East, 2 = South, 3 = West

void setup() {
  Serial.begin(9600);

  // Initialize I2C communication for MPU6050
  Wire.begin();

  // Initialize Ultrasonic sensor pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Initialize motor driver pins
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  // Connect to WiFi
  WiFi.begin("realme GT 2", "00000000");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Port: ");
  Serial.println(port);

  // Start the server
  server.begin();
}

long readDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  return duration / 58.2; // distance in cm
}

void moveForward() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, LOW);
  digitalWrite(in4, HIGH);
}

void stopMoving() {
  digitalWrite(in1, LOW);
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  digitalWrite(in4, LOW);
}

void turnRight() {
  // Rotate the rover to the right by turning the left wheels forward and right wheels backward
  digitalWrite(in1, LOW);
  digitalWrite(in2, HIGH);
  digitalWrite(in3, HIGH);
  digitalWrite(in4, LOW);
  delay(250); // Adjust delay to control the turn angle
  stopMoving();
  
  // Update direction after turning
  direction = (direction + 1) % 4;
}

void updatePosition(float distance) {
  switch (direction) {
    case 0: // Moving North
      posY += distance;
      break;
    case 1: // Moving East
      posX += distance;
      break;
    case 2: // Moving South
      posY -= distance;
      break;
    case 3: // Moving West
      posX -= distance;
      break;
  }
  
  Serial.println("Updated position - X: " + String(posX, 2) + ", Y: " + String(posY, 2));
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (command.startsWith("START")) {
            stopRover = false;
            Serial.println("Rover started");
          } else if (command.startsWith("STOP")) {
            stopRover = true;
            stopMoving();
            Serial.println("Rover stopped");
          }
          command = "";
        } else {
          command += c;
        }
      }

      if (!stopRover) {
        long distance = readDistance();

        if (distance > 20) {
          moveForward();
          delay(500);  // Adjust delay based on your rover's speed
          stopMoving();

          // Update position after moving forward
          updatePosition(10.0);
          client.println("COORD," + String(posX, 2) + "," + String(posY, 2));
          while (!client.available()) {
            delay(10);
          }
        } else {
          stopMoving();
          client.println("COORD," + String(posX, 2) + "," + String(posY, 2));
          while (!client.available()) {
            delay(10);
          }

          if (distance <= 20) {
            turnRight();  // Turn the rover to the right
          }
        }
      }
    }
    client.stop();
  }
}
