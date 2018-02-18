//ESP8266 Community V2.4 (In Board Manager)
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include "WiFiManager.h"         //https://github.com/tzapu/WiFiManager

//------- Install From Library Manager -------
#include <ArduinoJson.h>
#include <CoinMarketCapApi.h>

//--------------------------------------------
#include <NeoPixelBus.h>
//optimized ESP8266 neopixel library:  https://github.com/Makuna
//https://github.com/Makuna/NeoPixelBus/wiki/NeoPixelBus-object
//This library is optimized to use the DMA on the ESP8266 for minimal cup usage. The standard Adafruit library has the potential to interfere with the 
//WiFi processing done by the low level SDK
//NeoPixelBus<FEATURE, METHOD> strip(pixelCount, pixelPin);
// NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(16);
//On the ESP8266 the Neo800KbpsMethod method will use this underlying method: NeoEsp8266Dma800KbpsMethod 
//The NeoEsp8266Dma800KbpsMethod is the underlying method that gets used if you use Neo800KbpsMethod on Esp8266 platforms. There should be no need to use it directly.
//The NeoEsp8266Dma800KbpsMethod only supports the RDX0/GPIO3 pin. The Pin argument is omitted. See other esp8266 methods below if you don't have this pin available.
//This method uses very little CPU for actually sending the data to NeoPixels but it requires an extra buffer for the DMA to read from. 
//Thus there is a trade off of CPU use versus memory use. The extra buffer needed is four times the size of the primary pixel buffer.
// It also requires the use of the RDX0/GPIO3 pin. The normal feature of this pin is the "Serial" receive. 
//Using this DMA method will not allow you to receive serial from the primary Serial object; but it will not stop you from sending output to the terminal program of a PC
//Due to the pin overlap, there are a few things to take into consideration.
//First, when you are flashing the Esp8266, some LED types will react to the flashing and turn on. 
//This is important if you have longer strips of pixels where the power use of full bright might exceed your design.
//Second, the NeoPixelBus::Begin() MUST be called after the Serial.begin(). 
//If they are called out of order, no pixel data will be sent as the Serial reconfigured the RDX0/GPIO3 pin to its needs.


//--------------------------------------------

#define OLED_DISPLAY

#ifdef OLED_DISPLAY
//------- OLED display -------
#include <Arduino.h>
#include <U8g2lib.h>


#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

//  UNO connections -> use hardware I2C connections
//  SDA ->A4
//  SCL ->A5

//  ESP8266 module ->use hardware I2C connections
//  SDA ->NodeMCU D2 pin (GPIO4)
//  SCL ->NodeMCU D1 pin (GPIO5)

/*
  U8glib  Overview:
    Frame Buffer Examples: clearBuffer/sendBuffer. Fast, but may not work with all Arduino boards because of RAM consumption
    Page Buffer Examples: firstPage/nextPage. Less RAM usage, should work with all Arduino boards.
    U8x8 Text Only Example: No RAM usage, direct communication with display controller. No graphics, 8x8 Text only.
*/

// U8g2 Contructor List (Frame Buffer)
// The complete list is available here: https://github.com/olikraus/u8g2/wiki/u8g2setupcpp
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

#endif



//--------------------------------------------
//#define PixelPin 9            //Neopixel Data Pin  [ESP8266 - GPIO9] (
#define PixelCount 16       //Length of Neopixel Strand

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount);  //default on ESP8266 is to use the D9(GPIO3,RXD0) pin with DMA.

// LED Definitions
const int LED_GREEN = 16; //ESP8266 - GPIO16
const int LED_RED = 0;    //ESP8266 - GPIO5
const int LED_BLUE = 2;   //ESP8266 - GPIO4
int i, j = 0;


// Onboard LED I/O pin on NodeMCU board
const int PIN_LED = 2; // D4 on NodeMCU and WeMos. Controls the onboard LED.


/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[32] = "";
char password[32] = "";
char coinname[32] = "";
char address[36] = "";
float balance = 0.000000000;


// Web server

//std::unique_ptr<ESP8266WebServer> server;
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;


/** Should I connect to WLAN asap? */
//boolean connect;

/** Last time I tried to connect to WLAN */
//long lastConnectTry = 0;

/** Current WLAN status */
//int status = WL_IDLE_STATUS;

// CoinMarketCap's limit is "no more than 10 per minute"
unsigned long api_mtbs = 10000; //mean time between api requests
unsigned long api_due_time = 0;

WiFiClientSecure client;
CoinMarketCapApi api(client);
HTTPClient http;

//  select wich pin will trigger the configuration portal when set to LOW
//  Needs to be connected to a button to use this pin. It must be a momentary connection
//  not connected permanently to ground.
const int TRIGGER_PIN = 13; // D7 on NodeMCU

// Indicates whether ESP has WiFi credentials saved from previous session
bool initialConfig = false;

void setup() {
#ifdef OLED_DISPLAY
  u8g2.begin();
  u8g2.clearBuffer();          // clear the internal memory
  //u8g2.setFont(u8g2_font_ncenB08_tr);  // 8 pixel height
  u8g2.setFont(u8g2_font_ncenB12_tr );  // 12 pixel height

  u8g2.drawStr(0, 12, "Connecting"); // write something to the internal memory
  u8g2.sendBuffer();          // transfer internal memory to the display
#endif
  //watchdog info
  //known bug: first hardware watchdog reset after serial firmware download will freez device. After 1 hardware reset the system will correctly reset after hardware watchdog timeout.

  //https://sigmdel.ca/michel/program/esp8266/arduino/watchdogs_en.html#ESP8266_SW_WDT

  //feeding watchdogs
  //  https://github.com/esp8266/Arduino/pull/2533/files

  //https://community.blynk.cc/t/solved-esp8266-nodemcu-v1-0-and-wdt-resets/7047/11
  //ESP.wdtDisable();
  //ESP.wdtEnable(WDTO_8S);
//  delay(2);    //feed watchdog

  // initialize the on board LED digital pin as an output.
  // This pin is used to indicated AP configuration mode
  pinMode(PIN_LED, OUTPUT);

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  Serial.begin(115200);
  Serial.println("\n Starting");
  
  //configure NeoPixels
  strip.Begin();
  strip.Show(); // Initialize all pixels to 'off'

  WiFi.printDiag(Serial); //Remove this line if you do not want to see WiFi password printed

  //If there is no stored SSID then set flag so we can startup in AP mode to configure the wifi login paramaters. The ESP8266 low level SDK stores the SSID in a reserved part of memory.
  if (WiFi.SSID() == "") {
    Serial.println("We haven't got any access point credentials, so get them now");
    initialConfig = true;
  }

  //there is stored SSID so try to connect to saved WiFi network
  else {
    digitalWrite(PIN_LED, HIGH); // Turn led off as we are not in configuration mode.
    WiFi.mode(WIFI_STA); // Force to station mode because if device was switched off while in access point mode it will start up next time in access point mode.
    unsigned long startedAt = millis();     //time that we began the wifi connection attempt
    Serial.print("After waiting ");
    int connRes = WiFi.waitForConnectResult();
    float waited = (millis() - startedAt);
    Serial.print(waited / 1000);
    Serial.print(" secs in setup() connection result is ");
    Serial.println(connRes);

  }

  //connect push button to the this pin,with internal pullup.
  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("failed to connect, finishing setup anyway");

  } else {
    //if you get here you have connected to the WiFi
    Serial.println("connected...giddyup :)");
    Serial.print("local ip: ");
    Serial.println(WiFi.localIP());

    /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
    server.on("/", handleRoot);
    server.on("/wifi", handleWifi);
    server.on("/wifisave", handleWifiSave);
    server.on("/ticker", handleTicker);
    server.on("/balance", handleBalance);
    //server.on("/update", handleUpdate);
    server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
    server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
    server.onNotFound ( handleNotFound );
    server.begin(); // Web server start
    httpUpdater.setup(&server); //setup web updater
    Serial.println("HTTP server started");

    loadCredentials(); // Load WLAN credentials, ticker from network
#ifdef OLED_DISPLAY
    u8g2.clearBuffer();               // clear the internal memory
    u8g2.drawStr(0, 12, "Connected"); // write to the internal memory
    u8g2.sendBuffer();                // transfer internal memory to the display
#endif

  }

}




void loop() {

  //  ESP.wdtFeed();
  //  delay(2);    //feed watchdog
  //=================================================================
  // is configuration portal requested by push button
  if (digitalRead(TRIGGER_PIN) == LOW) {
    Serial.println("Configuration portal requested by button press");
    digitalWrite(PIN_LED, LOW); // turn the LED on by making the voltage LOW to tell us we are in configuration mode.
    WiFiManager wifiManager;
    //reset settings - for testing (this clears the ssid stored in eeprom by the esp8266 ssd. not sure what else it does.
    wifiManager.resetSettings();
    ESP.restart(); //restart is supposedly better then reset. not really sure all the reasons.
    //    ESP.reset(); // This is a bit crude. For some unknown reason webserver can only be started once per boot up
  }
  //=================================================================


  //=================================================================
  // is configuration portal requested by blank SSID
  if ((initialConfig)) {
    Serial.println("Configuration portal requested by blank SSID");
    digitalWrite(PIN_LED, LOW); // turn the LED on by making the voltage LOW to tell us we are in configuration mode.
    //Local intialization. Once its business is done, there is no need to keep it around

    WiFiManager wifiManager;

    //reset settings - for testing (this clears the ssid stored in eeprom by the esp8266 ssd. not sure what else it does.
    //    wifiManager.resetSettings();


    //sets timeout in seconds until configuration portal gets turned off.
    //If not specified device will remain in configuration mode until
    //switched off via webserver or device is restarted.
    //wifiManager.setConfigPortalTimeout(600);

    //it starts an access point
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.startConfigPortal()) {
      Serial.println("Not connected to WiFi but continuing anyway.");
    } else {
      //if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");
    }
    digitalWrite(PIN_LED, HIGH); // Turn led off as we are not in configuration mode.

    ESP.restart();
    //   ESP.reset(); // This is a bit crude. For some unknown reason webserver can only be started once per boot up
    // so resetting the device allows to go back into config mode again when it reboots.
    //    delay(5000);

    //https://github.com/esp8266/Arduino/issues/1722
    //ESP.reset() is a hard reset and can leave some of the registers in the old state which can lead to problems, its more or less like the reset button on the PC.
    //ESP.restart() tells the SDK to reboot, so its a more clean reboot, use this one if possible.
    //the boot mode:(1,7) problem is known and only happens at the first restart after serial flashing.
    //if you do one manual reboot by power or RST pin all will work more info see: #1017

    //https://github.com/tzapu/WiFiManager/issues/86
    //ESP8266WebServer server(80);      https://github.com/esp8266/Arduino/issues/686
    // server.reset(new ESP8266WebServer(WiFi.localIP(), 80));
    //server.close
    //server.reset;
  }
  //=================================================================



  server.handleClient();


  unsigned long timeNow = millis();
  if ((timeNow > api_due_time))  {
    printTickerData(coinname);

    api_due_time = timeNow + api_mtbs;




    //Blockchain.info http GET
    String url;
    url = "http://blockchain.info/q/addressbalance/";
    url += address;
    Serial.println(url);
    // http.begin( "http://blockchain.info/q/getdifficulty");
    http.begin(url);
    int httpCode = http.GET();
    delay(2);    //feed watchdog
    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      //USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        balance = ((payload.toFloat()) / 100000000); //satoshi to BTC
        if (balance > 0) {
          Serial.println(balance, 8);
        }
        else {
          Serial.println("no address entered");
        }
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
  }
}

void printTickerData(String ticker) {
  Serial.println("---------------------------------");
  Serial.println("Getting ticker data for " + ticker);

  CMCTickerResponse response = api.GetTickerInfo(ticker);
  if (response.error == "") {
    Serial.print("Name: ");
    Serial.println(response.name);
    Serial.print("Symbol: ");
    Serial.println(response.symbol);
    Serial.print("Price in USD: ");
    Serial.println(response.price_usd);

#ifdef OLED_DISPLAY

    //u8g2.begin();
    //loadCredentials();
    u8g2.clearBuffer();          // clear the internal memory, requires sendBuffer
 //   u8g2.sendBuffer();          // transfer internal memory to the display

    //     u8g2.drawStr(0,30,"hello");  // write something to the internal memory
    //u8g2.clearBuffer();

    //u8g2.setFont(u8g2_font_ncenB12_tr );  // 12 pixel height
    u8g2.setFont(u8g2_font_ncenB10_tr );  // 11 pixel height

    u8g2.setCursor(0, 12);
    u8g2.print(ticker);

    u8g2.setCursor(65, 12);
    u8g2.print(response.price_usd);


    u8g2.setFont(u8g2_font_ncenB10_tr );  // 11 pixel height
    u8g2.setCursor(0, 30);
    u8g2.print("1hr");

    u8g2.setCursor(0, 45);
    u8g2.print(response.percent_change_1h);

    u8g2.setCursor(47, 30);
    u8g2.print("24hr");

    u8g2.setCursor(47, 45);
    u8g2.print(response.percent_change_24h);


    u8g2.setCursor(100, 30);
    u8g2.print("7d");

    u8g2.setCursor(90, 45);
    u8g2.print(response.percent_change_7d);



    u8g2.setCursor(0, 60);
    u8g2.print("balance: ");
    //   u8g2.setCursor(0, 60);
    //  u8g2.print(balance,3);    //display bitcoin balance in address
    u8g2.print(balance * response.price_usd, 3); //display USD value in bitcoin address
    u8g2.sendBuffer();          // transfer internal memory to the display

    //Serial.print("updating display!!!!!!!!!!!!!!!!!!!!!");
    //  delay(250);
 //   ESP.wdtFeed();


#endif
    Serial.print("Percent Change 24h: ");
    Serial.println(response.percent_change_24h);
    Serial.print("Last Updated: ");
    Serial.println(response.last_updated);

  } else {
    Serial.print("Error getting data: ");
    Serial.println(response.error);
  }
  Serial.println("---------------------------------");




  if ((response.percent_change_24h) > 0) {
    digitalWrite(LED_GREEN, LOW); //Green active low
    digitalWrite(LED_RED, HIGH);
    //strip.clear();
    strip.ClearTo(RgbColor(0, 0, 0));

    if ((response.percent_change_24h) < 5) {
      strip.SetPixelColor(3, RgbColor (0, 255, 0));
      strip.Show();
    }
    else if (((response.percent_change_24h) > 5) && (response.percent_change_24h) < 10) {
      strip.SetPixelColor(3, RgbColor (0, 255, 0));
      strip.SetPixelColor(4, RgbColor (0, 255, 0));
      strip.Show();
    }
    else if ((response.percent_change_24h) > 10) {
      strip.SetPixelColor(3, RgbColor(0, 255, 0));
      strip.SetPixelColor(4, RgbColor(0, 255, 0));
      strip.SetPixelColor(5, RgbColor(0, 255, 0));
      strip.Show();
    }
  }



  else if ((response.percent_change_24h) < 0) {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
   // strip.clear();
    strip.ClearTo(RgbColor(0, 0, 0));
    if ((response.percent_change_24h) > -5) {
      strip.SetPixelColor(2, RgbColor(255, 0, 0));
      strip.Show();
    }
    else if (((response.percent_change_24h) < -5) && (response.percent_change_24h) > -10) {
      strip.SetPixelColor(2, RgbColor(255, 0, 0));
      strip.SetPixelColor(1, RgbColor(255, 0, 0));
      strip.Show();
    }
    else if ((response.percent_change_24h) < -10) {
      strip.SetPixelColor(2, RgbColor(255, 0, 0));
      strip.SetPixelColor(1, RgbColor(255, 0, 0));
      strip.SetPixelColor(0, RgbColor(255, 0, 0));
      strip.Show();
    }
  }


}


//// Fill the dots one after the other with a color
//void colorWipe(uint32_t c, uint8_t wait) {
//  //for(uint16_t i=0; i<strip.numPixels(); i++) {
//  Serial.print(i);
//  strip.SetPixelColor(i, c);
//  strip.Show();
//  delay(wait);
//  i++;
//  if (i > strip.numPixels()) {
//    strip.Clear();
//    i = 0;
//  }
//}

//// Input a value 0 to 255 to get a color value.
//// The colours are a transition r - g - b - back to r.
//uint32_t Wheel(byte WheelPos) {
//  WheelPos = 255 - WheelPos;
//  if (WheelPos < 85) {
//    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
//  }
//  if (WheelPos < 170) {
//    WheelPos -= 85;
//    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
//  }
//  WheelPos -= 170;
//  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
//}

