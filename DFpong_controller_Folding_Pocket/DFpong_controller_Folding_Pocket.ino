/*********************************************************************
 * Experiment 4: Proximities-Pong Edition
 * Yudi Cao
 * For Creation & Computation OCADU 2024 Fall
 * Game Link : https://digitalfuturesocadu.github.io/df-pong/
 *
 * This controller uses two light sensors on the left and right to compare light intensity.
 * When one box is opened and receives more light than the other, the paddle moves.
 * - Left sensor is darker: The paddle moves up.
 * - Right sensor is darker: The paddle moves down.
 * When both sensors receive the same amount of light (ideally when both boxes are closed), the paddle remains still.
 * 
 * Movement Values:
 * 0 = No movement / Neutral position
 * 1 = Left movement (paddle moves up)
 * 2 = Right movement (paddle moves down)
 * 
 * Key Functions:
 * - handleInput(): Process the inputs to generate the states
 * - sendMovement(): Sends movement data over BLE (0-3)
 * - updateBLE(): Handles BLE connection management and updates
 * - updateBuzzer(): Provides different buzzer patterns for different movements
 * 
 * This program implements a Bluetooth Low Energy controller for Pong.
 * It sends movement data to a central device running in the browser and
 * provides audio feedback through a buzzer.
 *
 * Reference: 
 * light_compareSensors: https://github.com/DigitalFuturesOCADU/CC2024/tree/31af1a8070f3d7e2629c496767633e48b1e6b189/experiment4/Arduino/Sensors/Light 
 *
 * Buzzer Sensor: https://github.com/DigitalFuturesOCADU/CC2024/tree/31af1a8070f3d7e2629c496767633e48b1e6b189/experiment4/Arduino/BeepBoop/touch5_BeepBoop

 *********************************************************************/

#include <ArduinoBLE.h>
#include "ble_functions.h"
#include "buzzer_functions.h"

// Controller and hardware configuration
const char* deviceName = "Folding Pocket";  // Controller's Bluetooth name

// Pin definitions
const int lightPin1 = A7;         // Light sensor 1 (up movement)
const int lightPin2 = A6;         // Light sensor 2 (down movement)
const int BUZZER_PIN = 11;        // Pin for buzzer feedback
const int LED_PIN = LED_BUILTIN;  // Status LED pin

// Movement state tracking
int currentMovement = 0;          // Current movement value (0=none, 1=left, 2=right)

// Light sensor processing variables
const int lightAverageWindow = 10; // Number of samples used for the rolling average
const int equalityThreshold = 50;  // Sensitivity for determining if light intensities are equal
int lightValue1 = 0;              // Raw value for sensor 1
int lightValue2 = 0;              // Raw value for sensor 2
int smoothedLight1 = 0;           // Smoothed value for sensor 1
int smoothedLight2 = 0;           // Smoothed value for sensor 2
unsigned long lastLightReadTime = 0; // Time when the last reading was taken
unsigned int lightReadInterval = 50;  // Interval between light readings (milliseconds)

// Rolling average variables
int lightReadings1[lightAverageWindow]; // Array to store previous readings for sensor 1
int lightReadings2[lightAverageWindow]; // Array to store previous readings for sensor 2
int lightReadIndex = 0;                 // Index to track the current position in the rolling average arrays
long lightTotalValue1 = 0;              // Sum of all readings for sensor 1
long lightTotalValue2 = 0;              // Sum of all readings for sensor 2

// Function to initialize rolling average arrays
void initializeLightAverage() {
  // Initialize rolling average arrays with zeros
  for (int i = 0; i < lightAverageWindow; i++) {
    lightReadings1[i] = 0;
    lightReadings2[i] = 0;
  }
  // Reset total values and index
  lightTotalValue1 = 0;
  lightTotalValue2 = 0;
  lightReadIndex = 0;
}

// Update rolling average for light sensors
void updateLightAverage(int newValue1, int newValue2) {
  // Remove oldest reading from the total and replace it with the new value for sensor 1
  lightTotalValue1 = lightTotalValue1 - lightReadings1[lightReadIndex];
  lightReadings1[lightReadIndex] = newValue1;
  lightTotalValue1 = lightTotalValue1 + newValue1;

  // Remove oldest reading from the total and replace it with the new value for sensor 2
  lightTotalValue2 = lightTotalValue2 - lightReadings2[lightReadIndex];
  lightReadings2[lightReadIndex] = newValue2;
  lightTotalValue2 = lightTotalValue2 + newValue2;

  // Move to the next index in the rolling average arrays (circular buffer)
  lightReadIndex = (lightReadIndex + 1) % lightAverageWindow;

  // Calculate smoothed values for both sensors
  smoothedLight1 = lightTotalValue1 / lightAverageWindow;
  smoothedLight2 = lightTotalValue2 / lightAverageWindow;
}

// Read light sensors and update movement state
void handleInput() {
  unsigned long currentTime = millis();
  // Check if it's time to read the light sensors
  if (currentTime - lastLightReadTime >= lightReadInterval) {
    // Read the raw values from the light sensors
    lightValue1 = analogRead(lightPin1);
    lightValue2 = analogRead(lightPin2);

    // Update the rolling average with the new values
    updateLightAverage(lightValue1, lightValue2);

    // Determine movement direction based on smoothed values
    int difference = abs(smoothedLight1 - smoothedLight2);
    if (difference <= equalityThreshold) {
      currentMovement = 0;  // Neutral (stop)
    } else if (smoothedLight1 > smoothedLight2) {
      currentMovement = 1;  // Left movement
    } else {
      currentMovement = 2;  // Right movement
    }

    // Update the last light read time
    lastLightReadTime = currentTime;
  }
}

// Arduino setup function
void setup() {
  Serial.begin(9600);  // Start serial communication for debugging
  Serial.println("Pong Game Controller Initialized");

  // Initialize light sensor rolling averages
  initializeLightAverage();

  // Configure LED and buzzer pins
  pinMode(LED_PIN, OUTPUT);
  setupBuzzer(BUZZER_PIN);

  // Initialize BLE with the device name and status LED
  setupBLE(deviceName, LED_PIN);
}

// Arduino loop function
void loop() {
  // Update BLE connection and handle incoming data
  updateBLE();

  // Process light sensor input to determine movement state
  handleInput();

  // Send movement state to the central device (game)
  sendMovement(currentMovement);

  // Provide buzzer feedback based on movement state
  updateBuzzer(currentMovement);
}
