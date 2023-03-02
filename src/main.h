#include <Arduino.h>
#include <WiFi.h>
#include <NimBLEDevice.h>

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

// Custom libs
#include <Shelly.h>
#include <globals.h>


// iBeacon
#include <NimBLEBeacon.h>
#define Sprintf(f, ...) ({ char* s; asprintf(&s, f, __VA_ARGS__); String r = s; free(s); r; })
#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

// DateTime lib and conig for boot info
#include "time.h"

#ifndef FIRMWARE_VERSION
    #define FIRMWARE_VERSION "VersionNotSetInBuildFlags"
    #warning FIRMWARE_VERSION was not set in build flags!
#endif

NimBLEScan* pBLEScan;

DNSServer dnsServer;

AsyncWebServer server(80);

Shelly *shelly;


// ------------------------ Predefined in globals ------------------------

AsyncMqttClient mqttClient;
Preferences preferences;
String deviceName;

int mqttIgnoreCounter = 0;
unsigned long mqttDisableTime = 0;
bool mqttDisabled = false;
TOPIC_GLOBAL TopicGlobal;

// ------------------------ DateTime Configuration ------------------------

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

/*
Area            HostName
Worldwide	    pool.ntp.org
Asia	        asia.pool.ntp.org
Europe	        europe.pool.ntp.org
North America	north-america.pool.ntp.org
Oceania	        oceania.pool.ntp.org
South America	south-america.pool.ntp.org

%A	returns day of week
%B	returns month of year
%d	returns day of month
%Y	returns year
%H	returns hour
%M	returns minutes
%S	returns seconds
*/


// ------------------------ Information ------------------------

//char firmware_char_array[] = AUTO_VERSION;

struct INFO {
    String bootTime;
    String model;
    String toString(){ return "{ \"BootTime\": \"" + bootTime + "\", \"Version\": \"" + FIRMWARE_VERSION + "\", \"Model\": \"" + model + "\" }"; }
} Info;


// ------------------------ BLE Filter config ------------------------

String sFilterBle = "c0:6d:62:e7:4e:7a=gts4mini,e2:46:43:e2:2d:21=brieftasche,745ed2ff-f9e8-4a93-a634-b733598c16f0-0-0=pixel7";

std::vector<String> vecFilterBle;
std::vector<String> vecFilterAlias;

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
struct TOPIC_MAIN {
    String Filter,
    Results,
    Online,
    Ip,
    Config,
    Restart,
    HardReset,
    Info;
} Topic;


bool wifiWasConnected = false;
bool captivePortalActivated = false;

String deviceModel = "";


// ##########################################################################
// ------------------------ Shelly related variables ------------------------
// ##########################################################################

struct SHELLYP_2PM015 {
    int ButtonReset = 27;
    std::vector<int> Switch{ 2, 18 };
    std::vector<int> Relay{ 13, 12 };
    int I2C_SCL = 25;
    int I2C_SDA = 33;
    int ADE7953 = 36;
    int Temperature = 37;

    std::vector<String> SwitchMode{ "Switch", "Switch" };
    String Config = "{ \"Switch1_Mode\": \"Switch\", \"Switch2_Mode\": \"Switch\", \"SwapInput\": false, \"SwapOutput\": false ";
};

struct SHELLYP_1 {
    int ButtonReset = 25;
    std::vector<int> Switch{ 4 };
    std::vector<int> Relay{ 26 };
    int Relay1 = 26;
    int Temperature = 32;

    std::vector<String> SwitchMode{ "Switch" };
    String Config = "{ \"Switch1_Mode\": \"Switch\" ";
};

struct SHELLYP_I4 {
    int ButtonReset = -1;
    std::vector<int> Switch{ 12, 14, 27, 26 };
    std::vector<int> Relay;

    std::vector<String> SwitchMode{ "Detached", "Detached", "Detached", "Detached" };
    String Config = "{ \"Config\": \"ShellyPlus-i4\" ";
};






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

int PinSwitchR, PinSwitch1, PinSwitch2, PinRelay1, PinRelay2, PinSCL, PinSDA, PinADE7953;

String switchMode1, switchMode2; // possible switchMode: BUTTON or SWITCH or DETACHED



