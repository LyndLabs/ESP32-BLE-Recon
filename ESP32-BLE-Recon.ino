// add TX POWER
// toggle scan / interval / window via serial

#define PIN       1
#define NUMPIXELS 1

#define FW "v0.0_240301"
#define JSONSTR "{\"ID\":\"%s\",\"FW\":\"%s\",\"TYPE\":\"%s\",\"UUID\":\"%s\",\"MFR\":\"%s\",\"NAME\":\"%s\",\"MAC\":\"%s\",\"RSSI\":\"%i\",\"TX\":\"%i\"%s}\n\r"

#define SCANTIME 5
#define SCANTYPE ACTIVE
#define SCAN_INT 100
#define SCAN_WINDOW 99

#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEEddystoneURL.h>
#include <BLEEddystoneTLM.h>
#include <BLEBeacon.h>
#include <Adafruit_NeoPixel.h>

#define ENDIAN_CHANGE_U16(x) ((((x)&0xFF00) >> 8) + (((x)&0xFF) << 8))

int scanTime = SCANTIME; //In seconds
BLEScan *pBLEScan;

// Adafruit NeoPixel for headless status
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

uint8_t sta_mac[6];
char strID[18];
char strAddl[200];

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice)  {
      // reset strings
      char type[9]; char charManufacturerData[100];
      char uuidStr[37];
      
      strcpy(uuidStr,"");
      strcpy(strAddl,"");
      strcpy(type,"BLE");
      strcpy(charManufacturerData,"");

      if (advertisedDevice.haveName()) {}

      // SERVICE UUID
      if (advertisedDevice.haveServiceUUID()) {
        BLEUUID devUUID = advertisedDevice.getServiceUUID();
        sprintf(uuidStr,devUUID.toString().c_str());
      }
      
      else {
        // IBEACON
        if (advertisedDevice.haveManufacturerData() == true) {
          std::string strManufacturerData = advertisedDevice.getManufacturerData();

          uint8_t cManufacturerData[100];
          strManufacturerData.copy((char *)cManufacturerData, strManufacturerData.length(), 0);

          if (strManufacturerData.length() == 25 && cManufacturerData[0] == 0x4C && cManufacturerData[1] == 0x00) {
            BLEBeacon oBeacon = BLEBeacon();
            oBeacon.setData(strManufacturerData);

            strcpy(type,"IBEACON");
            strcpy(uuidStr,oBeacon.getProximityUUID().toString().c_str());
            sprintf(charManufacturerData,"%04X",oBeacon.getManufacturerId());

            sprintf(strAddl,",\"ADDL\":{\"MAJ\":\"%d\",\"MIN\":\"%d\",\"PWR\":\"%d\"}",ENDIAN_CHANGE_U16(oBeacon.getMajor()),ENDIAN_CHANGE_U16(oBeacon.getMinor()),oBeacon.getSignalPower());
          }

          /***** OTHER MANUFACTURER *****/
          else {
            for (int i = 0; i < strManufacturerData.length(); i++) {
              sprintf(charManufacturerData+i*2,"%02X",cManufacturerData[i]);
            }
          }
        }

        Serial.printf(JSONSTR,FW,strID,type,uuidStr,charManufacturerData,advertisedDevice.getName().c_str(),advertisedDevice.getAddress().toString().c_str(),advertisedDevice.getRSSI(),advertisedDevice.getTXPower(),strAddl);
        return;
      }

      /***** EDDYSTONE *****/ 
      uint8_t *payLoad = advertisedDevice.getPayload();
      BLEUUID checkUrlUUID = (uint16_t)0xfeaa;
      
      if (advertisedDevice.getServiceUUID().equals(checkUrlUUID)) {
        
        /***** URL BEACON *****/
        if (payLoad[11] == 0x10) {
          strcpy(type,"EDDY-URL");
          BLEEddystoneURL foundEddyURL = BLEEddystoneURL();
          std::string eddyContent((char *)&payLoad[11]); // incomplete EddystoneURL struct!

          foundEddyURL.setData(eddyContent);
          std::string bareURL = foundEddyURL.getURL();

          // invalid data handling: need to field test this
          if (bareURL[0] == 0x00) {
            size_t payLoadLen = advertisedDevice.getPayloadLength();
            for (int idx = 0; idx < payLoadLen; idx++) { 
              //Serial.printf("0x%08X ", payLoad[idx]); 
              eddyContent[idx] = payLoad[idx];
            }
            foundEddyURL.setData(eddyContent);
            sprintf(strAddl,",\"ADDL\":{\"DATA\":\"%s\",\"PWR\":\"%d\"}",foundEddyURL.getURL().c_str(),foundEddyURL.getPower());
            // Serial.printf(JSONSTR,FW,strID,type,uuidStr,charManufacturerData,advertisedDevice.getName().c_str(),advertisedDevice.getAddress().toString().c_str(),advertisedDevice.getRSSI(),advertisedDevice.getTXPower(), strAddl);
            // return;
          }
          sprintf(strAddl,",\"ADDL\":{\"URL\":\"%s\",\"PWR\":\"%d\"}",foundEddyURL.getDecodedURL().c_str(),foundEddyURL.getPower());
        }

        /***** TLM BEACON *****/
        else if (payLoad[11] == 0x20) {

          strcpy(type,"EDDY-TLM");
          BLEEddystoneTLM foundEddyURL = BLEEddystoneTLM();
          std::string eddyContent((char *)&payLoad[11]); // incomplete EddystoneURL struct!

          eddyContent = "01234567890123";

          for (int idx = 0; idx < 14; idx++) { eddyContent[idx] = payLoad[idx + 11]; }

          foundEddyURL.setData(eddyContent);
          int temp = (int)payLoad[16] + (int)(payLoad[15] << 8);
          float calcTemp = temp / 256.0f;

          // check calcTemp Example: (double) foundEddyURL.getTemp()
          // URL, VOLT, TEMP, ADV. COUNT, LAST REBOOT
          sprintf(strAddl,",ADDL:{\"URL\":\"%s\",\"VOLT\":\"%i\",\"TEMP\":\"%.2fC\",\"ACNT\":\"%d\",\"LTIME\":\"%d\"}",foundEddyURL.toString().c_str(),foundEddyURL.getVolt(),calcTemp);
        }
      }
      
       Serial.printf(JSONSTR,FW,strID,type,uuidStr,charManufacturerData,advertisedDevice.getName().c_str(),advertisedDevice.getAddress().toString().c_str(),advertisedDevice.getRSSI(),advertisedDevice.getTXPower(),strAddl);
    }
};

// provide scan interval / debug stats over Serial / runtime
// void getReport() { }

void setup() {
  Serial.begin(115200);
  pixels.begin();

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(SCAN_INT);
  pBLEScan->setWindow(SCAN_WINDOW);       // less or equal setInterval value

  // Green Status on Startup
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0, 150, 0));
  pixels.show();

  // Bluetooth MAC Address 
  esp_read_mac(sta_mac,ESP_MAC_BT);
  sprintf(strID,"%02X:%02X:%02X:%02X:%02X:%02X",sta_mac[0],sta_mac[1],sta_mac[2],sta_mac[3],sta_mac[4],sta_mac[5]);
  delay(500);
}

void loop() {
  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(0, 0, 150));
  pixels.show();

  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory

  pixels.clear();
  pixels.setPixelColor(0, pixels.Color(150, 0, 0));
  pixels.show();
  // delay(2000);
  //   Serial.println(foundDevices.getCount());
}


// https://github.com/nkolban/esp32-snippets/issues/858