//
// Created by Andri Yadi on 8/25/16.
//

#include "DCX_WifiManager.h"

DCX_WifiManager::DCX_WifiManager(DCX_AppSetting &setting):
        setting_(setting) {

}

DCX_WifiManager::~DCX_WifiManager() {
}

void DCX_WifiManager::begin() {

    WiFi.mode(WIFI_STA);

    DCX_WifiManager *mySelf = this;

    connectedEventHandler_ = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected& event) {
#ifdef DEBUG_SERIAL
        Serial.print("Station connected to: ");
        Serial.println(event.ssid);
#endif
    });

    gotIPEventHandler_ = WiFi.onStationModeGotIP([mySelf](const WiFiEventStationModeGotIP& event)
                                                 {
#ifdef DEBUG_SERIAL
                                                     Serial.print("Station connected, IP: ");
                                                     Serial.println(WiFi.localIP());
#endif
                                                     mySelf->wifiDidConnected();
                                                 });

    disconnectedEventHandler_ = WiFi.onStationModeDisconnected([mySelf](const WiFiEventStationModeDisconnected& event)
                                                               {
                                                                   if (event.reason == WIFI_DISCONNECT_REASON_ASSOC_LEAVE ||
                                                                       event.reason == WIFI_DISCONNECT_REASON_AUTH_LEAVE) {
                                                                       Serial.println("Station leave");
                                                                       return;
                                                                   }

                                                                   if (event.reason == WIFI_DISCONNECT_REASON_ASSOC_LEAVE ||
                                                                       event.reason == WIFI_DISCONNECT_REASON_AUTH_LEAVE) {
                                                                       Serial.println("Station leave");
                                                                       return;
                                                                   }
#ifdef DEBUG_SERIAL
                                                                   Serial.println("Station disconnected");
                                                                   Serial.println(event.reason);
#endif
                                                                   mySelf->wifiDidDisconnected(event.reason);
                                                                   mySelf->startSmartConfig();

                                                               });

    if (!setting_.exist() || !setting_.wifiConfigured || SETTING_FORCE_INIT)
    {
        startSmartConfig();
    }
    else {// Run our method when station was connected to AP (or not connected)
        tryToConnectWifi();
    }
}

void DCX_WifiManager::begin(const char *ssid, const char *passphrase) {
    setting_.wifiConfigured = true;

#ifdef DEBUG_SERIAL
    Serial.printf("WiFi SSID: %s, Pass: %s\r\n", ssid, passphrase);
#endif

    setting_.ssidName = String(ssid);
    setting_.ssidPass = String(passphrase);

    begin();
}

void DCX_WifiManager::loop() {

//    if (connectingToWifi_ && !connectedToWifi_) {
    if (!WiFi.isConnected()) {
//        Serial.print("X");

        unsigned long _ellapsed = (millis() - wifiConnCheckingMillis_);
        if (_ellapsed >= WIFI_CONNECTING_INTERVAL) {
            wifiConnTrial_++;
            wifiConnCheckingMillis_ = millis();

            if (wifiConnectingCallback_) {
                wifiConnectingCallback_(wifiConnTrial_*WIFI_CONNECTING_INTERVAL);
            }

            if (smartConfigRequested_) {
                if (WiFi.smartConfigDone()){

                    smartConfigRequested_ = false;
//                    connectingToWifi_ = false;
#ifdef DEBUG_SERIAL
                    Serial.println(F("SmartConfig Success"));
#endif
                }
                else {
                    if (wifiConnTrial_ > (120000 / WIFI_CONNECTING_INTERVAL)) {
                        smartConfigRequested_ = false;
#ifdef DEBUG_SERIAL
                        Serial.println(F("SmartConfig give up"));
#endif
                        WiFi.stopSmartConfig();
                        if (wifiDisconnectedHandler_) {
                            wifiDisconnectedHandler_(WIFI_DISCONNECT_REASON_UNSPECIFIED);
                        }
//                        connectingToWifi_ = false;
                    }
                }
            }
            delay(1);
        }
    }
}

void DCX_WifiManager::startSmartConfig() {
    if (smartConfigRequested_) {
        return;
    }

    smartConfigRequested_ = true;

    DEBUG_SERIAL("DAMN, RECONFIG WIFI USING SMART CONFIG\n");

    WiFi.disconnect();

    setting_.wifiConfigured = false; //make sure, but don't save

    WiFi.stopSmartConfig(); //make sure
    bool success = WiFi.beginSmartConfig();
    if (!success) {
        DEBUG_SERIAL("DAMN, SMART CONFIG FAILED\n");
        smartConfigRequested_ = false;
    }
    else {

//        connectedToWifi_ = false;
//        connectingToWifi_ = true;
        //smartConfigRequested_ = true;

        wifiConnCheckingMillis_ = millis();
        wifiConnTrial_ = 0;

//        if (tickerSmartConfig == NULL) {
//            tickerSmartConfig = new Ticker();
//        }
//
//        //tickerSmartConfig->detach();
//        delay(10);
//
        if (wifiConnectStartedCallback_) {
            wifiConnectStartedCallback_();
        }
//
//        tickerSmartConfig->attach(0.5, checkWifiConn, this);
    }
}

void DCX_WifiManager::onWifiConnectStarted(WifiConnectionCallback cb) {
    wifiConnectStartedCallback_ = cb;
}

void DCX_WifiManager::onWifiConnected(WifiConnectedCallback cb) {
    wifiConnectedHandler_ = cb;
}

void DCX_WifiManager::onWifiDisconnected(WifiDisconnectedCallback cb) {
    wifiDisconnectedHandler_ = cb;
}

void DCX_WifiManager::onWifiConnecting(WifiConnectingCallback cb) {
    wifiConnectingCallback_ = cb;
}


void DCX_WifiManager::setWifiConnecting() {
//    if (wifiConnectingCallback_) {
//        wifiConnectingCallback_(wifiConnTrial_*WIFI_CONNECTING_INTERVAL);
//    }
}

void DCX_WifiManager::wifiDidConnected() {

//    connectedToWifi_ = true;
//    connectingToWifi_ = false;

    smartConfigRequested_ = false;

#ifdef DEBUG_SERIAL
    WiFi.printDiag(Serial);
#endif

    boolean newConnection = (setting_.wifiConfigured == 0);

    IPAddress _localIP = WiFi.localIP();
    setting_.ipAddr = _localIP;
    setting_.wifiConfigured = true;

    static struct station_config conf;
    wifi_station_get_config(&conf);
    const char* ssid = reinterpret_cast<const char*>(conf.ssid);
    const char* passphrase = reinterpret_cast<const char*>(conf.password);

#ifdef DEBUG_SERIAL
    Serial.printf("WiFi SSID: %s, Pass: %s\r\n", ssid, passphrase);
#endif

    setting_.ssidName = String(ssid);
    setting_.ssidPass = String(passphrase);

    setting_.save();

    if (wifiConnectedHandler_) {
        wifiConnectedHandler_(newConnection);
    }

}

void DCX_WifiManager::wifiDidDisconnected(WiFiDisconnectReason reason) {
//    connectingToWifi_ = false;
//    connectedToWifi_ = false;

    smartConfigRequested_ = false;

    if (wifiDisconnectedHandler_) {
        wifiDisconnectedHandler_(reason);
    }

//    if (!smartConfigRequested_) {
//        startSmartConfig();
//    }
}

void DCX_WifiManager::tryToConnectWifi() {

//    cleanupTickerCheckConn();
//    cleanupTickerSmartConfig();

    DEBUG_SERIAL("YUHU, WIFI CONFIG READY! %s:%s\n", setting_.ssidName.c_str(), setting_.ssidPass.c_str());
    //WiFiMulti.addAP(settings_.ssidName.c_str(), settings_.ssidPass.c_str()); // Put you SSID and Password here
    //WifiStation.waitConnection(connectedDelegate_, 30, notConnectedDelegate_); // We recommend 20+ seconds at start

//    connectedToWifi_ = false;
//    connectingToWifi_ = true;

    smartConfigRequested_ = false;

    wifiConnTrial_ = 0;
    wifiConnCheckingMillis_ = millis();

    WiFi.begin(setting_.ssidName.c_str(), setting_.ssidPass.c_str());

    if (wifiConnectStartedCallback_) {
        wifiConnectStartedCallback_();
    }
}

bool DCX_WifiManager::isWifiConnected() {
    //return connectedToWifi_;
    return WiFi.isConnected();
}



