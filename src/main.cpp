#include <Arduino.h>
#include <WiFi.h>          // Replace with WiFi.h for ESP32
#include <WebServer.h>     // Replace with WebServer.h for ESP32
#include <AutoConnect.h>
#include <ArduinoOSCWiFi.h>
#include <string>
#include "driver/adc.h"
#include <esp_bt.h>


WebServer         Server;          // Replace with WebServer for ESP32
AutoConnect       Portal(Server);
AutoConnectConfig Config("LEFT HAND CONTROLLER", "12345678");
// AutoConnectConfig Config("RIGHT HAND CONTROLLER", "12345678");


const char* host = "192.168.1.27";
const int publish_port = 12000;

#define buttons_max 4
#define samples 3
char button_map[buttons_max] = {4, 12, 2, 15};
// char button_map[buttons_max] = {12, 14, 27, 33};

uint16_t pressed[buttons_max] = {0};
uint8_t sampled_values[buttons_max][samples] = {0};
uint32_t values[buttons_max] = {0};
uint8_t button_thresolds[buttons_max] = {40, 45, 40, 40};
uint8_t current_sample = 0;

void rootPage() {
  char content[] = "Hello, world";
  Server.send(200, "text/plain", content);
}

void setup() {
  delay(1000);
  pinMode(22, OUTPUT);
  setCpuFrequencyMhz(240);
  Serial.begin(9600);
  Serial.println();
  Portal.config(Config);
  btStop();
  esp_bt_controller_disable();


  Server.on("/", rootPage);
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
    for (int i = 0; i < buttons_max; i++) {
      String channel_pressed = "/left_controller/button_" + String(i) + "/pressed";
      OscWiFi.publish(host, publish_port, channel_pressed, pressed[i])
        ->setIntervalMsec(1);
      String channel_value = "/left_controller/button_" + String(i) + "/value";
      OscWiFi.publish(host, publish_port, channel_value, values[i])
        ->setIntervalMsec(1);
    }
  }
}

void loop() {
  Portal.handleClient();
  digitalWrite(22, LOW);

  if (WiFi.status() == WL_CONNECTED) {
    for (int i = 0; i < buttons_max; i++) {
      sampled_values[i][current_sample] = touchRead(button_map[i]);
    }
    current_sample++;
    if (current_sample == samples) {
      for (int i = 0; i < buttons_max; i++) {
        for (int j = 0; j < samples; j++) {
          values[i] += sampled_values[i][j];
        }
        values[i] = values[i] / samples;
        pressed[i] = values[i] < button_thresolds[i];
      }
      current_sample = 0;
    }

    OscWiFi.update(); 
  }
}