#include <main.h>

void createBleDevices(){

    vecBleDevices.clear();
    int str_len = sFilterBle.length() + 1;
    char cFilterBle[str_len];
    sFilterBle.toCharArray( cFilterBle, str_len);

    Serial.printf( "Splitting filter string \"%s\" into tokens: \n", sFilterBle.c_str() );

    BleAbsorbtion = readFloat( "BleAbsorbtion", BleAbsorbtion);
    char *end_str;
    char *token = strtok_r( cFilterBle, " ,.;", &end_str);
    while (token != NULL) {
        char *end_token;
        printf ("Entry token: %s\n", token);
        char *token2 = strtok_r(token, "=", &end_token);
        vecBleDevices.emplace_back( BleDevice() );
        size_t last = vecBleDevices.size() - 1;
        int i=0;
        while (token2 != NULL)
        {
            printf("Sub tokens = %s\n", token2);
            if (i==0){
                vecBleDevices[last].absorption = BleAbsorbtion;
                vecBleDevices[last].address = token2;
                vecBleDevices[last].alias = token2;
            }
            else if (i==1){
                vecBleDevices[last].alias = token2;
            } 
            token2 = strtok_r(NULL, "=", &end_token);
            i+=1;
        }
        token = strtok_r(NULL, ",", &end_str);
    }

    #ifdef DEBUG
        for(int i=0; i < vecBleDevices.size(); i++){
            printf("vecBleDevices Alias: %s\n", vecBleDevices[i].alias.c_str() );
        }
        for(int i=0; i < vecBleDevices.size(); i++){
            printf("vecBleDevices Address: %s\n", vecBleDevices[i].address.c_str() );
        }
    #endif
}

void initPubSub() {

    Serial.println("MQTT: Publish all states to server.");

    shelly->initPubSub();

    BleAbsorbtion = readFloat( "BleAbsorbtion", BleAbsorbtion);

    for (int i = 0; i < 2; i++) {
        pub( Topic.Online, "true");
        pub( Topic.Ip, WiFi.localIP().toString() );
        pub( Topic.Restart, "false", true);
        pub( Topic.HardReset, "false", true);
        pub( Topic.Filter, sFilterBle, true);
        pub( Topic.Absorbtion, String( BleAbsorbtion), true);
        pub( TopicGlobal.Message, "Ready");
        if( i==0) delay(500);
    }

    mqttClient.subscribe( Topic.Restart.c_str(), 1);
    mqttClient.subscribe( Topic.HardReset.c_str(), 1);
    mqttClient.subscribe( Topic.Filter.c_str(), 1);
    mqttClient.subscribe( Topic.Absorbtion.c_str(), 1);
}

bool onMqttMessageBleDevices( String top, String pay){

    if (top == Topic.Absorbtion){
        if ( pay.toFloat() == 0 ){
            report("MQTT payload for Absorbtion is non-numeric! Restoring last value...");
            pub( Topic.Absorbtion, String( BleAbsorbtion), true);
            return true;
        }
        float absorbtion = pay.toFloat();
        BleAbsorbtion = absorbtion;
        writeFloat( "BleAbsorbtion", absorbtion);
        for(int i=0; i < vecBleDevices.size(); i++){
            vecBleDevices[ i].absorption = absorbtion;
        }
        report("Calculation is using new absorbtion.");
        return true;
    }

    for(int i=0; i < vecBleDevices.size(); i++){
        if ( vecBleDevices[ i].onMqttMessage( top, pay) ) return true;
    }

    return false;
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    char new_payload[len + 1];
    new_payload[len] = '\0';
    strncpy(new_payload, payload, len);

    String top = String(topic);
    String pay = String(new_payload);

    Serial.println("MQTT received: " + top + " | " + pay);

    if ( mqttIgnoreCounter > 0){
        mqttIgnoreCounter--;
        Serial.printf(" >>> MQTT command ignored, remaining ignore-counter: %d\n", mqttIgnoreCounter);
        if ( mqttIgnoreCounter == 0){
            mqttDisabled = false;
            Serial.println(">>> MQTT commandhandler enabled again!");
        }
        return;
    }
    Serial.println();

    if (top == Topic.Restart){
        if ( pay == "true"){
            restartDevice();
        }
    }
    else if (top == Topic.HardReset){
        if ( pay == "true"){
            pub( Topic.HardReset, "false", true);
            clearPreferences();
            delay(1000);
            restartDevice();
        }
    }
    else if (top == Topic.Filter){
        pub( Topic.Filter, pay, true);
        pay.replace( " ", "");
        sFilterBle = pay;
        createBleDevices();
        report( "New filter activated");
        writeString( "sFilterBle", sFilterBle);
    }
    else if ( shelly->onMqttMessage( top, pay))
        ;
    else if ( onMqttMessageBleDevices( top, pay))
        ;
    else Serial.println("MQTT unknown: " + top + " | " + pay);

}


class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {

    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {

        String sAddress = advertisedDevice->getAddress().toString().c_str();
        std::string strManufacturerData = advertisedDevice->getManufacturerData();

        String manuf = Sprintf("%02x%02x", strManufacturerData[1], strManufacturerData[0]);

        if (manuf == "004c") { // Apple
            if (strManufacturerData.length() == 25 && strManufacturerData[2] == 0x02 && strManufacturerData[3] == 0x15){
                NimBLEBeacon iBeacon = NimBLEBeacon();
                iBeacon.setData( strManufacturerData);
                sAddress = Sprintf("%s-%u-%u", std::string(iBeacon.getProximityUUID()).c_str(), ENDIAN_CHANGE_U16(iBeacon.getMajor()), ENDIAN_CHANGE_U16(iBeacon.getMinor()));
                #ifdef DEBUG_MQTT
                    Serial.printf( "iBeacon found with UUID: <%s> \n", sAddress.c_str() );
                #endif
            }
        }

        #ifdef DEBUG_MQTT
            Serial.printf("Advertised Device: %s \n", advertisedDevice->toString().c_str());
        #endif
        
        for (int i=0; i < vecBleDevices.size(); i++) {
            if ( vecBleDevices[i].reportBleValue( sAddress, advertisedDevice) ) break;
        }
    }

};



class CaptiveRequestHandler : public AsyncWebHandler {
	public:
	CaptiveRequestHandler() {}
	virtual ~CaptiveRequestHandler() {}

	bool canHandle(AsyncWebServerRequest *request){
		//request->addInterestingHeader("ANY");
		return true;
	}

	void handleRequest(AsyncWebServerRequest *request) {
		request->send(SPIFFS, "/index.html", "text/html"); 
	}
	};

void setupApServer(){

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send(SPIFFS, "/index.html", "text/html"); 
		Serial.println("Client Connected");
	});
    
	server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
		String inputMessage;

		String ssid, pass, server, port, name, model, mode, input1, input2;
        int intPort = 0;

		int receivedParams = 0;

        Serial.println( "Received Parameters including default values:");
		
		if (request->hasParam( "ssid")) {
			ssid = request->getParam( "ssid")->value();
			ssid.replace(" ", "");
			Serial.print( "Received SSID: ");
			Serial.println( ssid);
			writeString( "ssid", ssid);
			if ( ssid.length() > 0 ) receivedParams++;
		}
		if (request->hasParam( "pass")) {
			pass = request->getParam( "pass")->value();
			pass.replace(" ", "");
			Serial.print( "Received Password: ");
			Serial.println( pass);
			writeString( "pass", pass);
			if ( pass.length() > 0 ) receivedParams++;
		}
		if (request->hasParam( "server")) {
			server = request->getParam( "server")->value();
			server.replace(" ", "");
			Serial.print( "Received MQTT Server: ");
			Serial.println( server);
			writeString( "server", server);
			if ( server.length() > 0 ) receivedParams++;
		}
		if (request->hasParam( "port")) {
			port = request->getParam( "port")->value();
			port.replace(" ", "");
			Serial.print( "Received MQTT Port: ");
			Serial.println( port);
			intPort = port.toInt();
			writeInt( "port", intPort);
			if ( intPort > 0 ) receivedParams++;
		}
		if (request->hasParam( "name")) {
			name = request->getParam( "name")->value();
			name.replace(" ", "");
			Serial.print( "Received Device Name: ");
			Serial.println( name);
			writeString( "name", name);
			if ( name.length() > 0 ) receivedParams++;
		}
		if (request->hasParam( "SelectModel")) {
			model = request->getParam( "SelectModel")->value();
			model.replace(" ", "");
			Serial.print( "Received Device Model: ");
			Serial.println( model);
			writeString( "model", model);
		}
        if (request->hasParam( "deviceMode")) {
			mode = request->getParam( "deviceMode")->value();
			mode.replace(" ", "");
			Serial.print( "Received Device Mode: ");
			Serial.println( mode);
			writeString( "mode", mode);
		}
        if (request->hasParam( "input1")) {
			input1 = request->getParam( "input1")->value();
			input1.replace(" ", "");
			Serial.print( "Received Input1 Mode: ");
			Serial.println( input1);
			writeString( "input1", input1);
		}
        if (request->hasParam( "input2")) {
			input2 = request->getParam( "input2")->value();
			input2.replace(" ", "");
			Serial.print( "Received Input2 Mode: ");
			Serial.println( input2);
			writeString( "input2", input2);
		}

		if ( receivedParams == 5){
			request->send(200, "text/plain", "All parameters received. ESP will restart and connect to your Wifi ...");
			delay(3000);
			ESP.restart();
		}
		else {
			Serial.println( "Missing or wrong parameter(s) received. Please check again, you will be redirected automatically...");
			request->redirect("/");
		}

	});
}


void initCaptivePortal(){

    captivePortalActivated = true;

	Serial.println("Setting up AP Mode");
	WiFi.mode(WIFI_AP);


	WiFi.softAP("esp32-Shelly");
	Serial.print("AP IP address: ");Serial.println(WiFi.softAPIP());
	Serial.println("Setting up Async WebServer");

	setupApServer();

	Serial.println("Starting DNS Server");
	dnsServer.start(53, "*", WiFi.softAPIP());
	server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);//only when requested from AP
	server.begin();
}


String getLocalTime(){

    struct tm timeinfo;
    delay( 500);
    if( !getLocalTime( &timeinfo)){
        Serial.println("Failed to obtain time (1st)!");
        delay( 2000);
        if( !getLocalTime( &timeinfo)){
            Serial.println("Failed to obtain time (2nd)!");
            delay( 4000);
            if( !getLocalTime( &timeinfo)){
                Serial.println("Failed to obtain time (3rd)!");
                return "Error-Getting-Time";
            }
        }
    }
    char timeStringBuff[50]; //50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "%B %d %Y %H:%M", &timeinfo);
    String asString(timeStringBuff);
    return asString;
}

void publishInfo(){
    #ifdef DEBUG
        Serial.println(">>> publish Info ...");
    #endif

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Info.bootTime = getLocalTime();
    Info.model = deviceModel;
    pub( Topic.Info, Info.toString() );
    delay(250);
    // Send two times to get non-empty ioBroker entry
    pub( Topic.Info, Info.toString() );
}


void connectToMqtt() {

    String mqttServer = readString( "server", "");
	int mqttPort      = readInt( "port", 0);

    if (mqttServer == "" || mqttPort == 0){
        clearPreferences();
		initCaptivePortal();
		return;
	}

    Serial.println( "Connecting to MQTT " + mqttServer + ":" + String(mqttPort) + "...");
    mqttClient.setServer( mqttServer.c_str(), mqttPort);
    mqttClient.connect();
}


void onMqttConnect( bool sessionPresent) {
    Serial.println("Connected to MQTT.");
    scanAutostart = true;
	int mqttValidated = readInt( "mqttValidated", 0);
	if ( mqttValidated == 0) writeInt( "mqttValidated", 1);

    delay(1000);

    initPubSub();

    publishInfo();

    if ( !firstConnectAfterBoot){
        Serial.println( "Timer for re-publish all states started...");
        xTimerStart( mqttRePublishAgain, 0);
    }

}


void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    Serial.println("Disconnected from MQTT.");
    scanAutostart = false;
    pBLEScan->stop();
    firstConnectAfterBoot = false;
    int mqttValidated = readInt( "mqttValidated", 0);
    if ( mqttValidated == 1){
        if ( WiFi.isConnected() ) {
            xTimerStart(mqttReconnectTimer, 0);
        }
    }
    else {
        Serial.println("MQTT not reachable, wrong configuration? HardReset ESP32 and restarting EPS32 into AP mode...");
        clearPreferences();
        delay(2000);
        ESP.restart();
    }
}


void WiFiEvent(WiFiEvent_t event) {

    String locIP;
    switch(event) {
        case SYSTEM_EVENT_STA_GOT_IP:
            Serial.print("WiFi connected with IP address: ");
            Serial.println( WiFi.localIP() );
            writeInt( "wifiValidated", 1);
            locIP = "Scanner_" + WiFi.localIP().toString();
            locIP.replace( ".", "");
            Serial.println("MQTT: Connecting to broker with ID: " + locIP);
            mqttClient.setClientId( locIP.c_str());
            connectToMqtt();
            break;

        case SYSTEM_EVENT_STA_DISCONNECTED:
            Serial.println("WiFi lost connection / WiFi not connectable");
            int wifiValidated = readInt( "wifiValidated", 0);
            if ( wifiValidated == 1){
                xTimerStop(mqttReconnectTimer, 0);
            }
            else {
                Serial.println("WIFI not reachable, wrong credentials? HardReset ESP32 and restarting into AP mode...");
                clearPreferences();
                delay(2000);
                ESP.restart();
            }
            break;

    }
}


void connectToWifi() {

    String wifiSSID   = readString( "ssid", "");
	String wifiPass   = readString( "pass", "");

    if ( wifiSSID == "" || wifiPass == ""){
        clearPreferences();
		initCaptivePortal();
		return;
	}

    Serial.println( "Connecting to Wi-Fi " + wifiSSID + " with " + wifiPass + "...");
    WiFi.begin( wifiSSID.c_str(), wifiPass.c_str() );
}


void initNetwork(){

    mqttRePublishAgain = xTimerCreate("wifiTimer", pdMS_TO_TICKS(30000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(initPubSub));
    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(5000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));

    WiFi.onEvent(WiFiEvent);

    firstConnectAfterBoot = true;

    mqttClient.setWill( Topic.Online.c_str(), 1, true, "false");
	mqttClient.onConnect( onMqttConnect);
    mqttClient.onDisconnect( onMqttDisconnect);
    mqttClient.onMessage( onMqttMessage);

    mqttIgnoreCounter = 0;

    connectToWifi();

}


void initMqttTopicsGlobal(){
    TopicGlobal.Main    = "shellyscanner"; // mqtt main topic
    TopicGlobal.Device  = TopicGlobal.Main + "/devices/" + deviceName;
    TopicGlobal.Results = TopicGlobal.Main + "/results";
    TopicGlobal.dbg     = TopicGlobal.Device + "/Debug/Dbg_";
    TopicGlobal.Message = TopicGlobal.Device + "/Message";
}

void initMqttTopics(){
    Topic.Online     = TopicGlobal.Device + "/Online";
    Topic.Ip         = TopicGlobal.Device + "/IP_Address";
    Topic.Restart    = TopicGlobal.Device + "/Restart";
    Topic.HardReset  = TopicGlobal.Device + "/HardReset";
    Topic.Filter     = TopicGlobal.Device + "/Filter";
    Topic.Absorbtion = TopicGlobal.Device + "/Absorbtion";
    Topic.Info       = TopicGlobal.Device + "/Info";
}


void setup() {

    Serial.begin(115200);
    Serial.println();

    if(!SPIFFS.begin()){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }

    // --------------------- Init Main ---------------------

    deviceName  = readString( "name", "");
    deviceModel = readString( "model", "");
    if ( deviceName == "" || deviceModel == ""){
        Serial.println( "device model or device name is empty, starting CaptivaPortal");
        clearPreferences();
        initCaptivePortal();
    }
    else {

        // --------------------- Get ESP32 CPU core info ---------------------
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        Serial.println(">>> Detected esp32 CPU cores: " + String( chip_info.cores) );

        // --------------------- Init MQTT Topics ---------------------

        initMqttTopicsGlobal();
        initMqttTopics();

        #ifdef DEBUG
            Serial.println(">>> Init MQTT Topics done.");
        #endif

        // --------------------- Setup Shelly ---------------------

        if ( deviceModel == "ShellyPlus-1(PM)"){
            static Shelly1PM specificShelly;
            shelly = &specificShelly;
        }
        else if ( deviceModel == "ShellyPlus-2PM"){
            static Shelly2PM specificShelly;
            shelly = &specificShelly;
        }
        else if ( deviceModel == "ShellyPlus-i4"){
            static Shellyi4 specificShelly;
            shelly = &specificShelly;
        }
        else {
            Serial.println("### ERROR: Device model unknown: " + deviceModel);
        }

        shelly->setup();

        #ifdef DEBUG
            Serial.println(">>> Init Shelly done.");
        #endif

        // --------------------- Scanner filter ---------------------

        sFilterBle = readString( "sFilterBle", sFilterBle);
        createBleDevices();

        #ifdef DEBUG
            Serial.println(">>> Init ScanFilter done.");
        #endif

        // --------------------- Init Network ---------------------

        initNetwork();

        #ifdef DEBUG
            Serial.println(">>> Init Network done.");
        #endif

        // --------------------- Scanner process ---------------------

        NimBLEDevice::init("");

        pBLEScan = NimBLEDevice::getScan();

        pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks(), true);
        pBLEScan->setMaxResults(0); // do not store the scan results, use callback only.

        #ifdef DEBUG
            Serial.println(">>> Init BLE scan process done.");
        #endif

        // --------------------- Setup OTA function for Arduino or PlatformIO ---------------------

        ArduinoOTA
            .onStart([]() {
                report( "OTA update started ...");
                String type;
                if (ArduinoOTA.getCommand() == U_FLASH)
                    type = "sketch";
                else // U_SPIFFS
                    type = "filesystem";

                // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
                Serial.println("Start updating " + type);
            })
            .onEnd([]() {
                Serial.println("\nEnd");
            })
            .onProgress([](unsigned int progress, unsigned int total) {
                Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
            })
            .onError([](ota_error_t error) {
                Serial.printf("Error[%u]: ", error);
                if      (error == OTA_AUTH_ERROR)    Serial.println("Auth Failed");
                else if (error == OTA_BEGIN_ERROR)   Serial.println("Begin Failed");
                else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
                else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
                else if (error == OTA_END_ERROR)     Serial.println("End Failed");
            });

        ArduinoOTA.begin();

        #ifdef DEBUG
            Serial.println(">>> Init Arduino OTA done.");
        #endif

        // --------------------- Start OTA function over WebServer ---------------------

        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->redirect("/update");
        });

        AsyncElegantOTA.begin(&server);    // Start ElegantOTA on "/update"

        server.begin();

        #ifdef DEBUG
            Serial.println(">>> Init OTA webserver done.");
        #endif
    }

}


void loop() {

    // --------------------- CaptivePortal handler ---------------------

    if ( captivePortalActivated){
        dnsServer.processNextRequest();
    }
    else {

        // --------------------- BLE Scanner Autostart ---------------------

        if ( pBLEScan->isScanning() == false && scanAutostart) {
            // Start scan with: duration = 0 seconds(forever), no scan end callback, not a continuation of a previous scan.
            pBLEScan->start(0, nullptr, false);
        }

        // --------------------- OTA handler ---------------------

        ArduinoOTA.handle();

        // --------------------- Disable MQTT handler temporary ---------------------

        if ( mqttDisabled && ( millis() - mqttDisableTime > 3000) ){
            mqttDisabled = false;
            mqttIgnoreCounter = 0;
            Serial.println(">>> MQTT commandhandler enabled again!");

            // Init PubSub after enabled MQTT to receive retained RssiAt1m values
            if ( !bleDevicesInitPubSubDone){
                bleDevicesInitPubSubDone = true;
                bleDevicesPubSubTime = millis();
                Serial.println(">>> Init PubSub for BLE devices!");
                for(int i=0; i < vecBleDevices.size(); i++){
                    vecBleDevices[ i].initPubSub();
                }
            }
        }

        // --------------------- Loop Shelly ---------------------

        static unsigned long shellyLoop = 0;
        if ( millis() - shellyLoop > 100){
            shellyLoop = millis();
            shelly->loop();
        }

        // --------------------- Check / Init RssiAt1m for BLE devices ---------------------
        
        if ( !bleDevicesRssiInitialized && bleDevicesPubSubTime > 0 && ( millis() - bleDevicesPubSubTime > 5000) ){
            bleDevicesRssiInitialized = true;
            for(int i=0; i < vecBleDevices.size(); i++){
                if ( !vecBleDevices[ i].rssiReceivedOverMqtt){
                    Serial.println(">>> MQTT: RssiAt1m not set! Publish for device: " + vecBleDevices[ i].alias);
                    vecBleDevices[ i].publishRssi();
                }
            }
        }

        // --------------------- Slow Loop ---------------------

        static unsigned long lastSlowLoop = 0;
        if ( millis() - lastSlowLoop > 60000){
            lastSlowLoop = millis();
            Serial.printf("Free memory heap: %u bytes free\r\n", ESP.getFreeHeap() );
            pub( Topic.Online, "true");
            pub( TopicGlobal.Message, "Ready");
        }

    }

}
