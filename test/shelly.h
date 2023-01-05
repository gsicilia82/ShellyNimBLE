#pragma once
#include <Arduino.h>
#include <utils.h>
#include <user_config.h>

namespace Shelly {

    void onMqttConnect();
    bool Command(String& command, String& pay);
    void Setup();
    bool Loop();
    
}  // namespace Shelly