# Wolk-Arduino-Environment-Monitoring
Measure and visualise Environment click sensor readings using the WolkAbout platform.

##Things used in this project
######Hardware components
- [Arduino MKR1000 WiFi](https://www.hackster.io/arduino/products/arduino-mkr1000)
- [Environment click](https://www.mikroe.com/environment-click)

##Software
- Arduino IDE

##Story

We had some Environment click sensors lying around so we decided to hook them up to the MKR1000 and visualise them on the [Wolkabout platform](https://demo.wolkabout.com/). The examples provided within the libraries we used made it quick to implement.
The idea is to do the measurement every 15 minutes and publish the results once every hour.
If the publish fails (due to a busy network or some other issue) the results should be persisted in Flash memory of the device. With max. 24 writes a day we avoid getting the Flash in a faulty state (Arduino guarantees 10000 write cycles).
In the meantime all devices go into sleep mode in order to minimise the power consumption. 

##Hardware setup

You'll need the Arduino MKR1000 and the Environment click sensor.
The Environment click sensor is hooked up to the I2C communication pins (namely 11 and 12 on the MKR1000), Vcc and ground.

##Dependencies

- [WiFi101 library](https://github.com/arduino-libraries/WiFi101)
- [Adafruit BME680 library](https://github.com/adafruit/Adafruit_BME680)
- [RTCZero library](https://github.com/arduino-libraries/RTCZero)
- [Flash storage library](https://github.com/cmaglie/FlashStorage)
- [WolkConnect library](https://github.com/Wolkabout/WolkConnect-Arduino)
All libraries are available via the Arduino Library Manager.

##You'll also need

An account on the [WolkAbout platform](https://demo.wolkabout.com/) and a device created using the BME680-manifest.json file provided on our [GitHub](https://github.com/Wolkabout/Wolk-Arduino-Environment-Monitoring) 

##How it works

The starting point for this project were the examples provided in the [WolkConnect-Arduino library](https://github.com/Wolkabout/WolkConnect-Arduino).

The flash persistence example provides all the necessary functions for a custom persistence implementation. It implements a structure of a circular buffer and uses it to store the measurement results. There is also an implementation of how to store that structure in Flash memory using the Flash storage library.
The Timed measure example implements timed reading and publishing of the data using RTCZero library. All you need to do is change time variables in order to represent the actual time and set the intervals you wish to use for the read and publish.
The sensor also has a library which makes reading easy.
The alarm interrupt changes flags for tasks that need to be done (read and publish). The rest of the work falls on the loop function.
Here we check if we need to read or publish and go to sleep when it's all done. For minimal power consumtion, WiFi is set to be in a low power mode as well as the MKR1000. The sensors go to low power automaticaly until a reading is requested.

##How to use it

After registering an account on the [WolkAbout platform](https://demo.wolkabout.com/) click on Devices->Device templates.
Then click on the plus sign to add template. Select "Upload".
When prompted use the BME680-manifest.json provided on our GitHub. You should now see the template on the list. Click on it.
In the next step click the Create device button, name it and be sure to send the key/pass combination to your mail or copy/paste it someplace, because you're going to need it in order to connect to the device.
Open the .ino file and change the connection details to suit your own wifi and device.
Change the time values if you desire so.

##Code

```c
#include <Adafruit_BME680.h>
#include <bme680_defs.h>
#include <bme680.h>

#include <WiFi101.h>
#include <RTCZero.h>
#include <FlashStorage.h>

#include "WolkConn.h"
#include "MQTTClient.h"
/*Number of outbound_message_t to store*/
#define STORAGE_SIZE 32
#define SEALEVELPRESSURE_HPA (1013.25)

/*Circular buffer to store outbound messages to persist*/
typedef struct{

  boolean valid;

  outbound_message_t outbound_messages[STORAGE_SIZE];

  uint32_t head;
  uint32_t tail;

  boolean empty;
  boolean full;

} Messages;

static Messages data;
/*Connection details*/
const char* ssid = "ssid";
const char* wifi_pass = "pass";

const char *device_key = "device_key";
const char *device_password = "device_password";
const char* hostname = "api-demo.wolkabout.com";
int portno = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

/* WolkConnect-Arduino Connector context */
static wolk_ctx_t wolk;
/* Init flash storage */
FlashStorage(flash_store, Messages);
/*Init i2c sensor communication*/
Adafruit_BME680 bme;

RTCZero rtc;
/* Change these values to set the current initial time */
const byte seconds = 0;
const byte minutes = 0;
const byte hours = 17;

/* Change these values to set the current initial date */
const byte day = 17;
const byte month = 11;
const byte year = 15;
bool read;
/*Read sensor every 15 minutes. If you change this parameter
make sure that it's <60*/
const byte readEvery = 15;
bool publish;
/*Publish every hour. If you change this parameter
make sure that it's <24*/
const byte publishEvery = 1;
byte publishHours = (hours + publishEvery) % 24;

/*Flash storage and custom persistence implementation*/
void _flash_store()
{
  data.valid = true;
  flash_store.write(data);
}
void increase_pointer(uint32_t* pointer)
{
    if ((*pointer) == (STORAGE_SIZE - 1))
    {
        (*pointer) = 0;
    }
    else
    {
        (*pointer)++;
    }
}

void _init()
{
    data = flash_store.read();

    if (data.valid == false)
    {
      data.head = 0;
      data.tail = 0;

      data.empty = true;
      data.full = false;

    }
}

bool _push(outbound_message_t* outbound_message)
{
    if(data.full)
    {
        increase_pointer(&data.head);
    }

    memcpy(&data.outbound_messages[data.tail], outbound_message, sizeof(outbound_message_t));

    increase_pointer(&data.tail);
    
    data.empty = false;
    data.full = (data.tail == data.head);

    return true;
}

bool _peek(outbound_message_t* outbound_message)
{
    memcpy(outbound_message, &data.outbound_messages[data.head], sizeof(outbound_message_t));
    return true;
}

bool _pop(outbound_message_t* outbound_message)
{
    memcpy(outbound_message, &data.outbound_messages[data.head], sizeof(outbound_message_t));
    
    increase_pointer(&data.head);
    
    data.full = false;
    data.empty = (data.tail == data.head);

    return true;
}

bool _is_empty()
{
    return data.empty;
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  WiFi.begin(ssid);

  if ( WiFi.status() != WL_CONNECTED) {
    while (WiFi.begin(ssid, wifi_pass) != WL_CONNECTED) {
      delay(300);
    }
  }
}

void setup() {
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  /*Initialize the circular buffer structure*/
  _init();
  
  setup_wifi();

  wolk_init(&wolk, NULL, NULL, NULL, NULL,
            device_key, device_password, &client, hostname, portno, PROTOCOL_JSON_SINGLE, NULL, NULL);

  wolk_init_custom_persistence(&wolk, _push, _peek, _pop, _is_empty);

  delay(1000);
  /*The on board LED will turn on if something went wrong*/
  if(!bme.begin())
  {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  /*Sensor init*/
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  rtc.begin();

  rtc.setTime(hours, minutes, seconds);
  rtc.setDate(day, month, year);

  rtc.setAlarmTime(17, 00, 10);
  rtc.enableAlarm(rtc.MATCH_MMSS);

  rtc.attachInterrupt(alarmMatch);
  
  WiFi.lowPowerMode();
  rtc.standbyMode();

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
    digitalWrite(LED_BUILTIN, HIGH);
    }
    
    wolk_add_numeric_sensor_reading(&wolk, "T", bme.temperature, 0);
    wolk_add_numeric_sensor_reading(&wolk, "H", bme.humidity, 0);
    wolk_add_numeric_sensor_reading(&wolk, "P", bme.pressure / 100.0, 0);
    wolk_add_numeric_sensor_reading(&wolk, "GR", bme.gas_resistance / 1000.0, 0);
    wolk_add_numeric_sensor_reading(&wolk, "A", bme.readAltitude(SEALEVELPRESSURE_HPA), 0);
    
    /*set new alarm*/
    int alarmMin = (rtc.getMinutes() + readEvery) % 60;
    rtc.setAlarmMinutes(alarmMin);
    delay(100);
  }
  
  if(publish)
  {
    publish = false;
    wolk_connect(&wolk);
    if(!wolk.is_connected)
    {
      _flash_store();
    }
    delay(100);
    wolk_publish(&wolk);
    /*set new publish time*/
    publishHours = (rtc.getHours() + publishEvery) % 24;
    delay(100);
    wolk_disconnect(&wolk);
    delay(100);
  }
  rtc.standbyMode();

}
/*Timed interrupt routine*/
void alarmMatch()
{
  read = true;
  if(publishHours == rtc.getHours())
  {
    publish = true;
  }
}
```
