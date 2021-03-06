/** Handle root or redirect to captive portal */
void handleRoot() {
  //  if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
  //    return;
  //  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    "<html><head></head><body>"
    "<h1>Welcome to Crypto Monitor!!</h1>"
  );
  //  if (server.client().localIP() == apIP) {
  //    server.sendContent(String("<p>You are connected through the soft AP: ") + softAP_ssid + "</p>");
  //  } else {
  server.sendContent(String("<p>You are connected through the wifi network: ") + ssid + "</p>");
  //  }
  server.sendContent(
    "<p><a href='/wifi'>Configure WIFI Connection</a></p>"
    "</body></html>"
    "<p><a href='/ticker'>Configure Ticker Settings</a></p>"
    "</body></html>"
    "<p><a href='/balance'>Configure Address Monitoring</a></p>"
    "</body></html>"
    "<p><a href='/update'>Firmware Updater</a></p>"
    "</body></html>"
  );
  server.sendContent(""); // *** END 1/2 ***         //https://github.com/esp8266/Arduino/issues/3375    http://www.esp8266.com/viewtopic.php?p=66825
  server.client().stop(); // Stop is needed because we sent no content length
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
//boolean captivePortal() {
//  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname)+".local")) {
//    Serial.print("Request redirected to captive portal");
//    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
//    server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
//    server.client().stop(); // Stop is needed because we sent no content length
//    return true;
//  }
//  return false;
//}

/** Wifi config page handler */
void handleWifi() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    "<html><head></head><body>"
    "<h1>WiFi Config</h1>"
  );
  //  if (server.client().localIP() == apIP) {
  //    server.sendContent(String("<p>You are connected through the soft AP: ") + softAP_ssid + "</p>");
  //  } else {
  server.sendContent(String("<p>You are connected through the wifi network: ") + ssid + "</p>");
  //  }
  server.sendContent(
    "\r\n<br />"
    "<table><tr><th align='left'>SoftAP config</th></tr>"
  );

  //server.sendContent(String() + "<tr><td>SSID " + String(softAP_ssid) + "</td></tr>");
  // server.sendContent(String() + "<tr><td>IP " + toStringIp(WiFi.softAPIP()) + "</td></tr>");


  server.sendContent(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN config</th></tr>"
  );
  server.sendContent(String() + "<tr><td>SSID " + String(ssid) + "</td></tr>");
  server.sendContent(String() + "<tr><td>IP " + toStringIp(WiFi.localIP()) + "</td></tr>");
  server.sendContent(
    "</table>"
    "\r\n<br />"
    "<table><tr><th align='left'>WLAN list (refresh if any missing)</th></tr>"
  );
  Serial.println("scan start");
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n > 0) {
    for (int i = 0; i < n; i++) {
      server.sendContent(String() + "\r\n<tr><td>SSID " + WiFi.SSID(i) + String((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : " *") + " (" + WiFi.RSSI(i) + ")</td></tr>");
    }
  } else {
    server.sendContent(String() + "<tr><td>No WLAN found</td></tr>");
  }
  server.sendContent(
    "</table>"
    "\r\n<br /><form method='POST' action='wifisave'><h4>Connect to network:</h4>"
    "<input type='text' placeholder='network' name='n'/>"
    "<br /><input type='password' placeholder='password' name='p'/>"
    "<br /><input type='submit' value='Connect/Disconnect'/></form>"
    "<p><a href='/'>BACK</a></p>"
    "</body></html>"
  );
  server.sendContent(""); // *** END 1/2 ***         //https://github.com/esp8266/Arduino/issues/3375    http://www.esp8266.com/viewtopic.php?p=66825
  server.client().stop(); // Stop is needed because we sent no content length
}

/** Ticker config page handler */
void handleTicker() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    "<html><head></head><body>"
    "<h1>Ticker Config</h1>"
  );
  server.sendContent(String("<p>You are currently monitoring ") + coinname + "</p>");
  server.sendContent(
    "</table>"
    "\r\n<br /><form method='POST' action='wifisave'><h4>Enter New Coin to Monitor:</h4>"
    "<input type='text' placeholder='coin name' name='t'/>"
    "<br /><input type='submit' value='Submit'/></form>"
    "<p> eg. bitcoin, ethereum, litecoin</p>"
    "<p><a href='/'>BACK</a></p>"
    "</body></html>"
  );
  server.sendContent(""); // *** END 1/2 ***         //https://github.com/esp8266/Arduino/issues/3375    http://www.esp8266.com/viewtopic.php?p=66825
  server.client().stop(); // Stop is needed because we sent no content length
}

/** Balance config page handler */
void handleBalance() {
  String CurrentBalance = String(balance, 8);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent(
    "<html><head></head><body>"
    "<h1>Address Monitoring</h1>"
  );
  server.sendContent(String("<p>You are currently monitoring bitcoin address: ") + address + "</p>");
  server.sendContent(String("<p>Current Balance: ") + CurrentBalance + " BTC</p>");
  server.sendContent(
    "</table>"
    "\r\n<br /><form method='POST' action='wifisave'><h4>Enter new address to monitor:</h4>"
    "<input type='text' placeholder='address' name='b'/>"
    "<br /><input type='submit' value='Submit'/></form>"
    "<p><a href='/'>BACK</a></p>"
    "</body></html>"
  );
  server.sendContent(""); // *** END 1/2 ***         //https://github.com/esp8266/Arduino/issues/3375    http://www.esp8266.com/viewtopic.php?p=66825
  server.client().stop(); // Stop is needed because we sent no content length

}

/** Web Update page handler */
void handleUpdate() {

}

/*void handleUpdate() {
    server.on("/", HTTP_GET, [](){
      server.sendHeader("Connection", "close");
      server.send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  //   "<html><head></head><body>"
  //   "<h1>Firmware Updater</h1>"
    });

      server.on("/update", HTTP_POST, [](){
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
      ESP.restart();
    },[](){
      HTTPUpload& upload = server.upload();
      if(upload.status == UPLOAD_FILE_START){
        Serial.setDebugOutput(true);
        WiFiUDP::stopAll();
        Serial.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if(!Update.begin(maxSketchSpace)){//start with max available size
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_WRITE){
        if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_END){
        if(Update.end(true)){ //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
      yield();
    });
     server.begin();
  }
*/
/** Handle the WLAN save form and redirect to WLAN config page again */
void handleWifiSave() {

  if (server.arg("n") != 0) {
    Serial.println("wifi save");
    server.arg("n").toCharArray(ssid, sizeof(ssid) - 1);
    server.arg("p").toCharArray(password, sizeof(password) - 1);

    server.sendHeader("Location", "wifi", true);
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.send ( 302, "text/plain", "");  // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.sendContent(""); // *** END 1/2 ***         //https://github.com/esp8266/Arduino/issues/3375    http://www.esp8266.com/viewtopic.php?p=66825
    server.client().stop(); // Stop is needed because we sent no content length
    saveCredentials();
    //   connect = strlen(ssid) > 0; // Request WLAN connect with new credentials if there is a SSID
  }

  if (server.arg("t") != 0) {
    Serial.println("ticker save");
    server.arg("t").toCharArray(coinname, sizeof(coinname) - 1);

    server.sendHeader("Location", "ticker", true);
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.send ( 302, "text/plain", "");  // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.sendContent(""); // *** END 1/2 ***         //https://github.com/esp8266/Arduino/issues/3375    http://www.esp8266.com/viewtopic.php?p=66825 
    server.client().stop(); // Stop is needed because we sent no content length
    saveCredentials();
  }
  if (server.arg("b") != 0) {
    server.arg("b").toCharArray(address, sizeof(address) - 1);
    //Serial.println("address save: ");
    //Serial.println(address);
    server.sendHeader("Location", "balance", true);
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.send ( 302, "text/plain", "");  // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.sendContent(""); // *** END 1/2 ***         //https://github.com/esp8266/Arduino/issues/3375    http://www.esp8266.com/viewtopic.php?p=66825 
    server.client().stop(); // Stop is needed because we sent no content length
    //saveCredentials();
  }
}

void handleNotFound() {
  //  if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
  //    return;
  //  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 404, "text/plain", message );
}

