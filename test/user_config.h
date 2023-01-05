#pragma once
#include <Arduino.h>

String deviceName = "master";
String topicMain  = "shellyscanner";
String devMode = "LIGHT"; // possible devMode: LIGHT or COVER




/**
 * ----- topicMain ---
 *                    results ---
 *                               deviceName ---
 *                                             MAC1* = rssi1
 *                                             MAC2* = rssi2
 *                    devices ---
 *                               deviceName ---
 *                                             topicOnline
 *                                             topicIp
 *                                             topicRelay1
 *                                             topicRelay2
 *                                             topicRelaySet1
 *                                             topicRelaySet2
 *                                             topicSwitch1
 *                                             topicSwitch2
 * 
 *    ( * = depends from scan results )
*/

// topicMain and deviceName are defined in user_config_h

String topicDevice  = topicMain   + "/devices/" + deviceName;
String topicResults = topicMain   + "/results/" + deviceName;

String topicOnline     = topicDevice + "/online"; // true || false
String topicIp         = topicDevice + "/ip_address";
String topicRelay1     = topicDevice + "/relay1";
String topicRelay2     = topicDevice + "/relay2";
String topicRelaySet1  = topicDevice + "/relaySet1";
String topicRelaySet2  = topicDevice + "/relaySet2";
String topicSwitch1    = topicDevice + "/switch1";
String topicSwitch2    = topicDevice + "/switch2";