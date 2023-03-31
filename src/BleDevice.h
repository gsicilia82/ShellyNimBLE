#pragma once

#include <Arduino.h>
#include <SoftFilters.h>
#include <NimBLEAdvertisedDevice.h>
#include <globals.h>

#define NO_RSSI (-128)
#define ONE_EURO_FCMIN 1e-5f
#define ONE_EURO_BETA 0.2f
#define ONE_EURO_DCUTOFF 1e-5f

class BleDevice {

    private:
        int rssi = NO_RSSI, newest = NO_RSSI, recent = NO_RSSI, oldest = NO_RSSI;
        float raw = 0.0f;
        float absorption = 3.5f;
        bool hasValue = false;

        Reading<Differential<float>> output;

        OneEuroFilter<float, unsigned long> oneEuro;
        DifferentialFilter<float, unsigned long> diffFilter;

        int get1mRssi();
        bool filter();
    public:
        String address, alias;

        BleDevice();
        float getBleValue( NimBLEAdvertisedDevice *advertisedDevice);
        
};