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
            std::vector<String> Input{};
            std::vector<String> Relay{};

            String UserConfig,
            CoverState,
            CoverPosSet,
            CoverStop,
            CoverCalib,
            Power1,
            Power2,
            PowerAcc;
        } Topic;

        struct PINCONFIG {
            int ButtonReset = -1;
            std::vector<int> Input;
            std::vector<int> Relay;
            int I2C_SCL = -1;
            int I2C_SDA = -1;
            int ADE7953 = -1;
            int Temperature = -1;
        } Pin;

        std::vector<String> SwitchMode;
        String UserConfig = "";

        bool getRelayState( int relay);
        uint debounce = 100; // debounce switch input in ms

        virtual void initMqttTopics();
        virtual void initPubSub();

    public:
        Shelly();
        virtual void setup() = 0;
        virtual void loop();
        virtual bool parseJsonUserConfig() = 0;
};


class Shelly2PM : public Shelly{

    private:
        ADE7953 myADE7953;
        ENERGY Energy;

        struct PINCONFIG { /* Version 0.1.9 */
            int ButtonReset = 4;
            std::vector<int> Input{ 5, 18 };
            std::vector<int> Relay{ 13, 12 };
            int I2C_SCL = 25;
            int I2C_SDA = 26;
            int ADE7953 = 27;
            int Temperature = 35;
        } Pin;

        struct USERCONFIG {
            std::vector<String> ModeInputX{ "Switch", "Switch" };
            bool SwapInput = false;
            bool SwapOutput = false;
        } UserConfig;

        struct DEVMODE {
            String Setting = "";
            // String Variables Cover and Light for comparison purposes only!
            String Cover = "Cover";
            String Light = "Light";
        } deviceMode;

        bool measEnergy = false;
        int measIntervall;

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

        #ifdef DEBUG_CALIB
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
        // End COVER mode variables
        // #########################

        String getJsonUserConfig();
        void initMqttTopics();
        void initPubSub();

    public:
        void setup();
        void loop();
        bool parseJsonUserConfig();
};
