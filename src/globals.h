#pragma once

#include <Arduino.h>
#include <AsyncMqttClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

/*
####################################################
extern defined variables / objects are set in main.h
####################################################
*/

// helper to temp. ignore arriving MQTT mesages
extern int mqttIgnoreCounter;
extern unsigned long mqttDisableTime;
extern bool mqttDisabled;

extern AsyncMqttClient mqttClient;
extern Preferences preferences;
extern String deviceName;

struct TOPIC_GLOBAL {
    String dbg,
    Main,
    Message,
    Device;
};

extern TOPIC_GLOBAL TopicGlobal;

/*
####################################################
####################################################
*/

bool pub(String topic, String payload, bool ignoreReceivings=false, uint8_t qos = 0, bool retain = false, size_t length = 0, bool dup = false, uint16_t message_id = 0);
bool pub(String topic, int payload, bool ignoreReceivings=false, uint8_t qos = 0, bool retain = false, size_t length = 0, bool dup = false, uint16_t message_id = 0);

bool pubFast(String topic, String payload, bool ignoreReceivings=false, uint8_t qos = 0, bool retain = false, size_t length = 0, bool dup = false, uint16_t message_id = 0);

void report( String msg, bool withPub=true);


//String boolToString(bool b);

bool stringToBool(String s);

void clearPreferences( const char* space="shelly");

int readInt( const char* key, int defVal, const char* space="shelly");

void writeInt( const char* key, int value, const char* space="shelly");

String readString( const char* key, String defVal, const char* space="shelly");

void writeString( const char* key, String value, const char* space="shelly");

void restartDevice();

