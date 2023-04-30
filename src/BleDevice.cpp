#include <BleDevice.h>


int BleDevice::get1mRssi(){

    /*
    iBeacon: NimBLEBeacon -> int8_t getSignalPower()
    Allgemein: NimBLEAdvertisedDevice -> bool haveTXPower() -> int8_t getTXPower()

    Über MQTT korrigierbar machen
    */
    return rssi1m;
}

float BleDevice::mvgAverage( float value){
    sum -= values[index];
    sum += value;
    values[index] = value;
    index = (index + 1) % N;
    return sum / N;
}

void BleDevice::initMqttTopics(){
    Topic.RssiAt1m = TopicGlobal.Main + "/rssiAt1m/" + alias;
}

BleDevice::BleDevice(){
    rssi1m = readInt( "rssi1m", rssi1m);
}

void BleDevice::initPubSub(){

    initMqttTopics();
    mqttClient.subscribe( Topic.RssiAt1m.c_str(), 1);
}

void BleDevice::publishRssi(){
    pub( Topic.RssiAt1m, String( rssi1m), false, 0, true);
}

bool BleDevice::onMqttMessage( String& top, String& pay){

    if (top == Topic.RssiAt1m){
        if ( pay.toInt() == 0 ){
            report("MQTT payload for RssiAt1m is not valid! Restoring last value...");
            pub( Topic.RssiAt1m, String( rssi1m), true);
            return true;
        }
        Serial.println(">>> MQTT: Received RssiAt1m for device: " + alias);
        rssiReceivedOverMqtt = true;
        rssi1m = pay.toInt();
        writeInt( "rssi1m", rssi1m);
        return true;
    }
    return false;
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

    float fTmp = mvgAverage( raw);

/*
    Serial.println( "Device: <" + alias + "> Dist: " + String( raw) + " | Avg: " + String( fTmp));
    for ( float i : values) {
        Serial.print( i);
        Serial.print( " ");
    }
    Serial.println();
*/

    return fTmp;

}