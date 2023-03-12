#include <globals.h>

bool pub(String topic, String payload, bool ignoreReceivings /*= false*/, uint8_t qos /*= 0*/, bool retain /*= false*/, size_t length /*= 0*/, bool dup /*= false*/, uint16_t message_id /*= 0*/){

    if ( ignoreReceivings) {
        mqttIgnoreCounter++;
        #ifdef VERBOSE_MQTT
            Serial.print( "MQTT, pub: " + topic);
            Serial.printf("| Ignore next <%d> incoming messages!\n", mqttIgnoreCounter);
        #endif
        mqttDisableTime = millis();
        mqttDisabled = true;
    }
    for (int i = 0; i < 10; i++){
        if ( mqttClient.publish(topic.c_str(), qos, retain, payload.c_str(), length, dup, message_id) ){
            return true;
        }
        delay(25);
    }
    return false;

}

bool pub(String topic, int payload, bool ignoreReceivings /*= false*/, uint8_t qos /*= 0*/, bool retain /*= false*/, size_t length /*= 0*/, bool dup /*= false*/, uint16_t message_id /*= 0*/){

    String sPayload;
    if ( payload == 0) sPayload = "false";
    else sPayload = "true";
    return pub( topic, sPayload, ignoreReceivings , qos, retain, length, dup, message_id);
}

bool pubFast(String topic, String payload, bool ignoreReceivings /*= false*/, uint8_t qos /*= 0*/, bool retain /*= false*/, size_t length /*= 0*/, bool dup /*= false*/, uint16_t message_id /*= 0*/){

    for (int i = 0; i < 10; i++){
        if ( mqttClient.publish(topic.c_str(), qos, retain, payload.c_str(), length, dup, message_id) ){
            return true;
        }
        delay(25);
    }
    return mqttClient.publish(topic.c_str(), qos, retain, payload.c_str(), length, dup, message_id);

}

void report( String msg, bool withPub /*= true*/){
    Serial.println( msg);
    if ( withPub) pub( TopicGlobal.Message, msg);
}


//String boolToString(bool b){ return b ? "true" : "false"; }

bool stringToBool(String s){ return s == "true" ? true : false; }

void clearPreferences( const char* space /*= "shelly"*/){
    report( "Local memory cleared!");
    preferences.begin( space, false);
    preferences.clear();
    preferences.end();
}

int readInt( const char* key, int defVal, const char* space /*= "shelly"*/){
    preferences.begin( space, false);
    int tmp = preferences.getInt( key, defVal);
    preferences.end();
    return tmp;
}

void writeInt( const char* key, int value, const char* space /*= "shelly"*/){
    preferences.begin( space, false);
    preferences.putInt( key, value);
    preferences.end();
}

String readString( const char* key, String defVal, const char* space /*= "shelly"*/){
    preferences.begin( space, false);
    String tmp = preferences.getString( key, defVal);
    preferences.end();
    return tmp;
}

void writeString( const char* key, String value, const char* space /*= "shelly"*/){
    preferences.begin( space, false);
    preferences.putString( key, value);
    preferences.end();
}

void restartDevice(){
    report("Restarting device in 1s ...");
    delay(1000);
    ESP.restart();
}
