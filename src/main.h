#include <Arduino.h>
#include <WiFi.h>
#include <NimBLEDevice.h>
extern "C" {
	#include "freertos/FreeRTOS.h"
	#include "freertos/timers.h"
}

// OTA dependencies
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <AsyncElegantOTA.h>

// Captive Portal dependecies
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <DNSServer.h>

extern const uint8_t index_html[] asm("_binary_src_index_html_start");
const String html = String((const char*)index_html);

// Custom libs
#include <Shelly.h>
#include <BleDevice.h>
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

TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
TimerHandle_t mqttRePublishAgain;
bool firstConnectAfterBoot = false;

NimBLEScan* pBLEScan;
bool scanAutostart = true;

DNSServer dnsServer;

AsyncWebServer server(80);

Shelly *shelly;

bool captivePortalActivated = false;

String deviceModel = "";

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
Germany         de.pool.ntp.org
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


// ------------------------ BLE config ------------------------

// Default filter entry
String sFilterBle = "c0:6d:62:e7:4e:7a=gts4mini,e2:46:43:e2:2d:21=brieftasche,745ed2ff-f9e8-4a93-a634-b733598c16f0-0-0=pixel7";

float BleAbsorbtion = 3.5f; // default value

bool bleDevicesRssiInitialized = false;

bool bleDevicesInitPubSubDone = false;

std::vector<BleDevice> vecBleDevices;

// ------------------------ MQTT variables ------------------------
struct TOPIC_MAIN {
    String Filter,
    Absorbtion,
    Online,
    Ip,
    Config,
    Restart,
    HardReset,
    Info;
} Topic;

unsigned long bleDevicesPubSubTime = 0;
