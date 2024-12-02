#include <Arduino.h>
#include <HardwareSerial.h>
#include "BPparse.h"
#include <WiFi.h>
#include <esp_wifi.h>

#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

// Insert your network credentials
#define WIFI_SSID "SmartBPMonitor"
#define WIFI_PASSWORD "123456789"

// Insert Firebase project API Key
#define API_KEY "ENTER_YOUR_API_KEY"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "ENTER_YOUR_RTDB_URL"

// ntp client
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 0;

// Function that gets current epoch time
unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    // Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

// get MAC address
String readMacAddress()
{
  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  char buffer[128];
  if (ret == ESP_OK)
  {
    sprintf(buffer, "%d%d%d%d%d%d",
            baseMac[0], baseMac[1], baseMac[2],
            baseMac[3], baseMac[4], baseMac[5]);
  }
  return String(buffer);
}

// define serial1
#define RXD2 9
#define TXD2 10

void setup()
{
  // setup led
  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);
  // setting up usb serial
  Serial.begin(115200);
  // setting up serial for communicating to BP monitor
  Serial1.begin(115200, SERIAL_8N1, RXD2, TXD2);
  // init wifi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    digitalWrite(8, HIGH);
    delay(500);
    digitalWrite(8, LOW);
    delay(500);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  digitalWrite(8, HIGH);

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", ""))
  {
    Serial.println("ok");
    signupOK = true;
  }
  else
  {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

// RAW READOUTS
// startBPTest
// test return:0save record
//
// 8C 61 00 46 00 00 E4 07 01 01 08 23 37 00

int state = 0;

void loop()
{
  if (Serial1.available() > 0)
  {
    String inp = Serial1.readStringUntil('\n');
    inp.trim();
    if (inp == String("startBPTest"))
      Serial.println("test started!");
    else if (inp == String("test return:0save record"))
    {
      while (true)
      {
        if (Serial1.available() > 0)
        {
          String inp2 = Serial1.readStringUntil('\n');
          inp2.trim();
          if (inp2 != "")
          {
            Serial.print("test done! result: ");
            Serial.println(inp2);
            // Prepare a buffer for the parsed hex data
            uint8_t hexData[14];
            size_t length;

            // Convert input string to byte array
            if (parseHexInput(inp2, hexData, sizeof(hexData), length) && length == 14)
            {
              // Parse the record
              Record record = parseRecord(hexData);

              // Output the parsed data
              Serial.println("Record Data:");
              Serial.print("Systolic: ");
              Serial.println(record.sys);
              Serial.print("Diastolic: ");
              Serial.println(record.dia);
              Serial.print("Pulse: ");
              Serial.println(record.pulse);
              Serial.print("Mean: ");
              Serial.println(record.mean);
              Serial.print("Heat: ");
              Serial.println(record.heat);
              Serial.print("Timestamp: ");
              Serial.println(record.record_time);
              // write data to rtdb
              if (Firebase.ready() && signupOK)
              {
                // Write an Int number on the database path test/sys
                char pathBuffer[256];
                sprintf(pathBuffer, "%d/%d/", readMacAddress().toInt(), getTime());
                char recordBuffer[256];
                sprintf(recordBuffer, "%d;%d;%d", record.sys, record.dia, record.pulse);
                Serial.println(readMacAddress());
                if (Firebase.RTDB.setString(&fbdo, pathBuffer, recordBuffer))
                {
                  Serial.println("PASSED");
                  Serial.println("PATH: " + fbdo.dataPath());
                  Serial.println("TYPE: " + fbdo.dataType());
                }
                else
                {
                  Serial.println("FAILED");
                  Serial.println("REASON: " + fbdo.errorReason());
                }
              }
              else
              {
                Serial.println("Invalid input.");
              }
              break;
            }
          }
        }
      }
    }
  }
}