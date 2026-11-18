
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <DHT.h>

// ---------------- PIN CONFIGURATION ----------------

#define AIR_MOSFET_PIN 14     // D5 - GPIO14
#define SOIL_MOSFET_PIN 12    // D6 - GPIO12

#define AIR_LED_PIN 13        // D7 - Green LED
#define SOIL_LED_PIN 15       // D8 - Blue LED

#define DHT_PIN 16            // D0 - GPIO16
#define DHT_TYPE DHT11

// ---------------- SENSOR CONFIGURATION ----------------

Adafruit_ADS1115 ads;
DHT dht(DHT_PIN, DHT_TYPE);

// ADS1115 channel mapping
#define MQ7_CHANNEL       0   // A0
#define MQ135_CHANNEL     1   // A1
#define RAIN_CHANNEL      2   // A2
#define SOIL_CHANNEL      3   // A3

// Voltage divider: 12k (top), 20k (bottom)
const float DIVIDER_FACTOR = 1.6;

// Adjust these values after calibrating your sensors.
// These are example ADC endpoint values, not universal constants.
const int SOIL_DRY_RAW = 26000;
const int SOIL_WET_RAW = 12000;

const int RAIN_DRY_RAW = 26000;
const int RAIN_WET_RAW = 12000;

// ---------------- TIMING ----------------

// Each block remains selected for 2 minutes.
// Note: MQ sensors require appropriate heater conditioning;
// this switching interval alone does not guarantee valid gas readings.
const unsigned long MODE_INTERVAL = 120000UL;

unsigned long lastModeChange = 0;
bool airMode = true;

// ---------------- HELPER FUNCTIONS ----------------

float readADCVoltage(uint8_t channel) {
  int16_t raw = ads.readADC_SingleEnded(channel);
  return ads.computeVolts(raw);
}

float readMQSensorVoltage(uint8_t channel) {
  // ADS voltage is measured after the voltage divider.
  // Convert it back to the sensor module's output voltage.
  return readADCVoltage(channel) * DIVIDER_FACTOR;
}

float calculatePercentage(int raw, int dryValue, int wetValue) {
  float percentage =
      ((float)(dryValue - raw) / (dryValue - wetValue)) * 100.0;

  return constrain(percentage, 0.0, 100.0);
}

void switchToAirMode() {
  // Turn both blocks OFF before selecting the next block.
  digitalWrite(AIR_MOSFET_PIN, LOW);
  digitalWrite(SOIL_MOSFET_PIN, LOW);

  digitalWrite(AIR_LED_PIN, LOW);
  digitalWrite(SOIL_LED_PIN, LOW);

  delay(100);

  digitalWrite(AIR_MOSFET_PIN, HIGH);
  digitalWrite(AIR_LED_PIN, HIGH);

  airMode = true;
  lastModeChange = millis();

  Serial.println("\nMODE: AIR QUALITY");
}

void switchToSoilMode() {
  digitalWrite(AIR_MOSFET_PIN, LOW);
  digitalWrite(SOIL_MOSFET_PIN, LOW);

  digitalWrite(AIR_LED_PIN, LOW);
  digitalWrite(SOIL_LED_PIN, LOW);

  delay(100);

  digitalWrite(SOIL_MOSFET_PIN, HIGH);
  digitalWrite(SOIL_LED_PIN, HIGH);

  airMode = false;
  lastModeChange = millis();

  Serial.println("\nMODE: SOIL QUALITY");
}

// ---------------- AIR QUALITY READINGS ----------------

void readAirSensors() {
  float mq7Voltage = readMQSensorVoltage(MQ7_CHANNEL);
  float mq135Voltage = readMQSensorVoltage(MQ135_CHANNEL);

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  Serial.println("----- AIR QUALITY DATA -----");

  Serial.print("MQ7 Raw Output Voltage: ");
  Serial.print(mq7Voltage, 3);
  Serial.println(" V");

  Serial.print("MQ135 Raw Output Voltage: ");
  Serial.print(mq135Voltage, 3);
  Serial.println(" V");

  if (!isnan(temperature) && !isnan(humidity)) {
    Serial.print("Temperature: ");
    Serial.print(temperature, 1);
    Serial.println(" C");

    Serial.print("Humidity: ");
    Serial.print(humidity, 1);
    Serial.println(" %");
  } else {
    Serial.println("DHT11: Sensor reading failed");
  }

  Serial.println("----------------------------");
}

// ---------------- SOIL QUALITY READINGS ----------------

void readSoilSensors() {
  int rainRaw = ads.readADC_SingleEnded(RAIN_CHANNEL);
  int soilRaw = ads.readADC_SingleEnded(SOIL_CHANNEL);

  float rainVoltage = ads.computeVolts(rainRaw);
  float soilVoltage = ads.computeVolts(soilRaw);

  float rainPercentage =
      calculatePercentage(rainRaw, RAIN_DRY_RAW, RAIN_WET_RAW);

  float soilPercentage =
      calculatePercentage(soilRaw, SOIL_DRY_RAW, SOIL_WET_RAW);

  Serial.println("----- SOIL QUALITY DATA -----");

  Serial.print("Rain Sensor Raw Value: ");
  Serial.println(rainRaw);

  Serial.print("Rain Sensor Voltage: ");
  Serial.print(rainVoltage, 3);
  Serial.println(" V");

  Serial.print("Rain Level: ");
  Serial.print(rainPercentage, 1);
  Serial.println(" %");

  Serial.print("Soil Moisture Raw Value: ");
  Serial.println(soilRaw);

  Serial.print("Soil Moisture Voltage: ");
  Serial.print(soilVoltage, 3);
  Serial.println(" V");

  Serial.print("Soil Moisture: ");
  Serial.print(soilPercentage, 1);
  Serial.println(" %");

  Serial.println("-----------------------------");
}

// ---------------- SETUP ----------------

void setup() {
  Serial.begin(115200);

  pinMode(AIR_MOSFET_PIN, OUTPUT);
  pinMode(SOIL_MOSFET_PIN, OUTPUT);

  pinMode(AIR_LED_PIN, OUTPUT);
  pinMode(SOIL_LED_PIN, OUTPUT);

  digitalWrite(AIR_MOSFET_PIN, LOW);
  digitalWrite(SOIL_MOSFET_PIN, LOW);

  digitalWrite(AIR_LED_PIN, LOW);
  digitalWrite(SOIL_LED_PIN, LOW);

  Wire.begin(D2, D1);  // SDA, SCL

  dht.begin();

  if (!ads.begin(0x48)) {
    Serial.println("ERROR: ADS1115 not detected!");
    while (1) {
      delay(1000);
    }
  }

  // Suitable measurement range for signals limited to ADS1115 supply.
  ads.setGain(GAIN_ONE);

  Serial.println("\nIntelligent Dual-Mode Environmental Monitoring System");
  Serial.println("ESP8266 + ADS1115 + MOSFET Switching");

  switchToAirMode();
}

// ---------------- MAIN LOOP ----------------

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastModeChange >= MODE_INTERVAL) {
    if (airMode) {
      switchToSoilMode();
    } else {
      switchToAirMode();
    }
  }

  if (airMode) {
    readAirSensors();
  } else {
    readSoilSensors();
  }

  delay(5000);
}
