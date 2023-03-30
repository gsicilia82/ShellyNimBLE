#include <BleDevice.h>

// private functions

int BleDevice::median_of_3(){

    int the_max = max(max(newest, recent), oldest);
    int the_min = min(min(newest, recent), oldest);
    // unnecessarily clever code
    int the_median = the_max ^ the_min ^ newest ^ recent ^ oldest;
    return (the_median);
}

// public functions

BleDevice::BleDevice(){}

int BleDevice::getBleValue( NimBLEAdvertisedDevice *advertisedDevice){

    oldest = recent;
    recent = newest;
    newest = advertisedDevice->getRSSI();

    rssi = median_of_3();

    return rssi;
}