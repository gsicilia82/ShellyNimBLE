
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

// Powermeter
#include <ADE7953.h>

// iBeacon
#include <NimBLEBeacon.h>
#define Sprintf(f, ...) ({ char* s; asprintf(&s, f, __VA_ARGS__); String r = s; free(s); r; })
#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))


NimBLEScan* pBLEScan;

AsyncMqttClient mqttClient;

Preferences preferences;

DNSServer dnsServer;

AsyncWebServer server(80);

ADE7953 myADE7953;

ENERGY Energy;

// ------------------------ Shelly Plus 2PM Cover as default config ------------------------

String devMode    = "";  // possible devMode: LIGHT or COVER, is set in init WebUI

String config = "{ \"Config\": \"Shelly Plus 2PM v0.1.9\", \"ButtonReset\": 4, \"Switch1\": 5, \"Switch1_Mode\": \"Switch\", \"Switch2\": 18, \"Switch2_Mode\": \"Switch\", \"Relay1\": 13, \"Relay2\": 12, \"I2C_SCL\": 25, \"I2C_SDA\": 26, \"ADE7953\": 27, \"Temperature\": 35 }";

// ------------------------ BLE Filter config ------------------------

String sFilterBle = "c1:e5:19:e6:bd:14,e2:46:43:e2:2d:21,745ed2ff-f9e8-4a93-a634-b733598c16f0-0-0";

std::vector<String> vecFilterBle;

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
String deviceName;

struct TOPIC {
    String dbg,
    Main,
    Filter,
    Device,
    Results,
    Message,
    Online,
    Ip,
    Config,
    Restart,
    HardReset,
    Switch1,
    Switch2,
    RelaySet1,
    RelaySet2,
    CoverState,
    CoverPosSet,
    CoverStop,
    CoverCalib,
    Power1,
    Power2,
    PowerAcc;
    
} Topic;

// ##########################################################################
// ------------------------ Device related variables ------------------------
// ##########################################################################

bool wifiWasConnected = false;
bool captivePortalActivated = false;

// helper to temp. ignore arriving MQTT mesages
int mqttIgnoreCounter = 0;
unsigned long mqttDisableTime = 0;
bool mqttDisabled = false;

// ##########################################################################
// ------------------------ Shelly related variables ------------------------
// ##########################################################################

StaticJsonDocument<400> doc; // Needed for JSON config over mqtt

uint debounce = 100; // debounce switch input in ms

bool measEnergy = false;
int measIntervall;

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


// ##########################
// Needed only in COVER mode
// ##########################

unsigned long coverStartTime = 0;  // Time when COVER was triggered to go UP/DOWN
unsigned long coverTargetTime = 0; // Max time, when end position should be reached
String coverDirection = "stopped";
int coverMaxTime  = 100;           // cnfigured over MQTT and saved in non-volatile memory
int coverPosition = 50;            // default value; real value from non-volatile memory

enum {
    NOT_CALIBRATED,
    RAISE_1ST_CHKPWR_0W,
    UP_REACHED_1ST,
    LOWER,
    LOWER_CHKPWR_20W,
    LOWER_CHKPWR_0W,
    DOWN_REACHED,
    RAISE_2ND,
    RAISE_2ND_CHKPWR_20W,
    RAISE_2ND_CHKPWR_0W,
    UP_REACHED_2ND,
    CALIBRATED
};

#ifdef DEBUG
    // Used in Serial.print for debug purposes
    String CalibState[] = {
        "NOT_CALIBRATED",
        "RAISE_1ST_CHKPWR_0W",
        "UP_REACHED_1ST",
        "LOWER",
        "LOWER_CHKPWR_20W",
        "LOWER_CHKPWR_0W",
        "DOWN_REACHED",
        "RAISE_2ND",
        "RAISE_2ND_CHKPWR_20W",
        "RAISE_2ND_CHKPWR_0W",
        "UP_REACHED_2ND",
        "CALIBRATED"
    };
#endif

int coverCalibState = NOT_CALIBRATED;
unsigned long calibTimer1  = 0;
unsigned long calibTimer2  = 0;
unsigned long calibStepTimer = 0;
bool isCalibWaiting = false;

// Watt limits for calibration
int limPowHigh = 40;
int limPowLow = 15;

// #########################
// #########################
