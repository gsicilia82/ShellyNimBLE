#include <Shelly.h>

/*
************************************************************
START: Parent Class Shelly
************************************************************
*/

Shelly::Shelly(){}

//void Shelly::setup(){}

bool Shelly::getRelayState ( int relay){
    return ( ( digitalRead( Pin.Relay[ relay]) == HIGH) ? true : false );
}


void Shelly::initMqttTopics(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly::initMqttTopics()");
        Serial.println(">>> Pin.Input 0: " + Pin.Input[0] );
    #endif

    Topic.UserConfig = TopicGlobal.Device + "/Config";
    for(int i : Pin.Input){
        Topic.Input.push_back( TopicGlobal.Device + "/Switch" + String(i+1) );
    }
    for(int i : Pin.Relay){
        Topic.Relay.push_back( TopicGlobal.Device + "/SetRelay" + String(i+1) );
    }

}

void Shelly::initPubSub(){

    for (int i = 0; i < 2; i++) {
        for(int i : Pin.Input){
            pub( Topic.Input[i], ( digitalRead( Pin.Input[i]) == HIGH) ? "true" : "false");
        }
        for(int i : Pin.Relay){
            pub( Topic.Relay[i], ( digitalRead( Pin.Relay[i]) == HIGH) ? "true" : "false");
        }
        if( i==0) delay(500);
    }
    for(int i : Pin.Relay){
        mqttClient.subscribe(Topic.Relay[i].c_str(), 1);
    }
}

bool Shelly::parseJsonUserConfig(){
    return true;
}

void Shelly::loop(){
    /* Handle Input and Relay*/
}


/*
************************************************************
END: Parent Class Shelly
************************************************************
START: Class Shelly Plus 2PM
************************************************************
*/

void Shelly2PM::setup(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly2PM::setup()");
    #endif

    deviceMode.Setting = readString( "mode", "Cover"); // Default setting is "Cover" for security reasons

    String tmp = readString( "input1", "");
    if ( tmp == ""){
        report( "Input SwitchMode 1 setting is empty! Remains to default value 'Switch'");
    } else {
        UserConfig.ModeInputX[0] = tmp;
    }

    tmp = readString( "input2", "");
    if ( tmp == ""){
        report( "Input SwitchMode 2 setting is empty! Remains to default value 'Switch'");
    } else {
        UserConfig.ModeInputX[1] = tmp;
    }
    
    // Set Cover specific values
    if ( deviceMode.Setting == deviceMode.Cover){
        coverCalibState = readInt( "coverCalibState", coverCalibState);
        coverMaxTime = readInt( "coverMaxTime", coverMaxTime);
        
        if ( coverCalibState < CALIBRATED){
            coverPosition = 50;
            writeInt( "coverPosition", coverPosition);
        }
        else {
            coverPosition = readInt( "coverPosition", coverPosition);
        }
    }

    initMqttTopics();
    initPubSub();
}

void Shelly2PM::initMqttTopics(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly2PM::initMqttTopics()");
    #endif

    Shelly::initMqttTopics();

    Topic.Power1    = TopicGlobal.Device + "/Power1";
    Topic.Power2    = TopicGlobal.Device + "/Power2";
    Topic.PowerAcc  = TopicGlobal.Device + "/PowerAcc";

    if ( deviceMode.Setting == deviceMode.Cover){
        Topic.CoverPosSet = TopicGlobal.Device + "/CoverSetPosition";
        Topic.CoverStop   = TopicGlobal.Device + "/CoverStop";
        Topic.CoverCalib  = TopicGlobal.Device + "/CoverStartCalibration";
        Topic.CoverState  = TopicGlobal.Device + "/CoverState";
        Topic.Relay[0] = TopicGlobal.Device + "/CoverUp";
        Topic.Relay[1] = TopicGlobal.Device + "/CoverDown";
    }

}

void Shelly2PM::initPubSub(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly2PM::initPubSub()");
    #endif

    Shelly::initPubSub();
    String JsonUserConfig = getJsonUserConfig();

    for ( int i = 0; i < 2; i++) {
        pub( Topic.UserConfig, JsonUserConfig );

        pub( Topic.Power1   , "0");
        pub( Topic.Power2   , "0");
        pub( Topic.PowerAcc , "0");

        if ( deviceMode.Setting == deviceMode.Cover){
            pub( Topic.CoverPosSet, String( coverPosition) );
            pub( Topic.CoverStop, "false");
            pub( Topic.CoverCalib, "false");
            pub( Topic.CoverState, "stopped");
        }
        if( i==0) delay(500);
    }
    mqttClient.subscribe(Topic.UserConfig.c_str(), 1);

    if ( deviceMode.Setting == deviceMode.Cover){
        mqttClient.subscribe(Topic.CoverPosSet.c_str(), 1);
        mqttClient.subscribe(Topic.CoverStop.c_str(), 1);
        mqttClient.subscribe(Topic.CoverCalib.c_str(), 1);
    }
}

String Shelly2PM::getJsonUserConfig(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly2PM::getJsonUserConfig()");
    #endif

    StaticJsonDocument<200> json;
    String JsonUserConfig;
    json[ "Mode_Input1"] = UserConfig.ModeInputX[0];
    json[ "Mode_Input2"] = UserConfig.ModeInputX[1];
    json[ "SwapInput"] = UserConfig.SwapInput;
    json[ "SwapOutput"] = UserConfig.SwapOutput;
    serializeJson( json, JsonUserConfig);

    return JsonUserConfig;
}

bool Shelly2PM::parseJsonUserConfig(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly2PM::parseJsonUserConfig()");
    #endif

    return true;
}
    
void Shelly2PM::loop(){
    
}
