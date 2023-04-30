#pragma once

#include <Arduino.h>
#include <NimBLEAdvertisedDevice.h>
#include <globals.h>

#define NO_RSSI (-128)
#define N 5 // Average filter values

class BleDevice {

    private:
        struct TOPICS {
            String RssiAt1m;
        } Topic;

        int rssi = NO_RSSI, newest = NO_RSSI, recent = NO_RSSI, oldest = NO_RSSI;
        float raw = 0.0f;
        int rssi1m = -70; // default value
        

        // Average filter
        float values[N] = {};
        float sum = 0;
        int index = 0;
        //

        int get1mRssi();
        float mvgAverage( float value);
        void initMqttTopics();

    public:
        String address, alias;
        float absorption = 3.5f;
        bool rssiReceivedOverMqtt = false;

        BleDevice();
        void initPubSub();
        void publishRssi();
        bool onMqttMessage( String& top, String& pay);
        float getBleValue( NimBLEAdvertisedDevice *advertisedDevice);
        
};