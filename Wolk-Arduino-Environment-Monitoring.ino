#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <bme680_defs.h>
#include <bme680.h>

#include <WiFi101.h>
#include <RTCZero.h>

#include "WolkConn.h"
#include "MQTTClient.h"
/*Number of outbound_message_t to store*/
#define STORAGE_SIZE 32
#define SEALEVELPRESSURE_HPA (1013.25)

/*Connection details*/
const char* ssid = "OpenWrt";
const char* wifi_pass = "basdobrasifra123";

const char *device_key = "xawa5fryg6y5denm";
const char *device_password = "f6a9549b-1717-4f4d-bb8b-2b49c55de165";
const char* hostname = "api-demo.wolkabout.com";
int portno = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

/* WolkConnect-Arduino Connector context */
static wolk_ctx_t wolk;

/*Init i2c sensor communication*/
Adafruit_BME680 bme;

RTCZero rtc;

bool read;
/*Read sensor every minute. If you change this parameter
make sure that it's <60*/
const byte readEvery = 1;

bool publish;
/*Publish every 10 minutes. If you change this parameter
make sure that it's <60*/
const byte publishEvery = 10;
byte publishMin;

void setup() {
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  /*Initialize the circular buffer structure*/
  init_flash();

  init_bme();
  
  init_wifi();

  wolk_init(&wolk, NULL, NULL, NULL, NULL,
            device_key, device_password, &client, hostname, portno, PROTOCOL_JSON_SINGLE, NULL, NULL);

  wolk_init_custom_persistence(&wolk, _push, _peek, _pop, _is_empty);

  delay(200);

  read = true;
  publish = true;

  /*Request epoch time from platform*/
  wolk_connect(&wolk);
  delay(100);

  /*This function requests and waits for
   * the epoch time from the platform.
   * If a problem occurred, the on-board LED will be on.
  */
  wolk_update_epoch(&wolk);
  
  rtc.begin();

  rtc.setEpoch(wolk.epoch_time);

  rtc.setAlarmTime(rtc.getHours(), (rtc.getMinutes() + readEvery) % 60, rtc.getSeconds());
  rtc.enableAlarm(rtc.MATCH_MMSS);

  rtc.attachInterrupt(alarmMatch);
  publishMin = (rtc.getMinutes() + publishEvery) % 60;
  
  WiFi.lowPowerMode();

}

void loop() {
  /*In order to keep the interrupt routine as short as possible
  routine only sets the tasks to be done
  read = true means that the sensor reading should be done
  publish = true means that the readings should be published to platform
  or persisted in flash if the connection is not available
  */
  if(read)
  {
    read = false;
    
    if (!bme.performReading()) {
      Serial.println("Couldn't read data!");
    }

    Serial.print("Temperature:");
    Serial.println(bme.temperature);
    Serial.print("Humidity:");
    Serial.println(bme.humidity);
    Serial.print("Pressure:");
    Serial.println(bme.pressure);
    
    wolk_add_numeric_sensor_reading(&wolk, "T", bme.temperature, rtc.getEpoch());
    wolk_add_numeric_sensor_reading(&wolk, "H", bme.humidity, rtc.getEpoch());
    wolk_add_numeric_sensor_reading(&wolk, "P", bme.pressure / 100.0, rtc.getEpoch());
    wolk_add_numeric_sensor_reading(&wolk, "GR", bme.gas_resistance, rtc.getEpoch());
    wolk_add_numeric_sensor_reading(&wolk, "A", bme.readAltitude(SEALEVELPRESSURE_HPA), rtc.getEpoch());
    
    /*set new alarm*/
    int alarmMin = (rtc.getMinutes() + readEvery) % 60;
    rtc.setAlarmMinutes(alarmMin);
    delay(100);
  }
  
  if(publish)
  {
    publish = false;
    setup_wifi();
    wolk_connect(&wolk);
    if(!wolk.is_connected)
    {
      _flash_store();
    }
    delay(100);
    if(wolk_publish(&wolk) == W_TRUE)
    {
      _flash_store();
    }
    /*set new publish time*/
    publishMin = (rtc.getMinutes() + publishEvery) % 60;
    delay(100);
    wolk_disconnect(&wolk);
    delay(100);
  }
  delay(100);
  
}
/*Timed interrupt routine*/
void alarmMatch()
{
  read = true;
  if(publishMin == rtc.getMinutes())
  {
    publish = true;
  }
}
