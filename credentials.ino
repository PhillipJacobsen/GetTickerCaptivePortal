/** Load WLAN credentials from EEPROM */
// note: ssid and password are no longer saved in this section of eeprom. The low level SDK handles storage in the reserved section of FLASH.
// On the ESP8266 there is no EEPROM. FLASH memory is used instead. 
//You need to call EEPROM.begin(size) before you start reading or writing, size being the number of bytes you want to use. Size can be anywhere between 4 and 4096 bytes.
//EEPROM.write does not write to flash immediately, instead you must call EEPROM.commit() whenever you wish to save changes to flash. EEPROM.end() will also commit, and will release the RAM copy of EEPROM contents.
void loadCredentials() {
  EEPROM.begin(512);
  EEPROM.get(0, ssid);    
  EEPROM.get(0+sizeof(ssid), password);
  EEPROM.get(0+sizeof(ssid)+sizeof(password), coinname);
  char ok[2+1];
  //EEPROM.get(0+sizeof(ssid)+sizeof(password), ok);
  EEPROM.get(0+sizeof(ssid)+sizeof(password)+sizeof(coinname), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
  }
  Serial.println("Recovered credentials:");
  //Serial.println(ssid);
  Serial.println(strlen(ssid)>0?ssid:"<no ssid>");
  Serial.println(strlen(password)>0?"********":"<no password>");
  Serial.println(strlen(coinname)>0?coinname:"<no coinname>");
}

/** Store WLAN credentials to EEPROM */
void saveCredentials() {
  EEPROM.begin(512);
  EEPROM.put(0, ssid);
  EEPROM.put(0+sizeof(ssid), password);
  EEPROM.put(0+sizeof(ssid)+sizeof(password), coinname);
  char ok[2+1] = "OK";
  //EEPROM.put(0+sizeof(ssid)+sizeof(password), ok);
  EEPROM.put(0+sizeof(ssid)+sizeof(password)+sizeof(coinname), ok);
  EEPROM.commit();
  EEPROM.end();
}
