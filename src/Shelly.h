#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <vector>
#include <globals.h>

// Powermeter
#include <ADE7953.h>


// Abstract parent class Shelly
class Shelly {

    protected:
        struct TOPICS {
            std::vector<String> Input;
            std::vector<String> Relay;
            std::vector<String> Power;

            String Config,
            CoverState,
            CoverPosSet,
            CoverStop,
            CoverCalib,
            PowerAcc;
        } Topic;

        // Base config will be overwritten with ShellyChild::overwriteBaseConfig()
        struct PINCONFIG {
            int ButtonReset = -1;
            int I2C_SCL = -1;
            int I2C_SDA = -1;
            int ADE7953 = -1;
            int Temperature = -1;
            std::vector<int> Input;
            std::vector<int> Relay;
        } Pin;

        struct SWITCH {
            unsigned long switchTime, switchLastTime;
            bool switchState, switchLastState;
        };
        // Vector with SWITCH struct entities; last entry will be reset button
        std::vector<SWITCH> Switch;

        // Vector with SwitchMode (Switch, Button, Detached)
        std::vector<String> SwitchMode;

        // Special variables for Reset button
        unsigned long switchTimeLongPressR = 0;
        bool flagLongPress = false;

        uint debounce = 100; // debounce switch input in ms
        bool measEnergy = false;
        
        bool getRelayState( int relay);
        bool setRelay( int relay, bool state, int openRelay=-1);
        bool toggleRelay( int relay, int openRelay=-1);
        bool updateConfigFromNVM();
        void saveConfigToNVM();
        void loopCheckInputs( int i);
        void loopCheckSwR( int i);

        virtual void initMqttTopics();
        virtual String getJsonFromConfig() = 0;
        virtual bool setConfigFromJson( String JsonConfig, bool withPub=true) = 0;
        virtual void workInput( int input, bool state) = 0;
        virtual void workReset() = 0;


    public:
        Shelly();
        virtual void setup() = 0;
        virtual void initPubSub();
        virtual bool onMqttMessage( String& topic, String& pay) = 0;
        virtual void loop() = 0;
};


class Shelly2PM : public Shelly{

    private:
        ADE7953 myADE7953;
        ENERGY Energy;

        // UserConfig is available as JSON over MQTT
        // SwitchMode will be added from base class to JSON
        struct USERCONFIG {
            bool SwapInput = false;
            bool SwapOutput = false;
            bool is_V019 = true;
        } UserConfig;

        struct DEVMODE {
            String Setting = "";
            // String Variables Cover and Light only for comparison to Setting!
            String Cover = "Cover";
            String Light = "Light";
        } deviceMode;

        bool measEnergy = false;
        int measIntervall = 5000; // 500ms in case of COVER mode, set during setup()

        // Auto detect pcb version with reset pin in setup()
        // String pcbVersion = "v0.1.9"; // "v0.1.5" || "v0.1.9"

        // ##########################
        // Needed only in COVER mode
        // ##########################

        // Definitions:
        // Input[0] -> Relay[0] -> opening
        // Input[1] -> Relay[1] -> closing

        unsigned long coverStartTime = 0;  // Time when COVER was triggered to go UP/DOWN
        unsigned long coverTargetTime = 0; // Max time, when end position should be reached
        
        int coverPosition = 50;            // default value; real value from non-volatile memory

        String isCoverDir = "stopped";

        struct COVERDIRS {
            String stopped = "stopped";
            String opening = "opening";
            String closing = "closing";
        } coverDirs;

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

        // Used in Serial.print from coverCalibrateRoutine() for debug purposes
        std::vector<String> CalibState{
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

        int coverCalibState = NOT_CALIBRATED;
        int coverMaxTime  = 100; // Calibrated cover time, default 100 if not calibrated
        unsigned long calibTimer1  = 0;
        unsigned long calibTimer2  = 0;
        unsigned long calibStepTimer = 0;
        bool isCalibWaiting = false;

        // Watt limits for calibration
        int limPowHigh = 40;
        int limPowLow = 15;


        void calcCoverPosition( String cmd, int coverTargetPosition=100);
        bool stopCover();
        bool isCalibRunning();
        void stopCalibration( String msg="");
        void coverCalibrateRoutine();

        // #####################################
        // End COVER mode variables / functions
        // #####################################

        void extendBaseConfig();
        void overwriteBaseConfig();
        void initMqttTopics();
        String getJsonFromConfig();
        bool setConfigFromJson( String JsonConfig, bool withPub=true);
        bool parseJsonConfig();
        void workInput( int input, bool state);
        void workReset();
        
    public:
        void setup();
        void initPubSub();
        bool onMqttMessage( String& topic, String& pay);
        void loop();
};


class Shelly1PM : public Shelly{

    private:
        void extendBaseConfig();
        void overwriteBaseConfig();
        void initMqttTopics();
        String getJsonFromConfig();
        bool setConfigFromJson( String JsonConfig, bool withPub=true);
        bool parseJsonConfig();
        void workInput( int input, bool state);
        void workReset();
        
    public:
        void setup();
        void initPubSub();
        bool onMqttMessage( String& topic, String& pay);
        void loop();
};

class Shellyi4 : public Shelly{

    private:
        void extendBaseConfig();
        void overwriteBaseConfig();
        void initMqttTopics();
        String getJsonFromConfig();
        bool setConfigFromJson( String JsonConfig, bool withPub=true);
        void workInput( int input, bool state);
        void workReset();
        
    public:
        void setup();
        void initPubSub();
        bool onMqttMessage( String& topic, String& pay);
        void loop();
};
