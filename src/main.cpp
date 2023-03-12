#include <main.h>

int median_of_3(int a, int b, int c){

    int the_max = max(max(a, b), c);
    int the_min = min(min(a, b), c);
    // unnecessarily clever code
    int the_median = the_max ^ the_min ^ a ^ b ^ c;
    return (the_median);
}

template <size_t n> void push(int (&arr)[n], int const value) {
  static size_t index = 0;

  arr[index] = value;
  index = (index + 1) % n;
}

template <size_t n> int pop(int (&arr)[n]) {
  static size_t index = 0;

  int result = arr[index];
  index = (index + 1) % n;
  return result;
}

template <size_t n> void roll(int (&arr)[n]) {
  static size_t index = 0;

  for (size_t i = 0; i < n; i++) {
    Serial.print(arr[(index + i) % n]);
    Serial.print(' ');
  }
  Serial.println();

  index = (index + 1) % n;
}

void filterStringToVector(){

    vecFilterBle.clear();
    int str_len = sFilterBle.length() + 1;
    char cFilterBle[str_len];
    sFilterBle.toCharArray( cFilterBle, str_len);

    Serial.printf( "Splitting filter string \"%s\" into tokens: \n", sFilterBle.c_str() );

    char *end_str;
    char *token = strtok_r( cFilterBle, " ,.;", &end_str);
    while (token != NULL) {
        char *end_token;
        printf ("Entry token: %s\n", token);
        char *token2 = strtok_r(token, "=", &end_token);
        int i=0;
        while (token2 != NULL)
        {
            printf("Sub tokens = %s\n", token2);
            if (i==0){
                vecFilterBle.push_back( token2);
                vecFilterAlias.push_back( token2);
            }
            else{
                vecFilterAlias.pop_back();
                vecFilterAlias.push_back( token2);
            } 
            token2 = strtok_r(NULL, "=", &end_token);
            i+=1;
        }
        token = strtok_r(NULL, ",", &end_str);
    }

    #ifdef DEBUG
        for(int i=0; i < vecFilterAlias.size(); i++){
            printf("vecFilterAlias: %s\n", vecFilterAlias[i].c_str() );
        }
        for(int i=0; i < vecFilterBle.size(); i++){
            printf("vecFilterBle: %s\n", vecFilterBle[i].c_str() );
        }
    #endif
}

void initPubSub() {

    for (int i = 0; i < 2; i++) {
        pub( Topic.Online, "true");
        pub( Topic.Ip, WiFi.localIP().toString() );
        pub( Topic.Restart, "false");
        pub( Topic.HardReset, "false");
        pub( Topic.Filter, sFilterBle);
        pub( TopicGlobal.Message, "Ready");
        if( i==0) delay(500);
    }
        mqttClient.subscribe( Topic.Restart.c_str(), 1);
        mqttClient.subscribe( Topic.HardReset.c_str(), 1);
        mqttClient.subscribe( Topic.Filter.c_str(), 1);
}

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
    char new_payload[len + 1];
    new_payload[len] = '\0';
    strncpy(new_payload, payload, len);

    String top = String(topic);
    String pay = String(new_payload);

    Serial.print("MQTT received: " + top + " | " + pay);

    if ( mqttIgnoreCounter > 0){
        mqttIgnoreCounter--;
        #ifdef DEBUG_MQTT
            Serial.printf(" >>> MQTT command ignored, remaining ignore-counter: %d!\n", mqttIgnoreCounter);
        #endif
        if ( mqttIgnoreCounter == 0) mqttDisabled = false;
        return;
    }
    Serial.println();

    if (top == Topic.Restart && pay == "true"){
        restartDevice();
    }
    else if (top == Topic.HardReset && pay == "true"){
        pub( Topic.HardReset, "false", true);
        clearPreferences();
        delay(2500);
        restartDevice();
    }
    else if (top == Topic.Filter){
        pub( Topic.Filter, pay, true);
        pay.replace( " ", "");
        sFilterBle = pay;
        filterStringToVector();
        report( "New filter activated");
        writeString( "sFilterBle", sFilterBle);
    }
    else if ( shelly->onMqttMessage( top, pay))
        ;
    else Serial.println("MQTT unknown: " + top + " | " + pay);

}


class MyAdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {

    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {

        String sAddress = advertisedDevice->getAddress().toString().c_str();
        int rssi = advertisedDevice->getRSSI();
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
            //Serial.printf("Advertised Device: %s \n", advertisedDevice->toString().c_str());
        #endif
        
        for (int i=0; i < vecFilterBle.size(); i++) {
            if ( sAddress == vecFilterBle[i] ){
                int pos = i;
                push( arrRssi[pos], rssi);
                String topic = Topic.Results + "/" + vecFilterAlias[i] + "/" + deviceName;
                int median = median_of_3( arrRssi[pos][0], arrRssi[pos][1], arrRssi[pos][2] );
                pubFast( topic, String(  median) );
                #ifdef DEBUG_MQTT
                    Serial.printf("Advertising device to topic: %s with RSSI: %d \n", topic.c_str(), median);
                #endif
                break;
            }
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
		request->send_P(200, "text/html", captive_html); 
	}
	};

void setupApServer(){

	server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
		request->send_P(200, "text/html", captive_html); 
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


void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {

    Serial.println("Disconnected from MQTT. Restarting ...");
    ESP.restart();
}


void WiFiEvent(WiFiEvent_t event) {

    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.print("WiFi connected with IP address: ");
        Serial.println( WiFi.localIP() );
        writeInt( "wifiValidated", 1);
        wifiWasConnected = true;
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
    // restart only if was connected one time, otherwise not necessary cause of loop in initNetwork()
        if ( wifiWasConnected){ 
            Serial.println("WiFi lost connection! Restarting ESP32 for clean init.");
            ESP.restart();
        }
        break;
    }
}


void onMqttConnect( bool sessionPresent) {
    Serial.println("Connected to MQTT.");
	int mqttValidated = readInt( "mqttValidated", 0);
	if ( mqttValidated == 0) writeInt( "mqttValidated", 1);
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

void initNetwork(){

	String wifiSSID   = readString( "ssid", "");
	String wifiPass   = readString( "pass", "");
	String mqttServer = readString( "server", "");
	int mqttPort      = readInt( "port", 0);

	if ( wifiSSID == "" || wifiPass == "" || mqttServer == "" || mqttPort == 0){
		initCaptivePortal();
		return;
	}

	int wifiValidated = readInt( "wifiValidated", 0);
	int mqttValidated = readInt( "mqttValidated", 0);

    // --------------------- Connect to WIFI ---------------------

    WiFi.onEvent(WiFiEvent);
    Serial.println( "Connecting to Wi-Fi " + wifiSSID + " with " + wifiPass + "...");
    WiFi.begin( wifiSSID.c_str(), wifiPass.c_str() );
    int timeout = 0;
    while ( WiFi.status() != WL_CONNECTED){
        delay(500);
        Serial.println(".");
        timeout++;
        if  (timeout > 120){
            Serial.println("");
			if ( wifiValidated == 1){
				Serial.println("WIFI not reachable, restarting ESP32");
				ESP.restart();
			}
			else {
				Serial.println("WIFI not reachable, wrong credentials? HardReset ESP32 and restarting into AP mode...");
				clearPreferences();
				delay(2000);
				ESP.restart();
			}
        }
    }
    Serial.println();

    // --------------------- Connect to MQTT ---------------------

    Serial.print( "Connecting to MQTT " + mqttServer + ":" + String(mqttPort) + "...");
	mqttClient.onConnect( onMqttConnect);
    mqttClient.onMessage( onMqttMessage);
    mqttClient.setServer( mqttServer.c_str(), mqttPort);
    mqttClient.setWill( Topic.Online.c_str(), 1, true, "false");
    String locIP = String( WiFi.localIP() );
    locIP.replace( ".", "");
    mqttClient.setClientId(  locIP.c_str() );
    mqttClient.connect();
    timeout = 0;
    while ( !mqttClient.connected() ){
        mqttClient.connect();
        delay(2000);
        Serial.print(".");
        timeout++;
        if  (timeout > 60){
            Serial.println("");
            if ( mqttValidated == 1){
				Serial.println("MQTT not reachable, restarting ESP32");
				ESP.restart();
			}
			else {
				Serial.println("MQTT not reachable, wrong configuration? HardReset ESP32 and restarting EPS32 into AP mode...");
				clearPreferences();
				delay(2000);
				ESP.restart();
			}
        }
    }
    mqttClient.onDisconnect( onMqttDisconnect);
    Serial.println();
    Serial.println("Connected to MQTT.");

}


void initMqttTopicsGlobal(){
    TopicGlobal.Main    = "shellyscanner"; // mqtt main topic
    TopicGlobal.Device  = TopicGlobal.Main + "/devices/" + deviceName;
    TopicGlobal.dbg     = TopicGlobal.Device + "/Debug/Dbg_";
    TopicGlobal.Message = TopicGlobal.Device + "/Message";
}

void initMqttTopics(){
    Topic.Online    = TopicGlobal.Device + "/Online";
    Topic.Ip        = TopicGlobal.Device + "/IP_Address";
    Topic.Restart   = TopicGlobal.Device + "/Restart";
    Topic.HardReset = TopicGlobal.Device + "/HardReset";
    Topic.Filter    = TopicGlobal.Device + "/Filter";
    Topic.Info      = TopicGlobal.Device + "/Info";

    Topic.Results   = TopicGlobal.Main + "/results";
    
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


void setup() {

    Serial.begin(115200);
    Serial.println();

    // --------------------- Init Main ---------------------

    deviceName  = readString( "name", "");
    deviceModel = readString( "model", "");
    if ( deviceName == "" || deviceModel == ""){
        Serial.println( "device model or device name is empty, starting CaptivaPortal");
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

        // --------------------- Init Network ---------------------

        initNetwork();

        #ifdef DEBUG
            Serial.println(">>> Init Network done.");
        #endif

        // --------------------- Scanner filter ---------------------

        sFilterBle = readString( "sFilterBle", sFilterBle);
        filterStringToVector();

        #ifdef DEBUG
            Serial.println(">>> Init ScanFilter done.");
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

        // --------------------- Get actual time and publish info ---------------------

        #ifdef DEBUG
            Serial.println(">>> Init boot-time ...");
        #endif

        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        Info.bootTime = getLocalTime();
        Info.model = deviceModel;
        pub( Topic.Info, Info.toString() );
        delay(250);
        // Send two times to get non-empty ioBroker entry
        pub( Topic.Info, Info.toString() );

        #ifdef DEBUG
            Serial.println(">>> Init boot-time done.");
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

        // --------------------- Publish init state ---------------------

        initPubSub();
        mqttIgnoreCounter = 0;

        #ifdef DEBUG
            Serial.println(">>> Init 1st publish of states done.");
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

        if ( pBLEScan->isScanning() == false) {
            // Start scan with: duration = 0 seconds(forever), no scan end callback, not a continuation of a previous scan.
            pBLEScan->start(0, nullptr, false);
        }

        // --------------------- OTA handler ---------------------

        ArduinoOTA.handle();

        // --------------------- Disable MQTT handler temporary ---------------------

        if ( mqttDisabled && ( millis() - mqttDisableTime > 5000) ){
            mqttDisabled = false;
            mqttIgnoreCounter = 0;
            #ifdef DEBUG_MQTT
                Serial.println("MQTT commandhandler enabled again!");
            #endif
        }

        // --------------------- Loop Shelly ---------------------

        shelly->loop();

    }

}
