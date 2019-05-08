void init_wifi()
{
  if ( WiFi.status() != WL_CONNECTED) {
    while (WiFi.begin(ssid, wifi_pass) != WL_CONNECTED) {
      delay(1000);
    }
  }
}
void setup_wifi() 
{

  delay(10);

  if ( WiFi.status() != WL_CONNECTED) {
    int numAttempts = 0;
    while (WiFi.begin(ssid, wifi_pass) != WL_CONNECTED) {
      numAttempts++;
      if(numAttempts == 10){
        Serial.println("Couldn't reach WiFi!");
        break;
      }
      delay(1000);
    }
  }
}
