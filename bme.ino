extern Adafruit_BME680 bme;

void init_bme()
{
  /*The on board LED will turn on if something went wrong*/
  if(!bme.begin())
  {
    Serial.println("BME failed to initialize!");
  }
  /*Sensor init*/
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
}
