#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h> // For Private MQTT Sever
#include <Wire.h>
#include "config.h" // Configuration file for WiFi & MQTT

// Pins definition
const int POT_PIN = 36;
const int LED_PIN = 2;

const float THRESHOLD_ALERT = 15.0; // Change this value to adjust the alert threshold

// Control Objects
LiquidCrystal_I2C lcd(0x27, 16, 2);
WiFiClientSecure espClient;
PubSubClient client(espClient);

unsigned long lastMsg = 0;

void setup_wifi()
{
  delay(10);
  lcd.clear();
  lcd.print("WiFi Connecting...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    lcd.print(".");
  }
  lcd.clear();
  lcd.print("WiFi Connected!");
  espClient.setInsecure();
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect with client id "ESP32Client01" and password on config file
    if (client.connect("ESP32Client01", mqtt_username, mqtt_password)) 
    {
      Serial.println("connected");
      lcd.setCursor(0, 1);
      lcd.print("MQTT Online");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT); // Set the LED pin as output

  // Initialize the LCD
  lcd.init();
  lcd.backlight();

  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  delay(1000);
  lcd.clear();
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  // Convert the raw value to amps
  int rawValue = analogRead(POT_PIN);
  // currentAmps = rawValue * (maxAmps / maxAnalogValue) | [I = V / R]
  float currentAmps = (rawValue * 20.0) / 4095.0;
  String status = (currentAmps >= THRESHOLD_ALERT) ? "OVERLOAD" : "NORMAL";

  // Display on LCD & LED
  if (status == "OVERLOAD")
  {
    lcd.setCursor(0, 0);
    lcd.print("!!OVERLOAD!!");
    digitalWrite(LED_PIN, HIGH);
    // Blinking effect
    if ((millis() / 500) % 2 == 0)
    {
      lcd.noBacklight();
    }
    else
    {
      lcd.backlight();
    }
  }
  else
  {
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("NORMAL      ");
    digitalWrite(LED_PIN, LOW);
  }

  lcd.setCursor(0, 1);
  lcd.print("Current: ");
  lcd.print(currentAmps, 1);
  lcd.print(" A   ");

  // Send MQTT message every 5 seconds
  unsigned long now = millis();
  if (now - lastMsg > 5000)
  {
    lastMsg = now;

    String payload = "{\"currentAmps\":" + String(currentAmps, 2) + ",\"status\":\"" + status + "\"}";
    client.publish("factory/sensor01/energy", payload.c_str());
    Serial.println("Published: " + payload);
  }
}