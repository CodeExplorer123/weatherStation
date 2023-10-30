 // Weather station control system
 // Author: Kristofers  Nīmanis

#include <Wire.h>
#include <uRTCLib.h>
#include <SD.h>
#include <SPI.h>
#include <SoftwareSerial.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>

#define AHT21_ADDRESS 0x38
#define RTC_ADDRESS 0x68
#define BMP280_ADDRESS 0x76

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 0, 43200000);

const char *ssid = "SSID-HERE";
const char *password = "PASSWORD-HERE";

const char *APssid = "ESP32-Access-Point";
const char *APpassword = "12345678";

unsigned long startMillis;
unsigned long currentMillis;
const unsigned long period = 180000;  // 3 minutes

#define SD_CS_PIN 27
#define SIM_TX_PIN 32
#define SIM_RX_PIN 33
#define RAINFALL_PIN 14
#define MAX485 4
#define RELAY_PIN 15
#define RX2 16
#define TX2 17


// Define the interval at which to take readings
#define READ_INTERVAL 400  //(in ms)

#define debounceDelay 150

String urlSignal = "http://api.callmebot.com/signal/send.php?phone=+xxxxxxxxx&apikey=xxxxx&text=";
String url = "http://REPLACE.xyz:33733/";

File dataFile;
Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;
SoftwareSerial mySerial(RX2, TX2);

volatile unsigned long rainCount = 0;
volatile long lastInterruptTime = 0;
unsigned long lastReadTime = 0;
int lastMinute = 99;

void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(APssid, APpassword);
  Serial.println(WiFi.softAPIP());

  mySerial.begin(4800);

  pinMode(MAX485, OUTPUT);
  pinMode(RAINFALL_PIN, INPUT_PULLUP);
  pinMode(RELAY_PIN, OUTPUT);

  URTCLIB_WIRE.begin();

  long long startTime = millis();
  while (startTime + 200000 > millis()) {
    ArduinoOTA.handle();
  }
  ArduinoOTA.end();

  pinMode(SD_CS_PIN, OUTPUT);
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD card initialization failed!");
    return;
  }
  Serial.println("SD card initialized!");

  if (!aht.begin()) {
    Serial.println("AHT21 initialization failed!");
    return;
  }
  Serial.println("AHT21 initialized!");

  if (!bmp.begin(BMP280_ADDRESS)) {
    Serial.println("BMP280 initialization failed!");
    return;
  }
  Serial.println("BMP280 initialized!");

  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X16, Adafruit_BMP280::STANDBY_MS_500);

  attachInterrupt(digitalPinToInterrupt(RAINFALL_PIN), rainfallInterrupt, FALLING);
}

void loop() {
  timeClient.update();
  time_t utc3Time = convertUnixToUTC3(timeClient.getEpochTime());

  int hourNow = hour(utc3Time);
  int minuteNow = minute(utc3Time);
  int secondNow = second(utc3Time);
  String dateTimeString = String(timeClient.getEpochTime());

  if (millis() - lastReadTime >= READ_INTERVAL) {
    if (secondNow == 0) {
      if (lastMinute != minuteNow) {
        lastMinute = minuteNow;
        lastReadTime = millis();

        sensors_event_t humidity, temp;
        aht.getEvent(&humidity, &temp);
        float pressure = bmp.readPressure() / 100.0F;
        float temperature_bmp = bmp.readTemperature();
        float speedValue;
        int dirValue;
        windDataStack(speedValue, dirValue);

        String fullString = dateTimeString + ";" + String(temp.temperature) + ";" + String(humidity.relative_humidity) + ";" + String(pressure) + ";" + String(temperature_bmp) + ";" + String(speedValue) + ";" + String(dirValue) + ";" + String(rainCount) + "\n";
        rainCount = 0;

        appendFile(SD, "/data.txt", fullString.c_str());
        Serial.println(fullString);
        sendWifi(fullString);
      }
    }
  }
}

void sendWifi(String data) {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.disconnect();
    WiFi.reconnect();
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(url);
    int httpCode = http.POST(data);

    if (httpCode > 0) {
      Serial.println(httpCode);
    } else {
      Serial.println("Error on HTTP request");
    }

    http.end();
  }
}


void AbfrageFrameDataSpeed() {
  byte data[8];
  data[0] = 0x01;  ////speed
  data[1] = 0x03;
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = 0x00;
  data[5] = 0x01;
  data[6] = 0x84;
  data[7] = 0x0A;

  for (int d = 0; d < 8; d++) {
    mySerial.write(data[d]);
  }
  mySerial.flush();
}

void AbfrageFrameDataDir() {
  byte data[8];
  data[0] = 0x01;  ////dir
  data[1] = 0x03;
  data[2] = 0x00;
  data[3] = 0x00;
  data[4] = 0x00;
  data[5] = 0x02;
  data[6] = 0xC4;
  data[7] = 0x0B;

  for (int d = 0; d < 8; d++) {
    mySerial.write(data[d]);
  }
  mySerial.flush();
}

int DirRead() {
  int us = 10000;
  while (us--) {
    ets_delay_us(1);  // this is a function provided by <util/delay.h> library in AVR microcontrollers
  }
  while (mySerial.available()) {

    byte Antwort_buf[9];
    mySerial.readBytes(Antwort_buf, 9);

    int ResultDir = (Antwort_buf[4]);

    ResultDir = ResultDir * 45;  //convert to deg

    Serial.print((ResultDir));
    Serial.print(" deg");
    Serial.println();

    return ResultDir;
  }
  // Add a default return value in case no data is available
  return -1;
}

float SpeedRead() {
  int us = 10000;
  while (us--) {
    ets_delay_us(1);  // this is a function provided by <util/delay.h> library in AVR microcontrollers
  }
  while (mySerial.available()) {

    byte Antwort_buf[7];
    mySerial.readBytes(Antwort_buf, 7);

    float speedReadig = (Antwort_buf[4]);

    speedReadig = speedReadig / 10.0;
    Serial.print((speedReadig));
    Serial.print(" m/s");
    Serial.println();

    return speedReadig;
  }
  // Add a default return value in case no data is available
  return -1;
}

void windDataStack(float &speed, int &dir) {
  digitalWrite(MAX485, HIGH);
  digitalWrite(RELAY_PIN, LOW);
  AbfrageFrameDataSpeed();
  digitalWrite(MAX485, LOW);
  speed = SpeedRead();

  digitalWrite(MAX485, HIGH);
  digitalWrite(RELAY_PIN, HIGH);
  AbfrageFrameDataDir();
  digitalWrite(MAX485, LOW);
  dir = DirRead();
  digitalWrite(RELAY_PIN, LOW);
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}

time_t convertUnixToUTC3(time_t unixTime) {
  struct tm *localTime = localtime(&unixTime);
  localTime->tm_hour += 3;
  time_t utc3Time = mktime(localTime);
  return utc3Time;
}

void rainfallInterrupt() {
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > debounceDelay) {
    rainCount++;
  }
  lastInterruptTime = interruptTime;
}
