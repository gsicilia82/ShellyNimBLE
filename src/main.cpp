#include <main.h>





/*


bool convertJsonToConfig( String& config, bool withPub=true){

    StaticJsonDocument<400> doc; // Needed for JSON config over mqtt

    DeserializationError error = deserializeJson(doc, config.c_str() );

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return false;
    }

    int tmpPin;
    const char* tmpChSwitchMode;
    String tmpStrSwitchMode;

    tmpPin = doc["ButtonReset"];
    if ( tmpPin == 0 || tmpPin < -1 || tmpPin > 38){
        report( "ERROR: Non valid pin received for ButtonReset!", withPub);
        return false;
    }
    else {
        PinSwitchR  = tmpPin;
    }

    tmpPin = doc["Switch1"];
    if ( tmpPin == 0 || tmpPin < -1 || tmpPin > 38) {
        report( "ERROR: Non valid pin received for Switch1!", withPub);
        return false;
    }
    else PinSwitch1  = tmpPin;
    
    tmpPin = doc["Switch2"];
    if ( tmpPin == 0 || tmpPin < -1 || tmpPin > 38) {
        report( "ERROR: Non valid pin received for Switch2!", withPub);
        return false;
    }
    else PinSwitch2  = tmpPin;

    tmpPin = doc["Relay1"];
    if ( tmpPin == 0 || tmpPin < -1 || tmpPin > 38) {
        report( "ERROR: Non valid pin received for Relay1!", withPub);
        return false;
    }
    else PinRelay1   = tmpPin;

    tmpPin = doc["Relay2"];
    if ( tmpPin == 0 || tmpPin < -1 || tmpPin > 38) {
        report( "ERROR: Non valid pin received for Relay2!", withPub);
        return false;
    }
    else PinRelay2   = tmpPin;

    tmpPin = doc["I2C_SCL"];
    if ( tmpPin == 0 || tmpPin < -1 || tmpPin > 38) {
        report( "ERROR: Non valid pin received for I2C_SCL!", withPub);
        return false;
    }
    else PinSCL      = tmpPin;

    tmpPin = doc["I2C_SDA"];
    if ( tmpPin == 0 || tmpPin < -1 || tmpPin > 38) {
        report( "ERROR: Non valid pin received for I2C_SDA!", withPub);
        return false;
    }
    else PinSDA      = tmpPin;

    tmpPin = doc["ADE7953"];
    if ( tmpPin == 0 || tmpPin < -1 || tmpPin > 38) {
        report( "ERROR: Non valid pin received for ADE7953!", withPub);
        return false;
    }
    else PinADE7953  = tmpPin;

    tmpChSwitchMode = doc["Switch1_Mode"];
    tmpStrSwitchMode = String( tmpChSwitchMode);
    tmpStrSwitchMode.toUpperCase();
    if ( tmpStrSwitchMode != "BUTTON" && tmpStrSwitchMode != "SWITCH" && tmpStrSwitchMode != "DETACHED") {
        report( "ERROR: Non valid Mode received for Switch1: <" + tmpStrSwitchMode + ">", withPub);
        return false;
    }
    else switchMode1  = tmpStrSwitchMode;

    tmpChSwitchMode = doc["Switch2_Mode"];
    tmpStrSwitchMode = String( tmpChSwitchMode);
    tmpStrSwitchMode.toUpperCase();
    if ( tmpStrSwitchMode != "BUTTON" && tmpStrSwitchMode != "SWITCH" && tmpStrSwitchMode != "DETACHED") {
        report( "ERROR: Non valid Mode received for Switch2: <" + tmpStrSwitchMode + ">", withPub);
        return false;
    }
    else switchMode2  = tmpStrSwitchMode;


    return true;
}


void calcCoverPosition( String cmd, int coverTargetPosition=100){ //opening, closing, stopped

    #ifdef DEBUG
        Serial.printf("Called calcCoverPosition() with: cmd=%s, coverTargetPosition=%d \n", cmd, coverTargetPosition);
    #endif

    pub( Topic.CoverState, cmd);

    if ( cmd == "stopped"){
        // calc actual position and report/save
        unsigned long deltaTime = millis() - coverStartTime;

        if ( coverCalibState == CALIBRATED && coverDirection == "opening"){
            coverPosition = coverPosition + (int)( deltaTime*100 / ( coverMaxTime*1000) +0.5);
            coverPosition = coverPosition > 100 ? 100 : coverPosition;
        }
        else if ( coverCalibState == CALIBRATED && coverDirection == "closing"){
            coverPosition = coverPosition - (int)( deltaTime*100 / ( coverMaxTime*1000) +0.5);
            coverPosition = coverPosition < 0 ? 0 : coverPosition;
        }
        else{
            coverPosition = 50;
        }
        coverDirection = cmd;
        writeInt( "coverPosition", coverPosition);
        pub( Topic.CoverPosSet, String( coverPosition), true);
        if ( PinADE7953 != -1) measEnergy = false;    // disable power measurement in loop()

    }
    else if ( cmd == "opening"){
        // activate loop statement and set time limit in loop observation
        coverStartTime = millis();
        if ( coverCalibState == CALIBRATED){
            coverTargetTime = coverStartTime + ( coverMaxTime*1000) * ( coverTargetPosition - coverPosition) / 100 ;
            if ( coverTargetPosition == 100) coverTargetTime += 2000;
        }
        else {
            coverTargetTime = coverStartTime + ( 100*1000); // default 100s if not calibrated
        }
        coverDirection = cmd; // enable time measurement in loop()
        if ( PinADE7953 != -1) measEnergy = true;    // enable power measurement in loop()
    }
    else if ( cmd == "closing"){
        // activate loop statement and set time limit in loop observation
        coverStartTime = millis();
        if ( coverCalibState == CALIBRATED){
            coverTargetTime = coverStartTime + ( coverMaxTime*1000) * ( coverPosition - coverTargetPosition) / 100 ;
            if ( coverTargetPosition == 0) coverTargetTime += 2000;
        }
        else {
            coverTargetTime = coverStartTime + ( 100*1000); // default 100s if not calibrated
        }
        coverDirection = cmd; // enable time measurement in loop()
        if ( PinADE7953 != -1) measEnergy = true;    // enable power measurement in loop()
    }

}


bool stopCover(){ // returns false if already stopped, otherwise true

    #ifdef DEBUG
        Serial.printf("Called stopCover(). Actual direction is: %s \n", coverDirection);
    #endif

    if ( coverDirection == "stopped") return false;

    if ( coverDirection == "opening"){
        digitalWrite(PinRelay1, LOW);
        Serial.println( "COVER: Set Relay 1 to: false");
        pub( Topic.RelaySet1, ( digitalRead( PinRelay1) == HIGH) ? "true" : "false" , true);
    }
    else if ( coverDirection == "closing"){
        digitalWrite(PinRelay2, LOW);
        Serial.println( "COVER: Set Relay 2 to: false");
        pub( Topic.RelaySet2, ( digitalRead( PinRelay2) == HIGH) ? "true" : "false" , true);
    }
    if ( PinADE7953 != -1) measEnergy = false;    // disable power measurement in loop()
    calcCoverPosition( "stopped");

    pub( Topic.Power1, "0");
    pub( Topic.Power2, "0");
    pub( Topic.PowerAcc, "0");

    return true;
}


void setRelayCover( byte relay, bool state, int coverTargetPosition=100){

    #ifdef DEBUG
        Serial.printf("Called setRelayCover() with: relay=%d, state=%d, target=%d \n", relay, state, coverTargetPosition);
    #endif

    if ( getRelayState ( relay) == state) return; // Do nothing if relay is in desired state
 
    bool waitAfterSwitch = stopCover(); // If cover was stopped, returns true and wait in next steps

    if ( !state) return; // false = stopped, stopCover() was already done in step before

    if ( waitAfterSwitch) delay(1000);

    if ( relay == 1){ // relay1 means always UP
        if ( coverPosition < 100){
            digitalWrite(PinRelay1, !digitalRead( PinRelay2) ); // Inverted relay2 should be HIGH, more secure as direct HIGH
            calcCoverPosition( "opening", coverTargetPosition);
            Serial.println( "COVER: Set Relay 1 to: true");
        }
        else {
            pub( Topic.Message, "End position already reached!");
        }
        pub( Topic.RelaySet1, ( digitalRead( PinRelay1) == HIGH) ? "true" : "false" , true);
    }
    else if ( relay == 2){// relay2 means always DOWN
        if ( coverPosition > 0){
            digitalWrite(PinRelay2, !digitalRead( PinRelay1) ); // Inverted Relay1 should be HIGH, more secure as direct HIGH
            calcCoverPosition( "closing", coverTargetPosition);
            Serial.println( "COVER: Set Relay 2 to: true");
        }
        else {
            pub( Topic.Message, "End position already reached!");
        }
        pub( Topic.RelaySet2, ( digitalRead( PinRelay2) == HIGH) ? "true" : "false" , true);
    }
}

bool isCalibRunning() { return ( coverCalibState > NOT_CALIBRATED && coverCalibState < CALIBRATED); }

void stopCalibration( String msg=""){
    if ( isCalibRunning() ){
        coverCalibState = NOT_CALIBRATED;
        report( "Cover Calibration stopped. " + msg);
        pub( Topic.CoverCalib, "false", true);
        stopCover();
    }
}

void coverCalibrateRoutine(){

    #ifdef DEBUG_CALIB
        Serial.println("Called coverCalibrateRoutine(). Actual coverCalibState is: " + CalibState[ coverCalibState] );
    #endif


    float fTmp;
    unsigned long lTmp;

    unsigned long calibTimeout = 100000; // Timeout to reach end positions in ms

    isCalibWaiting = false;

    switch( coverCalibState) {
        case NOT_CALIBRATED:
            report("Cover Calibration started! Raise cover up to end position...");
            setRelayCover( 1, true, 100);
            calibStepTimer = millis();
            delay(2000);
            coverCalibState = RAISE_1ST_CHKPWR_0W;
            break;
        case RAISE_1ST_CHKPWR_0W:
            if ( Energy.powerAcc < limPowLow){
                report("Up Position reached!");
                coverCalibState = UP_REACHED_1ST;
            }
            else if ( millis() - calibStepTimer > calibTimeout){
                stopCalibration( "Calibration routine stopped by Timeout!");
                break;
            }
            else break;
        case UP_REACHED_1ST:
            report("Lower cover down to end position. Start timer calib1.");
            setRelayCover( 2, true, 0);
            calibTimer1 = calibStepTimer = millis();
            coverCalibState = LOWER;
        case LOWER:
            report("Verify power measurement in 2s...");
            delay(2000);
            coverCalibState = LOWER_CHKPWR_20W;
            break;
        case LOWER_CHKPWR_20W:
            if ( Energy.powerAcc > limPowHigh){
                report("Power measurement verified! Power [W]: " + String( Energy.powerAcc) );
                coverCalibState = LOWER_CHKPWR_0W;
            }
            else {
                stopCalibration( "Power measurement not ok. Aborting calibration routine!");
                break;
            }
        case LOWER_CHKPWR_0W:
            if ( Energy.powerAcc < limPowLow){
                calibTimer1 = millis() - calibTimer1;
                float fTmp = calibTimer1 / 1000;
                report("Stop timer calib1. Down Position reached after [s]: " + String( fTmp) );
                coverCalibState = DOWN_REACHED;
            }
            else if ( millis() - calibStepTimer > calibTimeout){
                stopCalibration( "Calibration routine stopped by Timeout!");
                break;
            }
            else break;
        case DOWN_REACHED:
            report("Raise cover up to end position. Start timer calib2.");
            setRelayCover( 1, true, 100);
            calibTimer2 = calibStepTimer = millis();
            coverCalibState = RAISE_2ND;
        case RAISE_2ND:
            report("Verify power measurement in 2s...");
            delay(2000);
            coverCalibState = RAISE_2ND_CHKPWR_20W;
            break;
        case RAISE_2ND_CHKPWR_20W:
            if ( Energy.powerAcc > limPowHigh){
                report("Power measurement verified! Power [W]: " + String( Energy.powerAcc) );
                coverCalibState = RAISE_2ND_CHKPWR_0W;
            }
            else {
                stopCalibration( "Power measurement not ok. Aborting calibration routine!");
                break;
            }
        case RAISE_2ND_CHKPWR_0W:
            if ( Energy.powerAcc < limPowLow){
                calibTimer2 = millis() - calibTimer2;
                fTmp = calibTimer2 / 1000;
                report("Stop timer calib2. UP Position reached after [s]: " + String( fTmp) );
                coverCalibState = UP_REACHED_2ND;
                stopCover();
            }
            else if ( millis() - calibStepTimer > calibTimeout){
                stopCalibration( "Calibration routine stopped by Timeout!");
                break;
            }
            else break;
        case UP_REACHED_2ND:
            fTmp = (calibTimer2 - calibTimer1) / 1000;
            report("Calibration Done! DeltaTimer 2-1 [s]: " + String( fTmp) );
            coverCalibState = CALIBRATED;
            writeInt( "coverCalibState", coverCalibState);
            writeInt( "coverPosition", 100);
            coverPosition = 100;
            lTmp  = ( calibTimer1 > calibTimer2 ? calibTimer1 : calibTimer2);
            coverMaxTime = int( lTmp/1000) + 1;
            writeInt( "coverMaxTime", coverMaxTime);
            report("Calibration CoverTime [s]: " + String(coverMaxTime) );
            pub( Topic.CoverPosSet, "100", true);
            pub( Topic.CoverCalib, "false", true);
            break;
    }

    isCalibWaiting = true;

    #ifdef DEBUG_CALIB
        Serial.println("Function coverCalibrateRoutine() finished. Actual coverCalibState is: " + CalibState[ coverCalibState] );
    #endif

}


// Only used in mode LIGHT
void setRelayLight( byte relay, bool state){

    if ( devMode == "COVER"){
        Serial.println( "Call function setRelayLight not allowed in COVER mode!!!");
        return;
    }

    if ( getRelayState ( relay) == state) return; // Do nothing if relay is in desired state

    if ( PinADE7953 != -1) measEnergy = state; // enable/disable power measurement in loop()
    if ( relay == 1){
        digitalWrite(PinRelay1, state);
        Serial.println( "LIGHT: Set Relay 1 to: " + boolToString( state) );
        delay(100);
        pub( Topic.RelaySet1, ( digitalRead( PinRelay1) == HIGH) ? "true" : "false" , true);
        if ( !state && PinADE7953 != -1) pub( Topic.Power1, "0");
    }
    else if ( relay == 2){
        digitalWrite(PinRelay2, state);
        Serial.println( "LIGHT: Set Relay 2 to: " + boolToString( state) );
        delay(100);
        pub( Topic.RelaySet2, ( digitalRead( PinRelay2) == HIGH) ? "true" : "false" , true);
        if ( !state && PinADE7953 != -1) pub( Topic.Power2, "0");
    }
    if ( digitalRead( PinRelay1) == LOW && digitalRead( PinRelay2) == LOW && PinADE7953 != -1) pub( Topic.PowerAcc, "0");
}
void toggleRelay( byte relay){
    bool state;
    if ( relay == 1){
        digitalWrite(PinRelay1, !digitalRead(PinRelay1) );
        Serial.println( "Toggle Relay 1");
        state = digitalRead( PinRelay1) == HIGH;
        if ( PinADE7953 != -1) measEnergy = state; // enable/disable power measurement in loop()
        pub( Topic.RelaySet1, state ? "true" : "false" , true);
        if ( !state && PinADE7953 != -1) pub( Topic.Power1, "0");
    }
    else if ( relay == 2){
        digitalWrite(PinRelay2, !digitalRead(PinRelay2) );
        Serial.println( "Toggle Relay 2");
        state = digitalRead( PinRelay2) == HIGH;
        if ( PinADE7953 != -1) measEnergy = state; // enable/disable power measurement in loop()
        pub( Topic.RelaySet2, state ? "true" : "false" , true);
        pub( Topic.Power2, "0");
        if ( !state && PinADE7953 != -1) pub( Topic.Power2, "0");
    }
    if ( digitalRead( PinRelay1) == LOW && digitalRead( PinRelay2) == LOW && PinADE7953 != -1) pub( Topic.PowerAcc, "0");
}


void loopCheckSw1() {
    switchTime1 = millis();
    switchState1 = digitalRead( PinSwitch1) == HIGH;
    if ( switchTime1 - switchLastTime1 > debounce && switchState1 != switchLastState1 ){
        // Switch change detected ...
        Serial.println( "Switch1 changed to state: " + boolToString( switchState1) );
        pub( Topic.Switch1, boolToString( switchState1) );
        switchLastTime1 = switchTime1;
        switchLastState1 = switchState1;
        stopCalibration( "Calibration routine stopped by Switch 1!");
        if ( devMode == "LIGHT"){
            if ( switchMode1 == "BUTTON" && switchState1){
                toggleRelay( 1);
            }
            else if ( switchMode1 == "SWITCH") {
                toggleRelay( 1);
            }
        }
        else if ( devMode == "COVER"){
            if ( switchMode1 == "BUTTON" && switchState1){
                setRelayCover( 1, !digitalRead(PinRelay1), 100);
            }
            else if ( switchMode1 == "SWITCH") {
                setRelayCover( 1, switchState1, 100);
            }
        }
    }
}


void loopCheckSw2() {
    switchTime2 = millis();
    switchState2 = digitalRead( PinSwitch2) == HIGH;
    if ( switchTime2 - switchLastTime2 > debounce && switchState2 != switchLastState2 ){
        // Switch change detected ...
        Serial.println( "Switch2 changed to state: " + boolToString( switchState2) );
        pub( Topic.Switch2, boolToString( switchState2) );
        switchLastTime2 = switchTime2;
        switchLastState2 = switchState2;
        stopCalibration( "Calibration routine stopped by Switch 2!");
        if ( devMode == "LIGHT"){
            if ( switchMode2 == "BUTTON" && switchState2){
                toggleRelay( 2);
            }
            else if ( switchMode2 == "SWITCH") {
                toggleRelay( 2);
            }
        }
        else if ( devMode == "COVER"){
            if ( switchMode2 == "BUTTON" && switchState2){
                setRelayCover( 2, !digitalRead(PinRelay2), 0);
            }
            else if ( switchMode2 == "SWITCH") {
                setRelayCover( 2, switchState2, 0);
            }
        }
    }
}



void loopCheckSwR() {
    switchTimeR = millis();
    switchStateR = digitalRead( PinSwitchR) == HIGH;
    if ( switchTimeR - switchLastTimeR > debounce && switchStateR != switchLastStateR ){
        // Switch change detected ...
        Serial.println( "SwitchR changed to state: " + boolToString( switchStateR) );
        switchLastTimeR = switchTimeR;
        switchLastStateR = switchStateR;
        if ( !switchStateR){
            flagLongPress = true;
        }
        else {
            flagLongPress = false;
            Serial.print( "Reset pressed for [s]: ");
            Serial.println( switchTimeLongPressR);
            if ( switchTimeLongPressR > 200 && switchTimeLongPressR < 10000){
                stopCalibration( "Calibration routine stopped by Reset button!");
                restartDevice();
            }
            else if ( switchTimeLongPressR > 10000){
                stopCalibration( "Calibration routine stopped by Reset button!");
                clearPreferences();
                delay(5000);
                restartDevice();
            }
        }
    }
}


bool MqttCommandShelly(String& topic, String& pay) {

    stopCalibration( "Calibration routine stopped over MQTT!");

    if (topic == Topic.RelaySet1) {
        if ( devMode == "LIGHT") setRelayLight( 1, stringToBool( pay) );
        else setRelayCover( 1, stringToBool( pay), ( pay=="true" ? 100 : -1) );
    }
    else if (topic == Topic.RelaySet2) {
        if ( devMode == "LIGHT") setRelayLight( 2, stringToBool( pay) );
        else setRelayCover( 2, stringToBool( pay), ( pay=="true" ? 0 : -1) );
    }
    else if (topic == Topic.CoverPosSet) {
        if ( coverCalibState < CALIBRATED){
            report( "ERROR: Cover not calibrated!");
            pub( Topic.CoverPosSet, "50", true);
            return true;
        }
        int setPosition = pay.toInt();
        if ( setPosition < 0 || setPosition > 100){
            report( "ERROR: Non valid Position received over MQTT!");
            pub( Topic.CoverPosSet, String( coverPosition) , true);
        }
        else {
            stopCover();
            if      ( setPosition > coverPosition) setRelayCover( 1, true, setPosition);
            else if ( setPosition < coverPosition) setRelayCover( 2, true, setPosition);
        }
    }
    else if (topic == Topic.CoverStop) {
        if ( pay == "true"){
            stopCover();
            pub( Topic.CoverStop, "false", true);
        }
    }
    else if (topic == Topic.Config) {
        bool convertOk = convertJsonToConfig( pay);
        if ( convertOk){
            writeString( "config", pay);
            report( "New config saved to memory. Reboot triggered!");
            delay( 2000);
            restartDevice();
        }
        else {
            pub( Topic.Config, config, true);
        }
    }
    else if (topic == Topic.CoverCalib) {
        if ( pay == "true"){
            if ( PinADE7953 != -1){
                coverCalibrateRoutine();
            }
            else {
                report( "Calibration not possible with disabled ADE7953!");
            }
        }
        else {
            stopCalibration( "Calibration routine stopped over MQTT!");
        }
    }
    else
        return false; // command unknown
    return true;

}

*/





void SetupShelly() {



    // Initialize Shelly device


/*
    // --------------------- Read config from non-volatile memory ---------------------

    config = readString( "config", config);
    convertJsonToConfig( config);

    #ifdef DEBUG
        Serial.println(">>> Init Shelly converting config done.");
    #endif

    // --------------------- Read values from non-volatile memory if in COVER mode ---------------------

    if ( devMode == "COVER"){
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

    // --------------------- Init MQTT topics ---------------------

    initMqttTopicsShelly();

    #ifdef DEBUG
        Serial.println(">>> Init Shelly MQTT topics done.");
    #endif

    // --------------------- Modify topics for COVER mode ---------------------

    if ( PinRelay2 == -1 || PinSwitch2 == -1) devMode == "LIGHT"; // Force LIGHT Mode if 2nd Input or Output disabled

    if ( devMode == "COVER"){
        Topic.RelaySet1  = Topic.Device + "/CoverUp";
        Topic.RelaySet2  = Topic.Device + "/CoverDown";
    }

    #ifdef DEBUG
        Serial.println(">>> Init Shelly modification of MQTT topics done.");
    #endif

    // --------------------- Set measurement interval for ADE7953 ---------------------

    if ( devMode == "COVER") measIntervall = 500;
    else measIntervall = 5000;

    // --------------------- Init Switches and Relays ---------------------

    pinMode( PinSwitch1, INPUT_PULLDOWN);
    switchState1 = switchLastState1 = digitalRead( PinSwitch1) == HIGH;
    pinMode( PinRelay1, OUTPUT);
    digitalWrite(PinRelay1, LOW);
    
    pinMode( PinSwitchR, INPUT_PULLUP);
    switchStateR = switchLastStateR = digitalRead( PinSwitchR) == HIGH;

    if ( PinSwitch2 != -1 && PinRelay2 != -1 ){
        pinMode( PinSwitch2, INPUT_PULLDOWN);
        switchState2 = switchLastState2 = digitalRead( PinSwitch1) == HIGH;
        pinMode( PinRelay2, OUTPUT);
        digitalWrite(PinRelay2, LOW);
    }
  
    // --------------------- Publish init state ---------------------

    pubsubShelly();

    #ifdef DEBUG
        Serial.println(">>> Init Shelly publish init states done.");
    #endif

    // --------------------- Init ADE7953 ---------------------

    if ( PinADE7953 != -1){
        myADE7953.initialize( PinSCL, PinSDA);

        #ifdef DEBUG
            Serial.println(">>> Init Shelly ADE7953 done.");
        #endif
    }
*/
}



void LoopShelly() {

/*/
    // --------------------- Check switch entries ---------------------

    loopCheckSw1();
    loopCheckSw2();
    loopCheckSwR();

    // --------------------- Read values from ADE7953 ---------------------

    static unsigned long lastSlowLoop = 0;
    if ( measEnergy && millis() - lastSlowLoop > measIntervall){ // Intervall = 100 if COVER else 5000
        
        Energy = myADE7953.getData();

        #ifdef DEBUG
            pub( Topic.dbg+"voltage0", String( Energy.voltage[0] ) );
            pub( Topic.dbg+"current0", String( Energy.current[0] ) );
            pub( Topic.dbg+"voltage1", String( Energy.voltage[1] ) );
            pub( Topic.dbg+"current1", String( Energy.current[1] ) );
        #endif

        pub( Topic.Power1   , String( Energy.power[0] ) );
        pub( Topic.Power2   , String( Energy.power[1] ) );
        pub( Topic.PowerAcc , String( Energy.powerAcc ) );

        lastSlowLoop = millis();

        // --------------------- If calibration routine running callback routine ---------------------

        if ( isCalibRunning() && isCalibWaiting ) coverCalibrateRoutine();
    }

    // --------------------- Stop cover if target time is reached ---------------------

    if ( devMode == "COVER"){
        if ( coverDirection != "stopped" && millis() > coverTargetTime){
            #ifdef DEBUG
                Serial.printf("Loop timer triggered\n");
            #endif
            stopCover();
        }
    }

    // --------------------- Time measurement for long press Reset button ---------------------

    if ( flagLongPress) switchTimeLongPressR = millis() - switchLastTimeR;
*/

}


int median_of_3(int a, int b, int c){

    int the_max = max(max(a, b), c);
    int the_min = min(min(a, b), c);
    // unnecessarily clever code
    int the_median = the_max ^ the_min ^ a ^ b ^ c;
    return (the_median);
}

template <size_t n> void push(int (&arr)[n], int const value) {
  static size_t index = 0;

  arr[index] = value;
  index = (index + 1) % n;
}

template <size_t n> int pop(int (&arr)[n]) {
  static size_t index = 0;

  int result = arr[index];
  index = (index + 1) % n;
  return result;
}

template <size_t n> void roll(int (&arr)[n]) {
  static size_t index = 0;

  for (size_t i = 0; i < n; i++) {
    Serial.print(arr[(index + i) % n]);
    Serial.print(' ');
  }
  Serial.println();

  index = (index + 1) % n;
}

void filterStringToVector(){

    vecFilterBle.clear();
    int str_len = sFilterBle.length() + 1;
    char cFilterBle[str_len];
    sFilterBle.toCharArray( cFilterBle, str_len);

    Serial.printf( "Splitting filter string \"%s\" into tokens: \n", sFilterBle.c_str() );

    char *end_str;
    char *token = strtok_r( cFilterBle, " ,.;", &end_str);
    while (token != NULL) {
        char *end_token;
        printf ("Entry token: %s\n", token);
        char *token2 = strtok_r(token, "=", &end_token);
        int i=0;
        while (token2 != NULL)
        {
            printf("Sub tokens = %s\n", token2);
            if (i==0){
                vecFilterBle.push_back( token2);
                vecFilterAlias.push_back( token2);
            }
            else{
                vecFilterAlias.pop_back();
                vecFilterAlias.push_back( token2);
            } 
            token2 = strtok_r(NULL, "=", &end_token);
            i+=1;
        }
        token = strtok_r(NULL, ",", &end_str);
    }

    #ifdef DEBUG
        for(int i=0; i < vecFilterAlias.size(); i++){
            printf("vecFilterAlias: %s\n", vecFilterAlias[i].c_str() );
        }
        for(int i=0; i < vecFilterBle.size(); i++){
            printf("vecFilterBle: %s\n", vecFilterBle[i].c_str() );
        }
    #endif
}

void initPubSub() {

    for (int i = 0; i < 2; i++) {
        pub( Topic.Online, "true");
        pub( Topic.Ip, WiFi.localIP().toString() );
        pub( Topic.Restart, "false");
        pub( Topic.HardReset, "false");
        pub( Topic.Filter, sFilterBle);
        pub( TopicGlobal.Message, "Ready");
        if( i==0) delay(500);
    }
        mqttClient.subscribe( Topic.Restart.c_str(), 1);
        mqttClient.subscribe( Topic.HardReset.c_str(), 1);
        mqttClient.subscribe( Topic.Filter.c_str(), 1);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    char new_payload[len + 1];
    new_payload[len] = '\0';
    strncpy(new_payload, payload, len);

    String top = String(topic);
    String pay = String(new_payload);

    Serial.println("MQTT received: " + top + " | " + pay);

    if ( mqttIgnoreCounter > 0){
        mqttIgnoreCounter--;
        #ifdef DEBUG_MQTT
            Serial.printf("MQTT command ignored, remaining ignore-counter: %d!\n", mqttIgnoreCounter);
        #endif
        if ( mqttIgnoreCounter == 0) mqttDisabled = false;
        return;
    }

    if (top == Topic.Restart && pay == "true"){
        restartDevice();
    }
    else if (top == Topic.HardReset && pay == "true"){
        pub( Topic.HardReset, "false", true);
        clearPreferences();
        delay(2500);
        restartDevice();
    }
    else if (top == Topic.Filter){
        pub( Topic.Filter, pay, true);
        pay.replace( " ", "");
        sFilterBle = pay;
        filterStringToVector();
        report( "New filter activated");
        writeString( "sFilterBle", sFilterBle);
    }
/*    else if ( MqttCommandShelly( top, pay))
        ;
*/
    else Serial.println("MQTT unknown: " + top + " | " + pay);

}


class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {

    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {

        String sAddress = advertisedDevice->getAddress().toString().c_str();
        int rssi = advertisedDevice->getRSSI();
        std::string strManufacturerData = advertisedDevice->getManufacturerData();

        String manuf = Sprintf("%02x%02x", strManufacturerData[1], strManufacturerData[0]);

        if (manuf == "004c") { // Apple
            if (strManufacturerData.length() == 25 && strManufacturerData[2] == 0x02 && strManufacturerData[3] == 0x15){
                NimBLEBeacon iBeacon = NimBLEBeacon();
                iBeacon.setData( strManufacturerData);
                sAddress = Sprintf("%s-%u-%u", std::string(iBeacon.getProximityUUID()).c_str(), ENDIAN_CHANGE_U16(iBeacon.getMajor()), ENDIAN_CHANGE_U16(iBeacon.getMinor()));
                #ifdef DEBUG_MQTT
                    Serial.printf( "iBeacon found with UUID: <%s> \n", sAddress.c_str() );
                #endif
            }
        }

        #ifdef DEBUG_MQTT
            //Serial.printf("Advertised Device: %s \n", advertisedDevice->toString().c_str());
        #endif
        
        for (int i=0; i < vecFilterBle.size(); i++) {
            if ( sAddress == vecFilterBle[i] ){
                int pos = i;
                push( arrRssi[pos], rssi);
                String topic = Topic.Results + "/" + vecFilterAlias[i] + "/" + deviceName;
                int median = median_of_3( arrRssi[pos][0], arrRssi[pos][1], arrRssi[pos][2] );
                pubFast( topic, String(  median) );
                #ifdef DEBUG_MQTT
                    Serial.printf("Advertising device to topic: %s with RSSI: %d \n", topic.c_str(), median);
                #endif
                break;
            }
        }
    }

};



class CaptiveRequestHandler : public AsyncWebHandler {
	public:
	CaptiveRequestHandler() {}
	virtual ~CaptiveRequestHandler() {}

	bool canHandle(AsyncWebServerRequest *request){
		//request->addInterestingHeader("ANY");
		return true;
	}

	void handleRequest(AsyncWebServerRequest *request) {
		request->send_P(200, "text/html", captive_html); 
	}
	};

void setupApServer(){

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send_P(200, "text/html", captive_html); 
		Serial.println("Client Connected");
	});
    
	server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
		String inputMessage;

		String ssid, pass, server, port, name, model, mode, input1, input2;
        int intPort = 0;

		int receivedParams = 0;

        Serial.println( "Received Parameters including default values:");
		
		if (request->hasParam( "ssid")) {
			ssid = request->getParam( "ssid")->value();
			ssid.replace(" ", "");
			Serial.print( "Received SSID: ");
			Serial.println( ssid);
			writeString( "ssid", ssid);
			if ( ssid.length() > 0 ) receivedParams++;
		}
		if (request->hasParam( "pass")) {
			pass = request->getParam( "pass")->value();
			pass.replace(" ", "");
			Serial.print( "Received Password: ");
			Serial.println( pass);
			writeString( "pass", pass);
			if ( pass.length() > 0 ) receivedParams++;
		}
		if (request->hasParam( "server")) {
			server = request->getParam( "server")->value();
			server.replace(" ", "");
			Serial.print( "Received MQTT Server: ");
			Serial.println( server);
			writeString( "server", server);
			if ( server.length() > 0 ) receivedParams++;
		}
		if (request->hasParam( "port")) {
			port = request->getParam( "port")->value();
			port.replace(" ", "");
			Serial.print( "Received MQTT Port: ");
			Serial.println( port);
			intPort = port.toInt();
			writeInt( "port", intPort);
			if ( intPort > 0 ) receivedParams++;
		}
		if (request->hasParam( "name")) {
			name = request->getParam( "name")->value();
			name.replace(" ", "");
			Serial.print( "Received Device Name: ");
			Serial.println( name);
			writeString( "name", name);
			if ( name.length() > 0 ) receivedParams++;
		}
		if (request->hasParam( "SelectModel")) {
			model = request->getParam( "SelectModel")->value();
			model.replace(" ", "");
			Serial.print( "Received Device Model: ");
			Serial.println( model);
			writeString( "model", model);
		}
        if (request->hasParam( "deviceMode")) {
			mode = request->getParam( "deviceMode")->value();
			mode.replace(" ", "");
			Serial.print( "Received Device Mode: ");
			Serial.println( mode);
			writeString( "mode", mode);
		}
        if (request->hasParam( "input1")) {
			input1 = request->getParam( "input1")->value();
			input1.replace(" ", "");
			Serial.print( "Received Input1 Mode: ");
			Serial.println( input1);
			writeString( "input1", input1);
		}
        if (request->hasParam( "input2")) {
			input2 = request->getParam( "input2")->value();
			input2.replace(" ", "");
			Serial.print( "Received Input2 Mode: ");
			Serial.println( input2);
			writeString( "input2", input2);
		}

		if ( receivedParams == 5){
			request->send(200, "text/plain", "All parameters received. ESP will restart and connect to your Wifi ...");
			delay(3000);
			ESP.restart();
		}
		else {
			Serial.println( "Missing or wrong parameter(s) received. Please check again, you will be redirected automatically...");
			request->redirect("/");
		}

	});
}


void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {

    Serial.println("Disconnected from MQTT. Restarting ...");
    ESP.restart();
}


void WiFiEvent(WiFiEvent_t event) {

    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.print("WiFi connected with IP address: ");
        Serial.println( WiFi.localIP() );
        writeInt( "wifiValidated", 1);
        wifiWasConnected = true;
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
    // restart only if was connected one time, otherwise not necessary cause of loop in initNetwork()
        if ( wifiWasConnected){ 
            Serial.println("WiFi lost connection! Restarting ESP32 for clean init.");
            ESP.restart();
        }
        break;
    }
}


void onMqttConnect( bool sessionPresent) {
    Serial.println("Connected to MQTT.");
	int mqttValidated = readInt( "mqttValidated", 0);
	if ( mqttValidated == 0) writeInt( "mqttValidated", 1);
}

void initCaptivePortal(){

    captivePortalActivated = true;

	Serial.println("Setting up AP Mode");
	WiFi.mode(WIFI_AP);


	WiFi.softAP("esp32-Shelly");
	Serial.print("AP IP address: ");Serial.println(WiFi.softAPIP());
	Serial.println("Setting up Async WebServer");

	setupApServer();

	Serial.println("Starting DNS Server");
	dnsServer.start(53, "*", WiFi.softAPIP());
	server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP
	server.begin();
}

void initNetwork(){

	String wifiSSID   = readString( "ssid", "");
	String wifiPass   = readString( "pass", "");
	String mqttServer = readString( "server", "");
	int mqttPort      = readInt( "port", 0);

	if ( wifiSSID == "" || wifiPass == "" || mqttServer == "" || mqttPort == 0){
		initCaptivePortal();
		return;
	}

	int wifiValidated = readInt( "wifiValidated", 0);
	int mqttValidated = readInt( "mqttValidated", 0);

    // --------------------- Connect to WIFI ---------------------

    WiFi.onEvent(WiFiEvent);
    Serial.println( "Connecting to Wi-Fi " + wifiSSID + " with " + wifiPass + "...");
    WiFi.begin( wifiSSID.c_str(), wifiPass.c_str() );
    int timeout = 0;
    while ( WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.println(".");
        timeout++;
        if  (timeout > 120){
            Serial.println("");
			if ( wifiValidated == 1){
				Serial.println("WIFI not reachable, restarting ESP32");
				ESP.restart();
			}
			else {
				Serial.println("WIFI not reachable, wrong credentials? HardReset ESP32 and restarting into AP mode...");
				clearPreferences();
				delay(2000);
				ESP.restart();
			}
        }
    }
    Serial.println();

    // --------------------- Connect to MQTT ---------------------

    Serial.print( "Connecting to MQTT " + mqttServer + ":" + String(mqttPort) + "...");
	mqttClient.onConnect( onMqttConnect);
    mqttClient.onMessage( onMqttMessage);
    mqttClient.setServer( mqttServer.c_str(), mqttPort);
    mqttClient.setWill( Topic.Online.c_str(), 1, true, "false");
    String locIP = String( WiFi.localIP() );
    locIP.replace( ".", "");
    mqttClient.setClientId(  locIP.c_str() );
    mqttClient.connect();
    timeout = 0;
    while ( !mqttClient.connected() ){
        mqttClient.connect();
        delay(2000);
        Serial.print(".");
        timeout++;
        if  (timeout > 60){
            Serial.println("");
            if ( mqttValidated == 1){
				Serial.println("MQTT not reachable, restarting ESP32");
				ESP.restart();
			}
			else {
				Serial.println("MQTT not reachable, wrong configuration? HardReset ESP32 and restarting EPS32 into AP mode...");
				clearPreferences();
				delay(2000);
				ESP.restart();
			}
        }
    }
    mqttClient.onDisconnect( onMqttDisconnect);
    Serial.println();
    Serial.println("Connected to MQTT.");

}


void initMqttTopicsGlobal(){
    TopicGlobal.Main    = "shellyscanner"; // mqtt main topic
    TopicGlobal.Device  = TopicGlobal.Main + "/devices/" + deviceName;
    TopicGlobal.dbg     = TopicGlobal.Device + "/Debug/Dbg_";
    TopicGlobal.Message = TopicGlobal.Device + "/Message";
}

void initMqttTopics(){
    Topic.Online    = TopicGlobal.Device + "/Online";
    Topic.Ip        = TopicGlobal.Device + "/IP_Address";
    Topic.Restart   = TopicGlobal.Device + "/Restart";
    Topic.HardReset = TopicGlobal.Device + "/HardReset";
    Topic.Filter    = TopicGlobal.Device + "/Filter";
    Topic.Info      = TopicGlobal.Device + "/Info";

    Topic.Results   = TopicGlobal.Main + "/results";
    
}



String getLocalTime(){

    struct tm timeinfo;
    if( !getLocalTime( &timeinfo)){
        Serial.println("Failed to obtain time (1st)!");
        delay( 2000);
        if( !getLocalTime( &timeinfo)){
            Serial.println("Failed to obtain time (2nd)!");
            return "Error-Getting-Time";
        }
    }
    char timeStringBuff[50]; //50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "%B %d %Y %H:%M", &timeinfo);
    String asString(timeStringBuff);
    return asString;
}


void setup() {

    Serial.begin(115200);
    Serial.println();

    // --------------------- Init Main ---------------------

    deviceName  = readString( "name", "");
    deviceModel = readString( "model", "");
    if ( deviceName == "" || deviceModel == ""){
        Serial.println( "device model or device name is empty, starting CaptivaPortal");
        initCaptivePortal();
    }
    else {

        // --------------------- Init MQTT Topics ---------------------

        initMqttTopicsGlobal();
        initMqttTopics();

        #ifdef DEBUG
            Serial.println(">>> Init MQTT Topics done.");
        #endif

        // --------------------- Init Network ---------------------

        initNetwork();

        #ifdef DEBUG
            Serial.println(">>> Init Network done.");
        #endif

        // --------------------- Scanner filter ---------------------

        sFilterBle = readString( "sFilterBle", sFilterBle);
        filterStringToVector();

        #ifdef DEBUG
            Serial.println(">>> Init ScanFilter done.");
        #endif

        // --------------------- Publish init state ---------------------

        initPubSub();
        mqttIgnoreCounter = 0;

        #ifdef DEBUG
            Serial.println(">>> Init 1st publish of states done.");
        #endif

        // --------------------- Scanner process ---------------------

        NimBLEDevice::init("");

        pBLEScan = NimBLEDevice::getScan();

        pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
        pBLEScan->setMaxResults(0); // do not store the scan results, use callback only.

        #ifdef DEBUG
            Serial.println(">>> Init BLE scan process done.");
        #endif

        // --------------------- Setup OTA function for Arduino or PlatformIO ---------------------

        ArduinoOTA
            .onStart([]() {
                report( "OTA update started ...");
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH)
                    type = "sketch";
                else // U_SPIFFS
                    type = "filesystem";

                // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                Serial.println("Start updating " + type);
            })
            .onEnd([]() {
                Serial.println("\nEnd");
            })
            .onProgress([](unsigned int progress, unsigned int total) {
                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
            })
            .onError([](ota_error_t error) {
                Serial.printf("Error[%u]: ", error);
                if      (error == OTA_AUTH_ERROR)    Serial.println("Auth Failed");
                else if (error == OTA_BEGIN_ERROR)   Serial.println("Begin Failed");
                else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
                else if (error == OTA_END_ERROR)     Serial.println("End Failed");
            });

        ArduinoOTA.begin();

        #ifdef DEBUG
            Serial.println(">>> Init Arduino OTA done.");
        #endif

        // --------------------- Start OTA function over WebServer ---------------------

        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->redirect("/update");
        });

        AsyncElegantOTA.begin(&server);    // Start ElegantOTA on "/update"

        server.begin();

        #ifdef DEBUG
            Serial.println(">>> Init OTA webserver done.");
        #endif

        // --------------------- Get actual time and publish info ---------------------

        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        Info.bootTime = getLocalTime();
        Info.model = deviceModel;
        pub( Topic.Info, Info.toString() );
        delay(250);
        // Send two times to get non-empty ioBroker entry
        pub( Topic.Info, Info.toString() );
        
        // --------------------- Setup Shelly ---------------------

        shelly = &shelly2pm;
        shelly->setup();

        #ifdef DEBUG
            Serial.println(">>> Init Shelly done.");
        #endif

    }

}


void loop() {

    // --------------------- CaptivePortal handler ---------------------

    if ( captivePortalActivated){
        dnsServer.processNextRequest();
    }
    else {

        // --------------------- BLE Scanner Autostart ---------------------

        if ( pBLEScan->isScanning() == false) {
            // Start scan with: duration = 0 seconds(forever), no scan end callback, not a continuation of a previous scan.
            pBLEScan->start(0, nullptr, false);
        }

        // --------------------- OTA handler ---------------------

        ArduinoOTA.handle();

        // --------------------- Disable MQTT handler temporary ---------------------

        if ( mqttDisabled && ( millis() - mqttDisableTime > 5000) ){
            mqttDisabled = false;
            mqttIgnoreCounter = 0;
            #ifdef DEBUG_MQTT
                Serial.println("MQTT commandhandler enabled again!");
            #endif
        }

        // --------------------- Loop Shelly ---------------------

        shelly->loop();

    }

}
