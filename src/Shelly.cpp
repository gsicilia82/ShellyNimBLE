#include <Shelly.h>

/*
************************************************************
************************************************************
START: Parent Class Shelly
************************************************************
************************************************************
*/

Shelly::Shelly(){}

// Reads userconfig from NVM, if exists updates Userconfig variables
void Shelly::setup(){
    updateConfigFromNVM();
}

bool Shelly::getRelayState( int relay){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly::getRelayState()");
    #endif
    
    return digitalRead( Pin.Relay[ relay] );
}

bool Shelly::setRelay( int relay, bool state, int openRelay /* =-1 */){
    #ifdef DEBUG
        Serial.printf(">>> Called Shelly::setRelay() with: relay=%d, state=%d, openRelay=%d \n", relay, state, openRelay);
    #endif

    if ( getRelayState( relay) == state) return true; // Do nothing if relay is in desired state

    if ( !state){
        // open relay without any risk
        digitalWrite( Pin.Relay[ relay], false);
        pub( Topic.Relay[ relay], digitalRead( Pin.Relay[ relay] ), true);
        Serial.printf( "Set Relay %d to: 0 \n", relay);
        // If at least one relay still closed, continue power measurement:
        bool tmp = false;
        for(int i=0; i < Pin.Relay.size(); i++){
            if ( digitalRead( Pin.Relay[ i] ) ) tmp = true;
        }
        measEnergy = tmp; // enable/disable power measurement
    }
    else {
        measEnergy = true; // enable power measurement
        if ( openRelay == -1){
            // close in any case without risks
            digitalWrite(Pin.Relay[ relay], true);
            pub( Topic.Relay[ relay], digitalRead( Pin.Relay[ relay] ), true);
            Serial.printf( "Set Relay %d to: 1 \n", relay);
        }
        else {
            // close relay and secure open counterpart relay (for cover mode)
            int waitTime = ( getRelayState( openRelay) ) ? 1000 : 0;
            digitalWrite( Pin.Relay[ openRelay], false);
            pub( Topic.Relay[ openRelay], digitalRead( Pin.Relay[ openRelay] ), true);
            Serial.printf( "Shortcut-Prevention: Set Relay %d to: 0 \n", openRelay);
            delay( waitTime); // wait only if other relay was opened now
            digitalWrite( Pin.Relay[ relay], !digitalRead( Pin.Relay[ openRelay] ));
            pub( Topic.Relay[ relay], digitalRead( Pin.Relay[ relay] ), true);
            Serial.printf( "Set Relay %d to: %d \n", relay, !digitalRead( Pin.Relay[ openRelay] ) );
        }
    }

    return digitalRead( Pin.Relay[ relay] );
}

bool Shelly::toggleRelay( int relay, int openRelay /* =-1 */){
    return setRelay( relay, !getRelayState( relay), openRelay);
}

void Shelly::initMqttTopics(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly::initMqttTopics()");
    #endif

    Topic.Config = TopicGlobal.Device + "/Config";
    for(int i=0; i < Pin.Input.size(); i++){
        Topic.Input.push_back( TopicGlobal.Device + "/Switch" + String(i+1) );
    }
    for(int i=0; i < Pin.Relay.size(); i++){
        Topic.Relay.push_back( TopicGlobal.Device + "/SetRelay" + String(i+1) );
    }

}

void Shelly::initPubSub(){

    for (int i = 0; i < 2; i++) {
        for(int j=0; j < Topic.Input.size(); j++){
            pub( Topic.Input[j], digitalRead( Pin.Input[j] ) );
        }
        for(int j=0; j < Topic.Relay.size(); j++){
            pub( Topic.Relay[j], digitalRead( Pin.Relay[j] ), true);
        }
        if( i==0) delay(500);
    }
    for(int i=0; i < Topic.Relay.size(); i++){
        mqttClient.subscribe(Topic.Relay[i].c_str(), 1);
    }
}

bool Shelly::updateConfigFromNVM(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly::updateConfigFromNVM()");
    #endif

    String JsonUserConfig = readString( "JsonConfig", "");
    if ( JsonUserConfig == "") return true;

    return setConfigFromJson( JsonUserConfig);
}

void Shelly::saveConfigToNVM(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly::saveConfigToNVM()");
    #endif

    writeString( "JsonConfig", getJsonFromConfig() );
}

void Shelly::loopCheckInputs( int i){
    #ifdef DEBUG_LOOP
        Serial.print(">>> Shelly::loopCheckInputs() with Pin: ");
        Serial.println(i);
    #endif

    Switch[i].switchTime = millis();
    Switch[i].switchState = digitalRead( Pin.Input[ i] ) == HIGH;
    if ( Switch[i].switchTime - Switch[i].switchLastTime > debounce && Switch[i].switchState != Switch[i].switchLastState){
        // Input change detected ...
        Serial.printf( "Input %d changed to state: %d \n", i, Switch[i].switchState);
        pub( Topic.Input[ i],  Switch[i].switchState);
        Switch[i].switchLastTime = Switch[i].switchTime;
        Switch[i].switchLastState = Switch[i].switchState;

        workInput( i, Switch[i].switchState);
    }
}

void Shelly::loopCheckSwR( int i){
    #ifdef DEBUG_LOOP
        Serial.print(">>> Shelly::loopCheckSwR() with Pin: ");
        Serial.println(i);
    #endif

    Switch[i].switchTime = millis();
    Switch[i].switchState = digitalRead( Pin.ButtonReset) == HIGH;
    if ( Switch[i].switchTime - Switch[i].switchLastTime > debounce && Switch[i].switchState != Switch[i].switchLastState){
        // Reset input change detected ...
        Serial.printf( "Reset button changed to state: %d \n", Switch[i].switchState);
        Switch[i].switchLastTime = Switch[i].switchTime;
        Switch[i].switchLastState = Switch[i].switchState;
        if ( !Switch[i].switchState){
            flagLongPress = true;
        }
        else {
            flagLongPress = false;
            Serial.print( "Reset pressed for [s]: ");
            Serial.println( switchTimeLongPressR);
            if ( switchTimeLongPressR > 200 && switchTimeLongPressR < 10000){
                workReset();
                restartDevice();
            }
            else if ( switchTimeLongPressR > 10000){
                workReset();
                clearPreferences();
                delay(5000);
                restartDevice();
            }
        }
    }
}

void Shelly::loop(){
    #ifdef DEBUG_LOOP
        Serial.println(">>> Called Shelly::loop()");
    #endif

    for(int i=0; i < Pin.Input.size(); i++){
        loopCheckInputs( i);
    }

    loopCheckSwR( Switch.size()-1 );

    // --------------------- Time measurement for long press Reset button ---------------------

    if ( flagLongPress) switchTimeLongPressR = millis() - Switch[ Switch.size()-1].switchLastTime;
}


/*
************************************************************
************************************************************
END:   Parent Class Shelly
************************************************************
************************************************************
START: Class Shelly Plus 2PM
************************************************************
************************************************************
*/

void Shelly2PM::setup(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly2PM::setup()");
    #endif

    #ifdef TEST_COVERSECURITY
        writeInt( "coverCalibState", CALIBRATED);
        writeInt( "coverPosition", 100);
        writeInt( "coverMaxTime", 20);
    #endif

    extendBaseConfig();

    Shelly::setup();

    // Set device mode
    deviceMode.Setting = readString( "mode", "Cover"); // Default setting is "Cover" for security reasons

    // Read Cover specific values from NVM
    if ( deviceMode.Setting == deviceMode.Cover){
        coverCalibState = readInt( "coverCalibState", coverCalibState);
        coverMaxTime = readInt( "coverMaxTime", coverMaxTime);
        measIntervall = 500; // Power measure intervall
        Serial.print(">>> COVER calibration state is: ");
        Serial.println( CalibState[ coverCalibState] );
        
        if ( coverCalibState < CALIBRATED){
            coverPosition = 50;
            writeInt( "coverPosition", coverPosition);
        }
        else {
            coverPosition = readInt( "coverPosition", coverPosition);
        }
    }
    else measIntervall = 5000;

    // Auto-Detect Shelly2PM pcb version by pushing reset button during boot (v0.1.5 || v0.1.9)
    Serial.println("Auto-Detecting Shelly PCB Version by pressing reset button...");
    String pcbVersion = "";
    pinMode( 4, INPUT_PULLUP); // v0.1.9
    if ( digitalRead( 4) == LOW){
        pcbVersion = "v0.1.9";
        Serial.println("Auto-Detected PCB Version: " + pcbVersion);
    }
    else {
        pinMode( 27, INPUT_PULLUP); // v0.1.5
        if ( digitalRead( 27) == LOW){
            pcbVersion = "v0.1.5";
            Serial.println("Auto-Detected PCB Version: " + pcbVersion);
        }
    }

    if ( pcbVersion == "") Serial.println("Auto-Detecting Shelly PCB Version failed. Using default/saved configuration");
    else UserConfig.is_V019 = pcbVersion == "v0.1.9" ? true : false;

    // Modify variables from base class: modify Pin, extend Switch, extend SwitchMode
    overwriteBaseConfig();

    saveConfigToNVM();


    // Init Switches and Relays
    for(int i=0; i < Pin.Input.size(); i++){
        pinMode( Pin.Input[i], INPUT_PULLDOWN);
        Switch[i].switchLastState = Switch[i].switchState = digitalRead( Pin.Input[i] ) == HIGH;
    }
    for(int i=0; i < Pin.Relay.size(); i++){
        pinMode( Pin.Relay[i], OUTPUT);
        digitalWrite( Pin.Relay[i], LOW);
    }

    // Init Reset button variables.
    pinMode( Pin.ButtonReset, INPUT_PULLUP);
    Switch[ Switch.size()-1 ].switchLastState = Switch[ Switch.size()-1 ].switchState = digitalRead( Pin.ButtonReset ) == HIGH;
    
    initMqttTopics();

    myADE7953.initialize( Pin.I2C_SCL, Pin.I2C_SDA);
}

void Shelly2PM::extendBaseConfig(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly2PM::overwriteBaseConfig()");
    #endif

    Pin.Input.push_back( -1);
    Pin.Input.push_back( -1);
    Pin.Relay.push_back( -1);
    Pin.Relay.push_back( -1);

    // !!! Last entry -> Reset button !!!
    Switch.push_back( { 0, 0, false, false} );
    Switch.push_back( { 0, 0, false, false} );
    Switch.push_back( { 0, 0, false, false} );

    SwitchMode.push_back( "");
    SwitchMode.push_back( "");    
}

void Shelly2PM::overwriteBaseConfig(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly2PM::overwriteBaseConfig()");
    #endif

    if ( UserConfig.is_V019){
        // Pinout PCB v0.1.9
        Pin.ButtonReset = 4;
        Pin.I2C_SCL = 25;
        Pin.I2C_SDA = 26;
        Pin.ADE7953 = 27;
        Pin.Temperature = 35;
        Pin.Input[0] = !UserConfig.SwapInput  ?  5 : 18;
        Pin.Input[1] = !UserConfig.SwapInput  ? 18 :  5;
        Pin.Relay[0] = !UserConfig.SwapOutput ? 13 : 12;
        Pin.Relay[1] = !UserConfig.SwapOutput ? 12 : 13;
    }
    else {
        // Pinout PCB v0.1.5
        Pin.ButtonReset = 27;
        Pin.I2C_SCL = 25;
        Pin.I2C_SDA = 33;
        Pin.ADE7953 = 36;
        Pin.Temperature = 37;
        Pin.Input[0] = !UserConfig.SwapInput  ?  2 : 18;
        Pin.Input[1] = !UserConfig.SwapInput  ? 18 :  2;
        Pin.Relay[0] = !UserConfig.SwapOutput ? 13 : 12;
        Pin.Relay[1] = !UserConfig.SwapOutput ? 12 : 13;
    }

    if ( SwitchMode[0] != "") return; // return if already initialized from NVM

    // Initialize switch behaviour (Switch || Button || Detached) from nvs and store in base class
    String tmp = readString( "input1", "");
    if ( tmp == ""){
        report( "Input SwitchMode 1 setting is empty! Remains to default value 'SWITCH'");
        SwitchMode[0] = "SWITCH";
    } else {
        SwitchMode[0] = tmp;
    }

    tmp = readString( "input2", "");
    if ( tmp == ""){
        report( "Input SwitchMode 2 setting is empty! Remains to default value 'SWITCH'");
        SwitchMode[1] = "SWITCH";
    } else {
        SwitchMode[1] = tmp;
    }
}

void Shelly2PM::initMqttTopics(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly2PM::initMqttTopics()");
    #endif

    Shelly::initMqttTopics();

    Topic.Power.push_back( TopicGlobal.Device + "/Power1");
    Topic.Power.push_back( TopicGlobal.Device + "/Power2");
    Topic.PowerAcc = TopicGlobal.Device + "/PowerAcc";

    if ( deviceMode.Setting == deviceMode.Cover){
        Topic.CoverPosSet = TopicGlobal.Device + "/CoverSetPosition";
        Topic.CoverStop   = TopicGlobal.Device + "/CoverStop";
        Topic.CoverCalib  = TopicGlobal.Device + "/CoverStartCalibration";
        Topic.CoverState  = TopicGlobal.Device + "/CoverState";
        Topic.Relay[0] = TopicGlobal.Device + "/CoverUp";   // overwrite base class init
        Topic.Relay[1] = TopicGlobal.Device + "/CoverDown"; // overwrite base class init
    }

}

void Shelly2PM::initPubSub(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly2PM::initPubSub()");
    #endif

    Shelly::initPubSub();

    for ( int i = 0; i < 2; i++) {
        pub( Topic.Config, getJsonFromConfig(), true);
        pub( Topic.Power[0] , "0");
        pub( Topic.Power[1] , "0");
        pub( Topic.PowerAcc , "0");

        if ( deviceMode.Setting == deviceMode.Cover){
            pub( Topic.CoverPosSet, String( coverPosition), true);
            pub( Topic.CoverStop, "false", true);
            pub( Topic.CoverCalib, "false", true);
            pub( Topic.CoverState, "stopped");
        }
        if( i==0) delay(500);
    }

    mqttClient.subscribe(Topic.Config.c_str(), 1);
    if ( deviceMode.Setting == deviceMode.Cover){
        mqttClient.subscribe(Topic.CoverPosSet.c_str(), 1);
        mqttClient.subscribe(Topic.CoverStop.c_str(), 1);
        mqttClient.subscribe(Topic.CoverCalib.c_str(), 1);
    }
}

String Shelly2PM::getJsonFromConfig(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly2PM::getJsonFromConfig()");
    #endif

    StaticJsonDocument<300> json;
    String JsonUserConfig;
    json[ "Mode_Input1"] = SwitchMode[0];
    json[ "Mode_Input2"] = SwitchMode[1];
    json[ "SwapInput"]   = UserConfig.SwapInput;
    json[ "SwapOutput"]  = UserConfig.SwapOutput;
    json[ "is_V019"]     = UserConfig.is_V019;
    serializeJson( json, JsonUserConfig);

    return JsonUserConfig;
}

bool Shelly2PM::setConfigFromJson( String JsonConfig, bool withPub /* =true */){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly2PM::setConfigFromJson()");
    #endif

    StaticJsonDocument<300> json;
    DeserializationError error = deserializeJson( json, JsonConfig.c_str() );

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return false;
    }

    const char* tmpChSwitchMode;
    String tmpStrSwitchMode;

    tmpChSwitchMode = json["Mode_Input1"];
    tmpStrSwitchMode = String( tmpChSwitchMode);
    tmpStrSwitchMode.toUpperCase();
    if ( tmpStrSwitchMode != "BUTTON" && tmpStrSwitchMode != "SWITCH" && tmpStrSwitchMode != "DETACHED") {
        report( "ERROR: Non valid Mode received for Switch1: <" + tmpStrSwitchMode + ">", withPub);
        return false;
    }
    else SwitchMode[0]  = tmpStrSwitchMode;

    tmpChSwitchMode = json["Mode_Input2"];
    tmpStrSwitchMode = String( tmpChSwitchMode);
    tmpStrSwitchMode.toUpperCase();
    if ( tmpStrSwitchMode != "BUTTON" && tmpStrSwitchMode != "SWITCH" && tmpStrSwitchMode != "DETACHED") {
        report( "ERROR: Non valid Mode received for Switch1: <" + tmpStrSwitchMode + ">", withPub);
        return false;
    }
    else SwitchMode[1]  = tmpStrSwitchMode;

    UserConfig.SwapInput = json["SwapInput"];
    UserConfig.SwapOutput = json["SwapOutput"];
    UserConfig.is_V019 = json["is_V019"];

    return true;
}

// "opening" || "closing": Calculates coverTargetTime related to coverTargetPosition
// "stopped": Calculates coverPosition
void Shelly2PM::calcCoverPosition( String cmd, int coverTargetPosition /* =100 */){ //opening, closing, stopped
    #ifdef DEBUG
        Serial.printf(">>> Called Shelly2PM::calcCoverPosition() with: cmd=%s, coverTargetPosition=%d \n", cmd, coverTargetPosition);
    #endif

    pub( Topic.CoverState, cmd);

    if ( cmd == coverDirs.stopped ){
        // calc actual position and report/save
        unsigned long deltaTime = millis() - coverStartTime;

        if ( coverCalibState == CALIBRATED && isCoverDir == coverDirs.opening){
            coverPosition = coverPosition + (int)( deltaTime*100 / ( coverMaxTime*1000) +0.5);
            coverPosition = coverPosition > 100 ? 100 : coverPosition;
        }
        else if ( coverCalibState == CALIBRATED && isCoverDir == coverDirs.closing){
            coverPosition = coverPosition - (int)( deltaTime*100 / ( coverMaxTime*1000) +0.5);
            coverPosition = coverPosition < 0 ? 0 : coverPosition;
        }
        else{
            coverPosition = 50;
        }
        isCoverDir = cmd;
        writeInt( "coverPosition", coverPosition);
        pub( Topic.CoverPosSet, String( coverPosition), true);
    }
    else if ( cmd == coverDirs.opening){
        // activate loop statement and set time limit in loop observation
        coverStartTime = millis();
        if ( coverCalibState == CALIBRATED){
            coverTargetTime = coverStartTime + ( coverMaxTime*1000) * ( coverTargetPosition - coverPosition) / 100 ;
            if ( coverTargetPosition == 100) coverTargetTime += 2000;
        }
        else {
            coverTargetTime = coverStartTime + ( 100*1000); // default 100s if not calibrated
        }
        isCoverDir = cmd; // enable time measurement in loop()
    }
    else if ( cmd == coverDirs.closing){
        // activate loop statement and set time limit in loop observation
        coverStartTime = millis();
        if ( coverCalibState == CALIBRATED){
            coverTargetTime = coverStartTime + ( coverMaxTime*1000) * ( coverPosition - coverTargetPosition) / 100 ;
            if ( coverTargetPosition == 0) coverTargetTime += 2000;
        }
        else {
            coverTargetTime = coverStartTime + ( 100*1000); // default 100s if not calibrated
        }
        isCoverDir = cmd; // enable time measurement in loop()
    }

}

bool Shelly2PM::stopCover(){ // returns false if already stopped, otherwise true
    #ifdef DEBUG
        Serial.printf(">>> Called Shelly2PM::stopCover(). Actual direction is: %s \n", isCoverDir);
    #endif

    if ( isCoverDir == coverDirs.stopped) return false;

    if ( isCoverDir == coverDirs.opening){
        setRelay( 0, false);
    }
    else if ( isCoverDir == coverDirs.closing){
        setRelay( 1, false);
    }
    calcCoverPosition( coverDirs.stopped);

    pub( Topic.Power[0], "0");
    pub( Topic.Power[1], "0");
    pub( Topic.PowerAcc, "0");

    return true;
}

bool Shelly2PM::isCalibRunning() {
    #ifdef DEBUG
        Serial.println(">>> Called Shelly2PM::isCalibRunning()");
    #endif

    return ( coverCalibState > NOT_CALIBRATED && coverCalibState < CALIBRATED);
}

void Shelly2PM::stopCalibration( String msg /* ="" */){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly2PM::stopCalibration()");
    #endif

    if ( isCalibRunning() ){
        coverCalibState = NOT_CALIBRATED;
        report( "Cover Calibration stopped. " + msg);
        pub( Topic.CoverCalib, "false", true);
        stopCover();
    }
}


void Shelly2PM::coverCalibrateRoutine(){
    #ifdef DEBUG_CALIB
        Serial.println("Called Shelly2PM::coverCalibrateRoutine(). Actual coverCalibState is: " + CalibState[ coverCalibState] );
    #endif

    float fTmp;
    unsigned long lTmp;
    unsigned long calibTimeout = 100000; // Timeout to reach end positions in ms
    isCalibWaiting = false;

    switch( coverCalibState) {
        case NOT_CALIBRATED:
            report("Cover Calibration started! Raise cover up to end position...");
            setRelay( 0, true, 1);
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
            setRelay( 1, true, 0);
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
                fTmp = calibTimer1 / 1000;
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
            setRelay( 0, true, 1);
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

bool Shelly2PM::onMqttMessage(String& topic, String& pay){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly2PM::onMqttMessage()");
    #endif

    stopCalibration( "Calibration routine stopped over MQTT!");

    if (topic == Topic.Relay[0]) {
        bool bPay = stringToBool( pay);
        if ( deviceMode.Setting == deviceMode.Light) setRelay( 0, bPay);
        else{
            int waitTime = stopCover() ? 1000 : 0;
            if ( bPay){
                delay( waitTime);
                calcCoverPosition( coverDirs.opening, 100);
                setRelay( 0, true, 1);
            }
        }
    }
    else if (topic == Topic.Relay[1]) {
        bool bPay = stringToBool( pay);
        if ( deviceMode.Setting == deviceMode.Light) setRelay( 1, bPay);
        else{
            int waitTime = stopCover() ? 1000 : 0;
            if ( bPay){
                delay( waitTime);
                calcCoverPosition( coverDirs.closing, 0);
                setRelay( 1, true, 0);
            }
        }
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
            int waitTime = stopCover() ? 1000 : 0;
            delay( waitTime);
            if ( setPosition > coverPosition){
                calcCoverPosition( coverDirs.opening, setPosition);
                setRelay( 0, true, 1);
            }
            else if ( setPosition < coverPosition){
                calcCoverPosition( coverDirs.closing, setPosition);
                setRelay( 1, true, 0);
            }
        }
    }
    else if (topic == Topic.CoverStop) {
        if ( pay == "true"){
            stopCover();
            pub( Topic.CoverStop, "false", true);
        }
    }
    else if (topic == Topic.CoverCalib) {
        if ( pay == "true") coverCalibrateRoutine();
    }
    else if (topic == Topic.Config) {
        bool convertOk = setConfigFromJson( pay);
        if ( convertOk){
            saveConfigToNVM();
            report( "New config saved to memory. Reboot triggered!");
            delay( 2000);
            restartDevice();
        }
        else {
            pub( Topic.Config, getJsonFromConfig(), true);
        }
    }
    else return false; // command unknown

    return true;
}

void Shelly2PM::workInput( int input, bool state){
    stopCalibration( "Calibration routine stopped by input!");

    if ( deviceMode.Setting == deviceMode.Light){

        if ( SwitchMode[ input] == "SWITCH"){
            toggleRelay( input);
        }
        else if ( SwitchMode[ input] == "BUTTON" && state){
            toggleRelay( input);
        }
    }
    else if ( deviceMode.Setting == deviceMode.Cover){

        if ( SwitchMode[ input] == "SWITCH"){
            int waitTime = stopCover() ? 1000 : 0;
            if ( state){
                delay( waitTime);
                if ( input == 0) calcCoverPosition( coverDirs.opening, 100);
                else calcCoverPosition( coverDirs.closing, 0);
                setRelay( input, state, 1-input);
            }
        }
        else if ( SwitchMode[ input] == "BUTTON" && state){
            if ( isCoverDir != coverDirs.stopped) stopCover();
            else {
                if ( input == 0) calcCoverPosition( coverDirs.opening, 100);
                else calcCoverPosition( coverDirs.closing, 0);
                setRelay( input, state, 1-input);
            }
        }
    }
    else report( "Device mode unknown: " + deviceMode.Setting);
}

void Shelly2PM::workReset(){
    stopCalibration( "Calibration routine stopped by reset input!");
}
    
void Shelly2PM::loop(){
    #ifdef DEBUG_LOOP
        Serial.println(">>> Shelly2PM::loop()");
    #endif

    Shelly::loop();

    // --------------------- Read values from ADE7953 ---------------------

    static unsigned long lastSlowLoopShelly = 0;
    if ( measEnergy && millis() - lastSlowLoopShelly > measIntervall){ // Intervall = 500 if COVER else 5000
        lastSlowLoopShelly = millis();
        
        Energy = myADE7953.getData();

        #ifdef DEBUG
            pub( TopicGlobal.dbg+"voltage0", String( Energy.voltage[0] ) );
            pub( TopicGlobal.dbg+"current0", String( Energy.current[0] ) );
            pub( TopicGlobal.dbg+"voltage1", String( Energy.voltage[1] ) );
            pub( TopicGlobal.dbg+"current1", String( Energy.current[1] ) );
        #endif

        pub( Topic.Power[0], String( Energy.power[0] ) );
        pub( Topic.Power[1], String( Energy.power[1] ) );
        pub( Topic.PowerAcc, String( Energy.powerAcc ) );

        // --------------------- If calibration routine running callback routine ---------------------

        if ( isCalibRunning() && isCalibWaiting ) coverCalibrateRoutine();
    }

    // --------------------- Stop cover if target time is reached ---------------------

    if ( deviceMode.Setting == deviceMode.Cover){
        if ( isCoverDir == coverDirs.stopped && measEnergy){
            // If cover movement was triggered by hardware Switch
            if      ( getRelayState( 0) ) calcCoverPosition( coverDirs.opening, 100);
            else if ( getRelayState( 1) ) calcCoverPosition( coverDirs.closing, 0);
            else {
                report( "ERROR: Cover is stopped and no relay is closed, but Energy measurement is active!");
            }
        }
#ifdef TEST_COVERSECURITY
        else if ( isCoverDir != coverDirs.stopped && millis() > coverTargetTime){
#else
        else if ( isCoverDir != coverDirs.stopped && ( millis() > coverTargetTime || (millis() - coverStartTime > 2000 && Energy.powerAcc < limPowLow) ) ){
#endif
        
            #ifdef DEBUG
                Serial.printf("Cover timer or power measurement triggered. Power: %f \n", Energy.powerAcc);
            #endif
            stopCover();
        }
    }

}

/*
************************************************************
************************************************************
END:   Class Shelly Plus 2PM
************************************************************
************************************************************
START: Class Shelly Plus 1(PM)
************************************************************
************************************************************
*/


void Shelly1PM::setup(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly1PM::setup()");
    #endif

    extendBaseConfig();

    Shelly::setup();

    // Modify variables from base class: Pin, extend Switch, extend SwitchMode
    overwriteBaseConfig();


    // Init Switches and Relays
    for(int i=0; i < Pin.Input.size(); i++){
        pinMode( Pin.Input[i], INPUT_PULLDOWN);
        Switch[i].switchLastState = Switch[i].switchState = digitalRead( Pin.Input[i] ) == HIGH;
    }
    for(int i=0; i < Pin.Relay.size(); i++){
        pinMode( Pin.Relay[i], OUTPUT);
        digitalWrite( Pin.Relay[i], LOW);
    }

    // Init Reset button variables
    pinMode( Pin.ButtonReset, INPUT_PULLUP);
    Switch[ Switch.size()-1 ].switchLastState = Switch[ Switch.size()-1 ].switchState = digitalRead( Pin.ButtonReset ) == HIGH;

    initMqttTopics();
}

void Shelly1PM::extendBaseConfig(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly1PM::overwriteBaseConfig()");
    #endif

    Pin.Input.push_back( -1);
    Pin.Relay.push_back( -1);

    // !!! Last entry -> Reset button !!!
    Switch.push_back( { 0, 0, false, false} );
    Switch.push_back( { 0, 0, false, false} );

    SwitchMode.push_back( "");  
}

void Shelly1PM::overwriteBaseConfig(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly1PM::overwriteBaseConfig()");
    #endif

    Pin.ButtonReset = 25;
    Pin.Temperature = 32;
    Pin.Input[0] = 4;
    Pin.Relay[0] = 26;

    if ( SwitchMode[0] != "") return; // return if already initialized from NVM
    
    // Initialize switch behaviour (Switch || Button || Detached) from nvs and store in base class
    String tmp = readString( "input1", "");
    if ( tmp == ""){
        report( "Input SwitchMode 1 setting is empty! Remains to default value 'SWITCH'");
        SwitchMode[0] = "SWITCH";
    } else {
        SwitchMode[0] = tmp;
    }    
}

void Shelly1PM::initMqttTopics(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly1PM::initMqttTopics()");
    #endif

    Shelly::initMqttTopics();
}

void Shelly1PM::initPubSub(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly1PM::initPubSub()");
    #endif

    Shelly::initPubSub();

    for ( int i = 0; i < 2; i++) {
        pub( Topic.Config, getJsonFromConfig(), true);
        if( i==0) delay(500);
    }

    mqttClient.subscribe(Topic.Config.c_str(), 1);
}

String Shelly1PM::getJsonFromConfig(){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly1PM::getJsonFromConfig()");
    #endif

    StaticJsonDocument<300> json;
    String JsonUserConfig;
    json[ "Mode_Input1"] = SwitchMode[0];
    serializeJson( json, JsonUserConfig);

    return JsonUserConfig;
}

bool Shelly1PM::setConfigFromJson( String JsonConfig, bool withPub /* =true */){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly1PM::setConfigFromJson()");
    #endif

    StaticJsonDocument<300> json;
    DeserializationError error = deserializeJson( json, JsonConfig.c_str() );

    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return false;
    }

    const char* tmpChSwitchMode;
    String tmpStrSwitchMode;

    tmpChSwitchMode = json["Mode_Input1"];
    tmpStrSwitchMode = String( tmpChSwitchMode);
    tmpStrSwitchMode.toUpperCase();
    if ( tmpStrSwitchMode != "BUTTON" && tmpStrSwitchMode != "SWITCH" && tmpStrSwitchMode != "DETACHED") {
        report( "ERROR: Non valid Mode received for Switch1: <" + tmpStrSwitchMode + ">", withPub);
        return false;
    }
    else SwitchMode[0]  = tmpStrSwitchMode;

    return true;
}

bool Shelly1PM::onMqttMessage(String& topic, String& pay){
    #ifdef DEBUG
        Serial.println(">>> Called Shelly1PM::onMqttMessage()");
    #endif

    if (topic == Topic.Relay[0]) {
        setRelay( 1, stringToBool( pay) );
        report( "Successfully set relay to state: " + pay);
    }
    else if (topic == Topic.Config) {
        bool convertOk = setConfigFromJson( pay);
        if ( convertOk){
            saveConfigToNVM();
            report( "New config saved to memory. Reboot triggered!");
            delay( 2000);
            restartDevice();
        }
        else {
            pub( Topic.Config, getJsonFromConfig(), true);
        }
    }
    else return false; // command unknown
    
    return true;
}

void Shelly1PM::workInput( int input, bool state){
    if ( SwitchMode[ input] == "SWITCH"){
        toggleRelay( input);
    }
    else if ( SwitchMode[ input] == "BUTTON" && state){
        toggleRelay( input);
    }
}

void Shelly1PM::workReset(){}
    
void Shelly1PM::loop(){ Shelly::loop();}

/*
************************************************************
************************************************************
END:   Class Shelly Plus 1(PM)
************************************************************
************************************************************
START: Class Shelly Plus i4
************************************************************
************************************************************
*/

void Shellyi4::setup(){
    #ifdef DEBUG
        Serial.println(">>> Called Shellyi4::setup()");
    #endif

    extendBaseConfig();

    Shelly::setup();

    // Modify variables from base class: Pin, extend Switch, extend SwitchMode
    overwriteBaseConfig();

    // Init Switches
    for(int i=0; i < Pin.Input.size(); i++){
        pinMode( Pin.Input[i], INPUT_PULLDOWN);
        Switch[i].switchLastState = Switch[i].switchState = digitalRead( Pin.Input[i] ) == HIGH;
    }

    // Init Reset button variables
    pinMode( Pin.ButtonReset, INPUT_PULLUP);
    Switch[ Switch.size()-1 ].switchLastState = Switch[ Switch.size()-1 ].switchState = digitalRead( Pin.ButtonReset ) == HIGH;

    initMqttTopics();
}

void Shellyi4::extendBaseConfig(){
    #ifdef DEBUG
        Serial.println(">>> Called Shellyi4::overwriteBaseConfig()");
    #endif

    Pin.Input.push_back( -1);
    Pin.Input.push_back( -1);
    Pin.Input.push_back( -1);
    Pin.Input.push_back( -1);

    // !!! Last entry -> Reset button !!!
    Switch.push_back( { 0, 0, false, false} );
    Switch.push_back( { 0, 0, false, false} );
    Switch.push_back( { 0, 0, false, false} );
    Switch.push_back( { 0, 0, false, false} );
    Switch.push_back( { 0, 0, false, false} );
}

void Shellyi4::overwriteBaseConfig(){
    #ifdef DEBUG
        Serial.println(">>> Called Shellyi4::overwriteBaseConfig()");
    #endif

    Pin.ButtonReset = 25;
    Pin.Input[0] = 12;
    Pin.Input[1] = 14;
    Pin.Input[2] = 27;
    Pin.Input[3] = 26;   
}

void Shellyi4::initMqttTopics(){ Shelly::initMqttTopics();}

void Shellyi4::initPubSub(){ Shelly::initPubSub();}

String Shellyi4::getJsonFromConfig(){ return "";}

bool Shellyi4::setConfigFromJson( String JsonConfig, bool withPub /* =true */){ return true;}

bool Shellyi4::onMqttMessage(String& topic, String& pay){ return false;}

void Shellyi4::workInput( int input, bool state){}

void Shellyi4::workReset(){}
    
void Shellyi4::loop(){ Shelly::loop();}
