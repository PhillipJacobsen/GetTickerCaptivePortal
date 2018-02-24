/********************************************************************************
    Crypto Monitor
    GetTickerCaptivePortal.ino
    2018 @dantasticdan @phillipjacobsen

********************************************************************************/

/********************************************************************************
               Electronic Hardware Requirements and Pin Connections

    ESP8266 NodeMCU1.0 ESP-12E development module
      Source:  https://www.amazon.ca/gp/product/B06VV39XD8

      Notes from NODEMCU_DEVKIT_V1.0.PDF
          On every boot/reset/wakeup, GPIO15 MUST keep LOW, GPIO2 MUST keep HIGH.
          GPIO0 HIGH ->RUN MODE, LOW -> FLASH MODE.
          When you need to use the sleep mode,GPIO16 and RST should be connected,
          and GPIO16 will output LOW to reset the system at the time of wakeup.

       Watchdog info Note!!!!!
        known bug: first hardware watchdog reset after serial firmware download will freeze device. After 1 hardware reset the system will correctly reset after hardware watchdog timeout.
          https://sigmdel.ca/michel/program/esp8266/arduino/watchdogs_en.html#ESP8266_SW_WDT
        feeding watchdogs
          https://github.com/esp8266/Arduino/pull/2533/files
          https://community.blynk.cc/t/solved-esp8266-nodemcu-v1-0-and-wdt-resets/7047/11
            //ESP.wdtDisable();
            //ESP.wdtEnable(WDTO_8S);
            //  delay(2);    //feed watchdog( I think....)

      ESP8266 GPIO pin Usage
          D0/GPIO16 -> Do not use for now. used for sleep mode??? Also connected to Onboard Blue LED(near USB connector). I think we could use this LED if R10 was removed
          D1/GPIO5  -> OLED SCL
          D2/GPIO4  -> OLED SDA
          D3/GPIO0  -> Do not use for now. Connected to onboard Flash Push Button. Not sure what this does(Maybe used by LUA interpreter)
          D4/GPIO2  -> Keep low on bootup - also connected to Blue LED on WiFiModule(Near antenna)
          D5/GPIO14 -> Green LED
          D6/GPIO12 -> Red LED
          D7/GPIO13 -> Push Button
          D8/GPIO15 -> Do not use for now.  Keep low on bootup.
          D9/GPIO3  -> NeoPixel Din. Also used for USB serial Rx. I
          D10/GPIO1 -> used for USB serial Tx.



    0.96 I2C 128x64 OLED display
      Source:    https://www.amazon.ca/gp/product/B01N78FUH7
      Pins cannot be remapped when using DMA mode
      SDA ->NodeMCU D2 pin (GPIO4)
      SCL ->NodeMCU D1 pin (GPIO5)
      VCC -> 3.3V
      GND

    NeoPixels
      Note: NeoPixels should really be powered by 5V or 3.7V at a minimum.
            When powered by 5V a 3.3V->5V level shifter should be used
            When powered by 3.7V the 3.3V i/O from ESP8266 is ok

      Din -> NodeMCU D9 pin (RDX0/GPIO3)
      VCC -> 3.3V (This is "working". should be 3.7V or 5V with level shifter on Din pin)
      GND -> GND

    Pushbutton
      NodeMCU D6 pin (GPIO12) (with internal pullup configured)
      GND

    Onboard Blue LED on NodeMCU dev board (near antenna)
      ->NodeMCU D5 pin (GPIO2)

    Green External LED
      D5 pin (GPIO14)

    Red External LED
      D6 pin (GPIO12)

 ********************************************************************************/



/********************************************************************************
                              Library Requirements
********************************************************************************/

/********************************************************************************
    ESP8266WiFi (ESP8266 Arduino Core) V2.4
      ARDUINO Board Manager URL:
        http://arduino.esp8266.com/stable/package_esp8266com_index.json
        https://github.com/esp8266/Arduino
********************************************************************************/
#include <ESP8266WiFi.h>
//#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
//#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
//#include <ESP8266mDNS.h>


/********************************************************************************
   ESP8266 WiFi Connection manager
     Available through Arduino Library Manager however development is done using lastest Master Branch on Github
     https://github.com/tzapu/WiFiManager
********************************************************************************/
#include "WiFiManager.h"


/********************************************************************************
    U8g2lib Monochrome Graphics Display Library
      Available through Arduino Library Manager
      https://github.com/olikraus/u8g2

    ESP8266 module ->use hardware I2C connections
      SDA ->NodeMCU D2 pin (GPIO4)
      SCL ->NodeMCU D1 pin (GPIO5)

    UNO connections -> use hardware I2C connections
      SDA ->A4
      SCL ->A5
    Frame Buffer Examples: clearBuffer/sendBuffer. Fast, but may not work with all Arduino boards because of RAM consumption
    Page Buffer Examples: firstPage/nextPage. Less RAM usage, should work with all Arduino boards.
    U8x8 Text Only Example: No RAM usage, direct communication with display controller. No graphics, 8x8 Text only.
********************************************************************************/
#define OLED_DISPLAY

#ifdef OLED_DISPLAY
#include <U8g2lib.h>

// U8g2 Contructor List for Frame Buffer Mode.
// This uses the Hardware I2C peripheral on ESP8266 with DMA interface
// The complete list is available here: https://github.com/olikraus/u8g2/wiki/u8g2setupcpp
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

#endif


/********************************************************************************
    Makuna NeoPixel Library - optimized for ESP8266
      Available through Arduino Library Manager however development is done using lastest Master Branch on Github
      https://github.com/Makuna/NeoPixelBus/

      This library is optimized to use the DMA on the ESP8266 for minimal cup usage. The standard Adafruit library has the potential to interfere with the
      WiFi processing done by the low level SDK
      NeoPixelBus<FEATURE, METHOD> strip(pixelCount, pixelPin);
       NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(16);
      On the ESP8266 the Neo800KbpsMethod method will use this underlying method: NeoEsp8266Dma800KbpsMethod
      The NeoEsp8266Dma800KbpsMethod is the underlying method that gets used if you use Neo800KbpsMethod on Esp8266 platforms. There should be no need to use it directly.
      The NeoEsp8266Dma800KbpsMethod only supports the RDX0/GPIO3 pin. The Pin argument is omitted. See other esp8266 methods below if you don't have this pin available.
      This method uses very little CPU for actually sending the data to NeoPixels but it requires an extra buffer for the DMA to read from.
      Thus there is a trade off of CPU use versus memory use. The extra buffer needed is four times the size of the primary pixel buffer.
       It also requires the use of the RDX0/GPIO3 pin. The normal feature of this pin is the "Serial" receive.
      Using this DMA method will not allow you to receive serial from the primary Serial object; but it will not stop you from sending output to the terminal program of a PC
      Due to the pin overlap, there are a few things to take into consideration.
      First, when you are flashing the Esp8266, some LED types will react to the flashing and turn on.
      This is important if you have longer strips of pixels where the power use of full bright might exceed your design.
      Second, the NeoPixelBus::Begin() MUST be called after the Serial.begin().
      If they are called out of order, no pixel data will be sent as the Serial reconfigured the RDX0/GPIO3 pin to its needs.
********************************************************************************/
#include <NeoPixelBus.h>

/********************************************************************************
    CoinMarketCap
   Install From Library Manager
********************************************************************************/
#include <CoinMarketCapApi.h>

/********************************************************************************
    Various other libraries
      Install From Library Manager
********************************************************************************/
#include <ArduinoJson.h>
#include <EEPROM.h>


//--------------------------------------------
//  NeoPixels
//#define PixelPin 9        //Neopixel Data Pin  [ESP8266 - GPIO9] - cannot map DMA interface to any other pin.
#define PixelCount 16       //Length of Neopixel Strand
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount);  //default on ESP8266 is to use the D9(GPIO3,RXD0) pin with DMA.


//--------------------------------------------
//  LED Definitions
const int LED_GREEN = 14;  //  D5/GPIO14
const int LED_RED = 12;    //  D6/GPIO12
const int LED_MODULE = 2; // Controls the onboard blue LED(Near antenna). Bootloader seems to toggle this and also toggles during programming

//--------------------------------------------
//  Push Button
//  select wich pin will trigger the configuration portal when set to LOW
//  Push Button needs to be connected to this pin. It must be a momentary connection
//  not connected permanently to ground.
const int TRIGGER_PIN = 13; // D7/GPIO13


//--------------------------------------------
//  Web server
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

// Indicates whether ESP has WiFi credentials saved from previous session
// If SSID is available then it will attempt to connect to saved SSID
// If SSID is blank then it will start AP and start Captive Server
bool SSID_Available = false;



int i, j = 0;

/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[32] = "";
char password[32] = "";
char coinname[32] = "";
char address[36] = "";
float balance = 0.000000000;



/** Current WLAN status */
//int status = WL_IDLE_STATUS;



//--------------------------------------------
//  HTTP Client Connections
WiFiClientSecure client;
CoinMarketCapApi api(client);
HTTPClient http;

// CoinMarketCap's limit is "no more than 10 per minute"
unsigned long api_mtbs = 10000; //mean time between api requests
unsigned long api_due_time = 0;






/********************************************************************************
                              Setup
********************************************************************************/
void setup() {

  //--------------------------------------------
  //  Configure LEDs/Buttons
  pinMode(LED_MODULE, OUTPUT);    //On Board Blue LED - Used to indicate Captive Portal Mode(LED On) / Normal Mode(LED Off)
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  pinMode(TRIGGER_PIN, INPUT_PULLUP);  //push button is used to Start Captive Portal


#ifdef OLED_DISPLAY
  u8g2.begin();
  u8g2.clearBuffer();          // clear the internal memory

  //u8g2.setFont(u8g2_font_ncenB08_tr);  // 8 pixel height
  u8g2.setFont(u8g2_font_ncenB12_tr );  // 12 pixel height
  u8g2.drawStr(0, 12, "Connecting"); // write something to the internal memory
  u8g2.sendBuffer();          // transfer internal memory to the display
#endif


  //--------------------------------------------
  //  Configure Serial Port for debug info
  Serial.begin(115200);
  Serial.println("\n Starting");

  //--------------------------------------------
  //  Configure NeoPixels.
  //NOTE! Make sure to call strip.Begin() after you call Serial.Begin because Serial RX pin is also connected to Serial RX pin.
  strip.Begin();
  strip.Show(); // Initialize all pixels to 'off'



  //--------------------------------------------
  //print out WiFi diagnostic info
  WiFi.printDiag(Serial); //Remove this line if you do not want to see WiFi password printed
  Serial.println();

  //--------------------------------------------
  //If there is no stored SSID then set flag so we can startup in AP mode to configure the wifi login paramaters. The ESP8266 low level SDK stores the SSID in a reserved part of memory.
  if (WiFi.SSID() == "") {
    Serial.println("We haven't got any access point credentials, so get them now");
    SSID_Available = false;
    digitalWrite(LED_MODULE, LOW); // Turn led on to indicate we are in AP Configuration mode.

  }

  //--------------------------------------------
  //there is a stored SSID so try to connect to saved WiFi network
  else {
    SSID_Available = true;
    digitalWrite(LED_MODULE, HIGH);       // Turn led off as we are not in AP Configuration mode.
    WiFi.mode(WIFI_STA);                  // Force to station mode because if device was switched off while in access point mode it will start up next time in access point mode.
    unsigned long startedAt = millis();   //time that we began the wifi connection attempt

    Serial.print("looking for WiFi network ");


    //    int connRes = WiFi.waitForConnectResult();     //There seems to be a built in timeout of around 6.3 seconds

    //method #1 to wait for connection
    //    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    //      Serial.println("Connection Failed! Rebooting...");
    //      ESP.restart();
    //      delay(5000);
    //   }

    //method #2 to wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      //it will hang here forever if there is no connection

      //=================================================================
      // Is configuration portal requested by push button?
      // If button is pushed then the SSID is erased and processor is reset so it can startup Captive Portal
      if (digitalRead(TRIGGER_PIN) == LOW) {
        Serial.println("Configuration portal requested by button press");
        digitalWrite(LED_MODULE, LOW); // turn the LED on by making the voltage LOW to tell us we are in configuration mode.

        WiFi.disconnect();    //erases SSID/Password stored in Flash
          ESP.restart(); //restart may be better then reset. not really sure all the reasons.
        //ESP.reset(); // This is a bit crude. For some unknown reason webserver can only be started once per boot up
        delay(1000);   //I don't know why this delay is needed here. Perhaps it has something to do with the RTOS and giving it time to process....
      }
      //=================================================================

    }

    int connRes = WiFi.status();

    Serial.print("After waiting ");
    float waited = (millis() - startedAt);
    Serial.print(waited / 1000);
    Serial.print(" secs in setup() connection result is ");
    Serial.println(connRes);

    //0 : WL_IDLE_STATUS when Wi-Fi is in process of changing between statuses
    //1 : WL_NO_SSID_AVAILin case configured SSID cannot be reached
    //3 : WL_CONNECTED after successful connection is established
    //4 : WL_CONNECT_FAILED if password is incorrect
    //6 : WL_DISCONNECTED if module is not configured in station mode


    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("failed to connect, finishing setup anyway");

    } else {
      //if you get here you have connected to the WiFi
      Serial.println("connected to WiFi...giddyup :)");
      Serial.print("local ip: ");
      Serial.println(WiFi.localIP());

#ifdef OLED_DISPLAY
      u8g2.clearBuffer();                     // clear the internal memory
      u8g2.setFont(u8g2_font_ncenB12_tr );    // 12 pixel height
      u8g2.drawStr(0, 12, "Connected!");      // write to internal memory
      u8g2.drawStr(0, 35, "IP address:");     // write to internal memory
      u8g2.setCursor(0, 60);
      u8g2.print(WiFi.localIP());             // display IP address of webserver
      u8g2.sendBuffer();                      // transfer internal memory to the display
#endif

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
      //#ifdef OLED_DISPLAY
      //      u8g2.clearBuffer();               // clear the internal memory
      //      u8g2.drawStr(0, 12, "Connected"); // write to the internal memory
      //      u8g2.sendBuffer();                // transfer internal memory to the display
      //#endif


      delay(1000);     //delay here for a while to show the IP address on the OLED before advancing to the next screen
    }

  }


}



/********************************************************************************
                              Main Loop
********************************************************************************/
void loop() {

  //=================================================================
  // is configuration portal requested by blank SSID ?
  //
  if ((!SSID_Available)) {
    Serial.println("Captive Portal requested by blank SSID");

    WiFiManager wifiManager;

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
      Serial.println("Connected to new access point... :)");
    }
    digitalWrite(LED_MODULE, HIGH); // Turn led off as we are not in configuration mode.

    ESP.restart();
    //    ESP.reset(); // This is a bit crude. For some unknown reason webserver can only be started once per boot up
    delay(1000);   //I don't know why this delay is needed. Perhaps it has something to do with the RTOS and giving it time to process

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


  //=================================================================
  // Is configuration portal requested by push button?
  // If button is pushed then the SSID is erased and processor is reset so it can startup Captive Portal
  if (digitalRead(TRIGGER_PIN) == LOW) {
    Serial.println("Configuration portal requested by button press");
    digitalWrite(LED_MODULE, LOW); // turn the LED on by making the voltage LOW to tell us we are in configuration mode.

    WiFi.disconnect();    //erases SSID/Password stored in Flash
    delay(1000);
    // So with persistent(true) Wifi.disconnect ERASES the SSID and password
    //With persistent(false) Wifi.disconnect doesn't erase anything.
    //https://github.com/tzapu/WiFiManager/issues/142
    ESP.restart(); //restart may be better then reset. not really sure all the reasons.
    //ESP.reset(); // This is a bit crude. For some unknown reason webserver can only be started once per boot up
    delay(1000);   //I don't know why this delay is needed here. Perhaps it has something to do with the RTOS and giving it time to process....
  }
  //=================================================================


  //=================================================================
  //check for disconnect
  //I am trying to figure out a reconnect loop. This is not working, however I think something is getting stuck in coinmarket cap api.
  //https://github.com/esp8266/Arduino/blob/master/doc/esp8266wifi/station-class.rst#waitforconnectresult

  /*
    if   (WiFi.status() != WL_CONNECTED) {
      Serial.println("Disconnected!!!!!!!");
      WiFi.setAutoReconnect(true);
      while (WiFi.status() != WL_CONNECTED) {
        delay(1);     //give RTOS some bandwidth
        ESP.wdtFeed();
      }
      Serial.println("reconnected!!!!!!!");
    }
  */
  //  ESP.wdtFeed();    //feed watchdog
  //=================================================================



  server.handleClient();

  unsigned long timeNow = millis();

  //=================================================================
  // retrieve coin price from Coinmarketcap API
  if ((timeNow > api_due_time))  {

    //=================================================================
    // retrieve coin price from Coinmarketcap API
    Serial.println("---------------------------------");
    Serial.print("Getting ticker data for ");
    Serial.println(coinname);
    //When the wifi networks goes away I think api.GetTickerInfo(ticker); gets stuck.
    //I keep getting a stack dump regardless of any of the wdt disable code below.
    ESP.wdtFeed();   //resets software and hardware watchdogs
    //  ESP.wdtDisable();      //disable software watchdog. Hardware watchdog is still going. (Maybe set to 6 seconds??)
    delay(2);    //give RTOS some bandwidth

    //this api seems to have a timeout of 1500m????????
    CMCTickerResponse response = api.GetTickerInfo(coinname);

    //  CMCTickerResponse response = api.GetTickerInfo(ticker);
    //  delay(1);    //give RTOS some bandwidth
    ESP.wdtFeed();   //resets software and hardware watchdogs
    //ESP.wdtEnable(1000);    //an integer value needs to be passed as input argument however the SDK may not use this value.
    //  ESP.wdtEnable(8000);
    //=================================================================

    //=================================================================
    // get bitcoin balance from address
    getBitcoinBalance();

    //=================================================================
    // update displays
    printTickerData(coinname, &response);
    updateOLED(coinname, &response);
    updateNeoPixels(&response);

    api_due_time = timeNow + api_mtbs;
  }

}


/********************************************************************************
                              Get Bitcoin Balance from Address
********************************************************************************/
void getBitcoinBalance() {
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


/********************************************************************************
                              Update NeoPixels
********************************************************************************/
void updateNeoPixels(CMCTickerResponse * response) {

  if ((response->percent_change_24h) > 0) {
    digitalWrite(LED_GREEN, LOW); //Green active low
    digitalWrite(LED_RED, HIGH);
    //strip.clear();
    strip.ClearTo(RgbColor(0, 0, 0));

    if ((response->percent_change_24h) < 5) {
      strip.SetPixelColor(3, RgbColor (0, 100, 0));
      strip.Show();
    }
    else if (((response->percent_change_24h) > 5) && (response->percent_change_24h) < 10) {
      strip.SetPixelColor(3, RgbColor (0, 100, 0));
      strip.SetPixelColor(4, RgbColor (0, 100, 0));
      strip.Show();
    }
    else if ((response->percent_change_24h) > 10) {
      strip.SetPixelColor(3, RgbColor(0, 100, 0));
      strip.SetPixelColor(4, RgbColor(0, 100, 0));
      strip.SetPixelColor(5, RgbColor(0, 100, 0));
      strip.Show();
    }
  }


  else if ((response->percent_change_24h) < 0) {
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_RED, LOW);
    // strip.clear();
    strip.ClearTo(RgbColor(0, 0, 0));
    if ((response->percent_change_24h) > -5) {
      strip.SetPixelColor(2, RgbColor(100, 0, 0));
      strip.Show();
    }
    else if (((response->percent_change_24h) < -5) && (response->percent_change_24h) > -10) {
      strip.SetPixelColor(2, RgbColor(100, 0, 0));
      strip.SetPixelColor(1, RgbColor(100, 0, 0));
      strip.Show();
    }
    else if ((response->percent_change_24h) < -10) {
      strip.SetPixelColor(2, RgbColor(100, 0, 0));
      strip.SetPixelColor(1, RgbColor(100, 0, 0));
      strip.SetPixelColor(0, RgbColor(100, 0, 0));
      strip.Show();
    }
  }

}

/********************************************************************************
                              Update 128x64 OLED
********************************************************************************/
void updateOLED(String ticker, CMCTickerResponse * response) {
  if (response->error == "") {
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
    u8g2.print(response->price_usd);


    u8g2.setFont(u8g2_font_ncenB10_tr );  // 11 pixel height
    u8g2.setCursor(0, 30);
    u8g2.print("1hr");

    u8g2.setCursor(0, 45);
    u8g2.print(response->percent_change_1h);

    u8g2.setCursor(47, 30);
    u8g2.print("24hr");

    u8g2.setCursor(47, 45);
    u8g2.print(response->percent_change_24h);

    u8g2.setCursor(100, 30);
    u8g2.print("7d");

    u8g2.setCursor(90, 45);
    u8g2.print(response->percent_change_7d);


    u8g2.setCursor(0, 60);
    u8g2.print("balance: ");
    //   u8g2.setCursor(0, 60);
    //  u8g2.print(balance,3);    //display bitcoin balance in address
    u8g2.print(balance * response->price_usd, 3); //display USD value in bitcoin address
    u8g2.sendBuffer();          // transfer internal memory to the display

    //Serial.print("updating display!!!!!!!!!!!!!!!!!!!!!");
    //  delay(250);
    //   ESP.wdtFeed();

  } else {

  }
}



/********************************************************************************
                              send Ticker data to terminal
********************************************************************************/
void printTickerData(String ticker, CMCTickerResponse * response) {
  if (response->error == "") {
    Serial.print("Name: ");
    Serial.println(response->name);
    Serial.print("Symbol: ");
    Serial.println(response->symbol);
    Serial.print("Price in USD: ");
    Serial.println(response->price_usd);
    Serial.print("Percent Change 24h: ");
    Serial.println(response->percent_change_24h);
    Serial.print("Last Updated: ");
    Serial.println(response->last_updated);

  } else {

    Serial.print("Error getting data: ");
    Serial.println(response->error);
  }

  Serial.println("---------------------------------");

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



void clearStrip() {
  for ( int i = 0; i < PixelCount; i++) {
    strip.SetPixelColor(i, 0x000000); strip.Show();
  }
}



