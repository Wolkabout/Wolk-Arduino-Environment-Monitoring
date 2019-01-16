# Wolk-Arduino-Environment-Monitoring

### Story

We had some Environment click sensors lying around so we decided to hook them up to the MKR1000 and visualise them on the [Wolkabout platform](https://demo.wolkabout.com/). The examples provided within the libraries we used made it quick to implement.
The idea is to do the measurement every 15 minutes and publish the results once every hour.
If the publish fails (due to a busy network or some other issue) the results should be persisted in Flash memory of the device. With max. 24 writes a day we avoid getting the Flash in a faulty state (Arduino guarantees 10000 write cycles).
In the meantime all devices go into sleep mode in order to minimise the power consumption. 

## Dependencies
##### Hardware components
- [Arduino MKR1000 WiFi](https://www.hackster.io/arduino/products/arduino-mkr1000)
- [Environment click](https://www.mikroe.com/environment-click)

##### Software
- Arduino IDE
- [WiFi101 library](https://github.com/arduino-libraries/WiFi101)
- [Adafruit BME680 library](https://github.com/adafruit/Adafruit_BME680)
- [RTCZero library](https://github.com/arduino-libraries/RTCZero)
- [Flash storage library](https://github.com/cmaglie/FlashStorage)
- [WolkConnect library](https://github.com/Wolkabout/WolkConnect-Arduino)
All libraries are available via the Arduino Library Manager.

##### You'll also need

- An account on the [WolkAbout platform](https://demo.wolkabout.com/) and 
- a device created using the BME680-manifest.json file provided on our [GitHub](https://github.com/Wolkabout/Wolk-Arduino-Environment-Monitoring) 

### How it works

The starting point for this project were the examples provided in the [WolkConnect-Arduino library](https://github.com/Wolkabout/WolkConnect-Arduino).

The flash persistence example provides all the necessary functions for a custom persistence implementation. It implements a structure of a circular buffer and uses it to store the measurement results. There is also an implementation of how to store that structure in Flash memory using the Flash storage library.
The Timed measure example implements timed reading and publishing of the data using RTCZero library. All you need to do is change time variables in order to represent the actual time and set the intervals you wish to use for the read and publish.
The sensor also has a library which makes reading easy.
The alarm interrupt changes flags for tasks that need to be done (read and publish). The rest of the work falls on the loop function.
Here we check if we need to read or publish and go to sleep when it's all done. For minimal power consumtion, WiFi is set to be in a low power mode as well as the MKR1000. The sensors go to low power automaticaly until a reading is requested.

## How to use it
#### Hardware setup

You'll need the Arduino MKR1000 and the Environment click sensor.
The Environment click sensor is hooked up to the I2C communication pins (namely 11 and 12 on the MKR1000), Vcc and ground.
#### Software
After registering an account on the [WolkAbout platform](https://demo.wolkabout.com/) click on Devices->Device templates.
Then click on the plus sign to add template. Select "Upload".
When prompted use the BME680-manifest.json provided on our GitHub. You should now see the template on the list. Click on it.
In the next step click the Create device button, name it and be sure to send the key/pass combination to your mail or copy/paste it someplace, because you're going to need it in order to connect to the device.
Open the .ino file and change the connection details to suit your own wifi and device.
Change the time values if you desire so.
Verify and upload to your MKR1000 board.
