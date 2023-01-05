#include <Arduino.h>
#include <utils.h>
#include <user_config.h>

namespace Shelly {


const uint8_t PinSwitch1 = 5;
const uint8_t PinSwitch2 = 18;
const uint8_t PinRelay1  = 13;
const uint8_t PinRelay2  = 12;

int debounce = 100; // debounce time in ms

//variables to keep track of the timing of recent changes from switches(=debounce)
unsigned long switchTime1 = 0;
unsigned long switchLastTime1 = 0;
bool switchState1, switchLastState1;
unsigned long switchTime2 = 0;
unsigned long switchLastTime2 = 0;
bool switchState2, switchLastState2;


void onMqttConnect() {
    pub( topicRelay1, "false");
    pub( topicRelay2, "false");
    pub( topicRelaySet1, "false");
    pub( topicRelaySet2, "false");
    pub( topicSwitch1, "false");
    pub( topicSwitch2, "false");
    pub( topicSwitch1, BoolToString( switchState1) );
    pub( topicSwitch2, BoolToString( switchState2) ); 
    mqttClient.subscribe(topicRelaySet1.c_str(), 1);
    mqttClient.subscribe(topicRelaySet2.c_str(), 1);
}

bool Command(String& command, String& pay) {

    if (command == "a") {
        ;
    } else if (command == "b") {
        ;
    } else
        return false;
    return true;

}


void setRelay( byte relay, bool state){

    if ( devMode == "LIGHT"){
        if ( relay == 1){
            digitalWrite(PinRelay1, BoolToPin( state) );
            Serial.println( "Set Relay 1 to: " + BoolToString( state) );
            delay(100);
            pub( topicRelay1, ( digitalRead( PinSwitch1) == HIGH) ? "true" : "false" );
        }
        else if ( relay == 2){
            digitalWrite(PinRelay2, BoolToPin( state) );
            Serial.println( "Set Relay 2 to: " + BoolToString( state) );
            delay(100);
            pub( topicRelay2, ( digitalRead( PinSwitch2) == HIGH) ? "true" : "false" );
        }
    }

    else { // COVER is always default for security reasons!
        if ( relay == 1){
            if ( state){
                // Be sure to open relay 2 before close relay1 !!!
                digitalWrite(PinRelay2, LOW);
                Serial.println( "Set Relay 2 to: false");
                pub( topicRelay2, "false");
                delay( 500);

                digitalWrite(PinRelay1, HIGH );
                Serial.println( "Set Relay 1 to: true");
                pub( topicRelay1, "true");                
            }
            else {
                digitalWrite(PinRelay1, LOW);
                Serial.println( "Set Relay 1 to: false");
                pub( topicRelay1, "false");
            }
        }
        else if ( relay == 2){

        }
    }
}


void toggleRelay( byte relay){
    if ( relay == 1){
        digitalWrite(PinRelay1, !digitalRead(PinRelay1));
        Serial.println( "Toggle Relay 1");
    }
    else if ( relay == 2){
        digitalWrite(PinRelay2, !digitalRead(PinRelay2));
        Serial.println( "Toggle Relay 2");
    }
}


void loopCheckSw1() {
    switchTime1 = millis();
    switchState1 = ( digitalRead( PinSwitch1) == HIGH ) ? true : false;
    if ( switchTime1 - switchLastTime1 > debounce && switchState1 != switchLastState1 ){
        Serial.println( "Switch1 changed to state: " + BoolToString( switchState1) );
        pub( topicSwitch1, BoolToString( switchState1) );
        switchLastTime1 = switchTime1;
        switchLastState1 = switchState1;

    }
}


void loopCheckSw2() {
    switchTime2 = millis();
    switchState2 = ( digitalRead( PinSwitch2) == HIGH ) ? true : false;
    if ( switchTime2 - switchLastTime2 > debounce && switchState2 != switchLastState2 ){
        Serial.println( "Switch2 changed to state: " + BoolToString( switchState2) );
        pub( topicSwitch2, BoolToString( switchState2) );
        switchLastTime2 = switchTime2;
        switchLastState2 = switchState2;
    }
}


void Setup() {

    // --------------------- Init Switches ---------------------------------------------

    pinMode( PinSwitch1, INPUT_PULLDOWN);
    switchState1 = switchLastState1 = ( digitalRead( PinSwitch1) == HIGH ) ? true : false;
    pinMode( PinSwitch2, INPUT_PULLDOWN);
    switchState2 = switchLastState2 = ( digitalRead( PinSwitch1) == HIGH ) ? true : false;

    // --------------------- Init Relais ----------------------------------------------

    pinMode( PinRelay1, OUTPUT);
    pinMode( PinRelay2, OUTPUT);

}

void Loop() {

    loopCheckSw1();
    loopCheckSw2();

}

}  // namespace Shelly