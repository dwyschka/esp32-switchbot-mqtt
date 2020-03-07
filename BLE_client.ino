#include "BLEDevice.h"
#include <WiFi.h>
#include <MQTT.h>

// The remote service we wish to connect to.
static BLEUUID serviceUUID("cba20d00-224d-11e6-9fb8-0002a5d5c51b");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("cba20002-224d-11e6-9fb8-0002a5d5c51b");
uint8_t dataToWrite[] = {0x57, 0x01, 0x00};

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;
BLEClient*  pClient;

WiFiClient wifiClient;

MQTTClient client;

const char* ssid = "l"; // Enter your WiFi name
const char* password =  ""; // Enter WiFi password
const char* mqttServer = "";
const int mqttPort = 1883;
const char* mqttUser = "";
const char* mqttPassword = "";

boolean sendPush = false;
bool initializedBluetooth = false;

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    ESP.restart();
    connected = false;
          doScan = true;
    doConnect = true;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");
    connected = true;
        doConnect = false;

    return true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    Serial.println(advertisedDevice.haveServiceUUID());
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks

void connectToWifi() {
      delay(10);
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    randomSeed(micros());
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

void connectToMQTT() {
  while(!client.connect("kaffeemaschine")) {
    Serial.print(".");
    delay(500);
  }
        Serial.println("connected");
      client.subscribe("kueche/kaffeemaschine/cmnd/push");
}


void callback(String &topic, String &payload) {
            Serial.println("got mqtt message");

  if( topic == "kueche/kaffeemaschine/cmnd/push" ) {  
        if( payload == "true" )   {
          Serial.println("got mqtt message");
          sendPush = true;
        }      
    }
}

void connectToBluetooth() {
        BLEScan* pBLEScan = BLEDevice::getScan();
      pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
      pBLEScan->setInterval(1349);
      pBLEScan->setWindow(449);
      pBLEScan->setActiveScan(true);
      pBLEScan->start(5, false);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  WiFi.begin(ssid, password);
  client.begin(mqttServer, wifiClient);
  client.onMessage(callback);
  
  connectToWifi();
  connectToMQTT();
  connectToBluetooth();
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.

  
} // End of setup.


// This is the Arduino main loop function.
void loop() {
  client.loop();
  if(!client.connected()) {
      connectToMQTT();
  }

  if(sendPush) {
    if(!initializedBluetooth) {
      BLEDevice::init("");
connectToBluetooth();
     initializedBluetooth = true;
    }

      if (connectToServer()) {
        Serial.println("Push button");
        pRemoteCharacteristic->writeValue(dataToWrite, sizeof(dataToWrite));
        sendPush = false;
        delay(50);
        ESP.restart();
      } 
  }

  delay(100);
  
} // End of loop
