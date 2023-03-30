#pragma once

#include <Arduino.h>
#include <SoftFilters.h>
#include <NimBLEAdvertisedDevice.h>
#include <globals.h>

#define NO_RSSI (-128)

class BleDevice {

    private:
        int rssi = NO_RSSI, newest = NO_RSSI, recent = NO_RSSI, oldest = NO_RSSI;
        float raw = 0;

        bool filter();
        int median_of_3();

    public:
        String address, alias;

        BleDevice();
        int getBleValue( NimBLEAdvertisedDevice *advertisedDevice);
        
};