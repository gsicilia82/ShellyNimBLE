#include <BleDevice.h>


int BleDevice::get1mRssi(){

    /*
    iBeacon: NimBLEBeacon -> int8_t getSignalPower()
    Allgemein: NimBLEAdvertisedDevice -> bool haveTXPower() -> int8_t getTXPower()

    Über MQTT korrigierbar machen
    */
    return -68;
}


BleDevice::BleDevice() : oneEuro{OneEuroFilter<float, unsigned long>(1, ONE_EURO_FCMIN, ONE_EURO_BETA, ONE_EURO_DCUTOFF)}{}


bool BleDevice::filter(){

    Reading<float, unsigned long> inter1, inter2;
    inter1.timestamp = millis();
    inter1.value = raw;

    return oneEuro.push(&inter1, &inter2) && diffFilter.push(&inter2, &output);
}

float BleDevice::getBleValue( NimBLEAdvertisedDevice *advertisedDevice){

    oldest = recent;
    recent = newest;
    newest = advertisedDevice->getRSSI();

    int the_max = max(max(newest, recent), oldest);
    int the_min = min(min(newest, recent), oldest);
    // unnecessarily clever code
    int the_median = the_max ^ the_min ^ newest ^ recent ^ oldest;

    rssi = the_median;

    float ratio = (get1mRssi() - rssi) / (10.0f * absorption);
    raw = pow(10, ratio);

    if ( filter() ) hasValue = true;

    #ifdef DEBUG
        Serial.print( "Device: <" + alias + "> Dist: " + String( raw));
        if ( hasValue) Serial.println( " | " + String( output.value.position));
        else Serial.println();
    #endif

    return hasValue ? output.value.position : raw;

}