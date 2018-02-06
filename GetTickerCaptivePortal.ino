#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <DNSServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>

//------- Install From Library Manager -------
#include <ArduinoJson.h>
#include <CoinMarketCapApi.h>
#include <Adafruit_NeoPixel.h>
//--------------------------------------------

#define PIN 9            //Neopixel Data Pin  [ESP8266 - GPIO9]
#define NUM_LEDS 6       //Length of Neopixel Strand

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, PIN, NEO_GRB + NEO_KHZ800);

// LED Definitions
const int LED_GREEN = 16; //ESP8266 - GPIO16 
const int LED_RED = 5;    //ESP8266 - GPIO5 
const int LED_BLUE = 4;   //ESP8266 - GPIO4
int i,j=0;

/* Set these to your desired softAP credentials. They are not configurable at runtime */
const char *softAP_ssid = "ESP8266";
const char *softAP_password = "12345678";

/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */
const char *myHostname = "esp8266";

/* Don't set this wifi credentials. They are configurated at runtime and stored on EEPROM */
char ssid[32] = "";
char password[32] = "";
char coinname[32] = "";
char address[36] = "";
float balance = 0.000000000;


// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;

// Web server
ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

/* Soft AP network parameters */
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

/** Should I connect to WLAN asap? */
boolean connect;

/** Last time I tried to connect to WLAN */
long lastConnectTry = 0;

/** Current WLAN status */
int status = WL_IDLE_STATUS;

// CoinMarketCap's limit is "no more than 10 per minute"
unsigned long api_mtbs = 60000; //mean time between api requests
unsigned long api_due_time = 0;

WiFiClientSecure client;
CoinMarketCapApi api(client);
HTTPClient http;

void setup() {
  delay(1000);
  
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid, softAP_password);     /* You can remove the password parameter if you want the AP to be open. */
  //WiFi.softAP(softAP_ssid);
  delay(500); // Without delay I've seen the IP address blank
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */  
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

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
  loadCredentials(); // Load WLAN credentials from network
  connect = strlen(ssid) > 0; // Request WLAN connect if there is a SSID
}

void connectWifi() {
  Serial.println("Connecting as wifi client...");
  WiFi.disconnect();
  WiFi.begin ( ssid, password );
  int connRes = WiFi.waitForConnectResult();
  Serial.print ( "connRes: " );
  Serial.println ( connRes );
   /*switch (connRes){
        case 0:
          Serial.println (connRes" - Idle Status");
          break;
        case 1:
          Serial.println (connRes" - Incorrect SSID");
          break;
        case 2:
          Serial.println (connRes" - WiFi Scan Completed");
          break;
        case 3:
          Serial.println (connRes" - Connected");
          break;
        case 4:
          Serial.println (connRes" - Connect Failed");
          break;
        case 5:
          Serial.println (connRes" - Connection Lost");
          break;
        case 6:
          Serial.println (connRes" - Disconnected");
          break;
          }     */
}

void loop() {
  if (connect) {
    Serial.println ( "Connect requested" );
    connect = false;
    connectWifi();
    lastConnectTry = millis();
  }
  {
    int s = WiFi.status(); 
    if (s == 0 && millis() > (lastConnectTry + 60000) ) {
      /* If WLAN disconnected and idle try to connect */
      /* Don't set retry time too low as retry interfere the softAP operation */
      connect = true;
    }
    if (status != s) { // WLAN status change
      Serial.print ( "Status: " );
      Serial.print ( s );
      status = s;
 
      if (s == WL_CONNECTED) {
        /* Just connected to WLAN */
        Serial.println ( "" );
        Serial.print ( "Connected to " );
        Serial.println ( ssid );
        Serial.print ( "IP address: " );
        Serial.println ( WiFi.localIP() );

        // Setup MDNS responder
        if (!MDNS.begin(myHostname)) {
          Serial.println("Error setting up MDNS responder!");
        } else {
          Serial.println("mDNS responder started");
          // Add service to MDNS-SD
          MDNS.addService("http", "tcp", 80);
        }
      } else if (s == WL_NO_SSID_AVAIL) {
        WiFi.disconnect();
      }
    }
  }
    
  // Do work:
  //DNS
  dnsServer.processNextRequest();
  //HTTP
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

   // httpCode will be negative on error
   if(httpCode > 0) {
    // HTTP header has been send and Server response header has been handled
     //USE_SERIAL.printf("[HTTP] GET... code: %d\n", httpCode);    
         if(httpCode == HTTP_CODE_OK) {
                String payload = http.getString();
                balance=((payload.toFloat())/100000000);  //satoshi to BTC
                if (balance >0) {
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
      strip.clear();
      if ((response.percent_change_24h) < 5) {
        strip.setPixelColor(3, strip.Color(0, 255, 0));
        strip.show(); 
      }
      else if (((response.percent_change_24h) > 5) && (response.percent_change_24h) < 10) {
        strip.setPixelColor(3, strip.Color(0, 255, 0));
        strip.setPixelColor(4, strip.Color(0, 255, 0));
        strip.show();
      }
      else if ((response.percent_change_24h) > 10) {
        strip.setPixelColor(3, strip.Color(0, 255, 0));
        strip.setPixelColor(4, strip.Color(0, 255, 0));
        strip.setPixelColor(5, strip.Color(0, 255, 0));
        strip.show();
      }  
    }
   else if((response.percent_change_24h) < 0) {
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, LOW); 
      strip.clear();
    if ((response.percent_change_24h) > -5) { 
        strip.setPixelColor(2, strip.Color(255, 0, 0));
        strip.show(); 
     }
     else if (((response.percent_change_24h) < -5) && (response.percent_change_24h) > -10) {
        strip.setPixelColor(2, strip.Color(255, 0, 0));
        strip.setPixelColor(1, strip.Color(255, 0, 0));
        strip.show();
     }
     else if ((response.percent_change_24h) < -10) {
        strip.setPixelColor(2, strip.Color(255, 0, 0));
        strip.setPixelColor(1, strip.Color(255, 0, 0));
        strip.setPixelColor(0, strip.Color(255, 0, 0));
        strip.show();
     }
   }

 
}
// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  //for(uint16_t i=0; i<strip.numPixels(); i++) {
    Serial.print(i);
    strip.setPixelColor(i, c);
    strip.show(); 
    delay(wait); 
    i++;
    if (i>strip.numPixels()) {
      strip.clear();
      i=0;
    } 
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

