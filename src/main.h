
#include <Arduino.h>
#include <WiFi.h>
#include <NimBLEDevice.h>
#include <AsyncMqttClient.h>
#include <Preferences.h>
#include <ArduinoJson.h>

// OTA dependencies
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <AsyncElegantOTA.h>

// Captive Portal dependecies
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <html.h>

NimBLEScan* pBLEScan;

//NimBLEAddress a("c2:2d:9f:fe:46:ec");

AsyncMqttClient mqttClient;
Preferences preferences;

DNSServer dnsServer;
AsyncWebServer server(80);

// ------------------------ Shelly Plus 2PM Cover as default config ------------------------

String devMode    = "";  // possible devMode: LIGHT or COVER, is set in init WebUI

String config = "{ \"Config\": \"Shelly Plus 2PM v0.1.9\", \"ButtonReset\": 4, \"Switch1\": 5, \"Switch1_Mode\": \"Switch\", \"Switch2\": 18, \"Switch2_Mode\": \"Switch\", \"Relay1\": 13, \"Relay2\": 12, \"I2C_SCL\": 25, \"I2C_SDA\": 26, \"ADE7953\": 27, \"Temperature\": 35 }";

// ------------------------ BLE Filter config ------------------------

String filterBle = "c2:2d:9f:fe:46:ec,e8:61:51:5f:f0:2e";

int arrRssi[10][3] = {
    { -150, -150, -150},
    { -150, -150, -150},
    { -150, -150, -150},
    { -150, -150, -150},
    { -150, -150, -150},
    { -150, -150, -150},
    { -150, -150, -150},
    { -150, -150, -150},
    { -150, -150, -150},
    { -150, -150, -150}
};

// ------------------------ MQTT variables ------------------------

String topicMain, deviceName, topicFilter, topicDevice, topicResults, topicMessage, topicOnline, topicIp, topicConfig, topicRestart, topicHardReset, topicSwitch1, topicSwitch2, topicRelaySet1, topicRelaySet2, topicCoverTime, topicPosSet, topicCoverStop;

// ------------------------ Shelly related variables ------------------------

StaticJsonDocument<400> doc; // Needed for JSON config over mqtt

uint debounce = 100; // debounce switch input in ms

// Needed only in COVER mode
unsigned long coverStartTime = 0;  // Time when COVER was triggered to go UP/DOWN
unsigned long coverTargetTime = 0; // Max time, when end position should be reached
String coverDirection = "STOPPED";
int coverMaxTime  = 100;           // cnfigured over MQTT and saved in non-volatile memory
int coverPosition = 100;           // default value; real value from non-volatile memory


//variables to keep track of the timing of recent changes from switches(=debounce)
unsigned long switchTime1 = 0;
unsigned long switchLastTime1 = 0;
bool switchState1, switchLastState1;
unsigned long switchTime2 = 0;
unsigned long switchLastTime2 = 0;
bool switchState2, switchLastState2;

unsigned long switchTimeR = 0;
unsigned long switchLastTimeR = 0;
bool switchStateR, switchLastStateR;

unsigned long switchTimeLongPressR = 0;
bool flagLongPress = false;

int PinSwitchR, PinSwitch1, PinSwitch2, PinRelay1, PinRelay2, PinADE7953;

String switchMode1, switchMode2; // possible switchMode: BUTTON or SWITCH or DETACHED

// ------------------------ Device related variables ------------------------

bool wifiWasConnected = false;
bool captivePortalActivated = false;

// helper to temp. ignore arriving MQTT mesages
unsigned long mqttDisableTime = 0;
bool mqttDisabled = false;
