
/*
  vMix wireless tally
  Copyright 2021 Thomas Mout

*/

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <WiFiClient.h>
#include "FS.h"

// Constants
const int SsidMaxLength = 64;
const int PassMaxLength = 64;
const int HostNameMaxLength = 64;
const int TallyNumberMaxValue = 64;

const int PROGRAM_LED2 = 4; //D2
const int PROGRAM_LED = 14; //D5
const int PREVIEW_LED = 12; //D6
const int STANDBY_LED = 13; //D7

// Settings object
struct Settings
{
  char ssid[SsidMaxLength];
  char pass[PassMaxLength];
  char hostName[HostNameMaxLength];
  int tallyNumber;
};

// Default settings object
Settings defaultSettings = {
  "ssid default",
  "pass default",
  "hostname default",
  1
};

Settings settings;

// HTTP Server settings
ESP8266WebServer httpServer(80);
char deviceName[32];
int status = WL_IDLE_STATUS;
bool apEnabled = false;
char apPass[64];

// vMix settings
int port = 8099;

// Tally info
char currentState = -1;
const char tallyStateOff = 0;
const char tallyStateProgram = 1;
const char tallyStatePreview = 2;

// The WiFi client
WiFiClient client;
int timeout = 10;
int delayTime = 10000;

// Time measure
int interval = 5000;
unsigned long lastCheck = 0;

// Load settings from EEPROM
void loadSettings()
{
  Serial.println("------------");
  Serial.println("Loading settings");

  long ptr = 0;

  for (int i = 0; i < SsidMaxLength; i++)
  {
    settings.ssid[i] = EEPROM.read(ptr);
    ptr++;
  }

  for (int i = 0; i < PassMaxLength; i++)
  {
    settings.pass[i] = EEPROM.read(ptr);
    ptr++;
  }

  for (int i = 0; i < HostNameMaxLength; i++)
  {
    settings.hostName[i] = EEPROM.read(ptr);
    ptr++;
  }

  settings.tallyNumber = EEPROM.read(ptr);

  if (strlen(settings.ssid) == 0 || strlen(settings.pass) == 0 || strlen(settings.hostName) == 0 || settings.tallyNumber == 0)
  {
    Serial.println("No settings found");
    Serial.println("Loading default settings");
    settings = defaultSettings;
    saveSettings();
    restart();
  }
  else
  {
    Serial.println("Settings loaded");
    printSettings();
    Serial.println("------------");
  }
}

// Save settings to EEPROM
void saveSettings()
{
  Serial.println("------------");
  Serial.println("Saving settings");

  long ptr = 0;

  for (int i = 0; i < 512; i++)
  {
    EEPROM.write(i, 0);
  }

  for (int i = 0; i < SsidMaxLength; i++)
  {
    EEPROM.write(ptr, settings.ssid[i]);
    ptr++;
  }

  for (int i = 0; i < PassMaxLength; i++)
  {
    EEPROM.write(ptr, settings.pass[i]);
    ptr++;
  }

  for (int i = 0; i < HostNameMaxLength; i++)
  {
    EEPROM.write(ptr, settings.hostName[i]);
    ptr++;
  }

  EEPROM.write(ptr, settings.tallyNumber);

  EEPROM.commit();

  Serial.println("Settings saved");
  printSettings();
  Serial.println("------------");
}

// Print settings
void printSettings()
{
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(settings.ssid);
  Serial.print("SSID Password: ");
  Serial.print(settings.pass[0]);
  Serial.print(settings.pass[1]);
  Serial.println("********");
  Serial.print("vMix hostname: ");
  Serial.println(settings.hostName);
  Serial.print("Tally number: ");
  Serial.println(settings.tallyNumber);
}

// Set LED's off
void ledSetOff()
{
  digitalWrite(PREVIEW_LED, LOW);
  digitalWrite(PROGRAM_LED, LOW);
  digitalWrite(STANDBY_LED, HIGH);
  digitalWrite(PROGRAM_LED2, LOW);  
}

// Draw L(ive) with LED's
void ledSetProgram()
{
  digitalWrite(PREVIEW_LED, LOW);
  digitalWrite(PROGRAM_LED, HIGH);
  digitalWrite(STANDBY_LED, LOW);
  digitalWrite(PROGRAM_LED2, HIGH);  
}

// Draw P(review) with LED's
void ledSetPreview()
{
  digitalWrite(PREVIEW_LED, HIGH);
  digitalWrite(PROGRAM_LED, LOW);Serial.print(settings.pass[0]);
  Serial.print(settings.pass[1]);
  digitalWrite(STANDBY_LED, LOW);
  digitalWrite(PROGRAM_LED2, LOW);  
}

// Draw C(onnecting) with LED's
void ledSetConnecting()
{
  digitalWrite(PREVIEW_LED, HIGH);
  digitalWrite(PROGRAM_LED, LOW);
  digitalWrite(STANDBY_LED, HIGH);
  digitalWrite(PROGRAM_LED2, LOW);  
}

// Draw S(ettings) with LED's
void ledSetSettings()
{
  digitalWrite(PREVIEW_LED, LOW);
  digitalWrite(PROGRAM_LED, HIGH);
  digitalWrite(STANDBY_LED, HIGH);
  digitalWrite(PROGRAM_LED2, LOW);  
}

// Set tally to off
void tallySetOff()
{
  Serial.println("Tally off");
  ledSetOff();
}

// Set tally to program
void tallySetProgram()
{
  Serial.println("Tally program");
  ledSetProgram();
}

// Set tally to preview
void tallySetPreview()
{
  Serial.println("Tally preview");
  ledSetPreview();
}

// Set tally to connecting
void tallySetConnecting()
{
  ledSetConnecting();
}

// Handle incoming data
void handleData(String data)
{
  // Check if server data is tally data
  if (data.indexOf("TALLY") == 0)
  {
    char newState = data.charAt(settings.tallyNumber + 8);

    // Check if tally state has changed
    if (currentState != newState)
    {
      currentState = newState;

      switch (currentState)
      {
        case '0':
          tallySetOff();
          break;
        case '1':
          tallySetProgram();
          break;
        case '2':
          tallySetPreview();
          break;
        default:
          tallySetOff();
      }
    }
  }
  else
  {
    Serial.print("Response from vMix: ");
    Serial.println(data);
  }
}

// Start access point
void apStart()
{
  ledSetSettings();
  Serial.println("AP Start");
  Serial.print("AP SSID: ");
  Serial.println(deviceName);
  Serial.print("AP password: ");
  Serial.println(apPass);

  WiFi.mode(WIFI_AP);
  WiFi.hostname(deviceName);
  WiFi.softAP(deviceName, apPass);
  delay(100);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("IP address: ");
  Serial.println(myIP);

  apEnabled = true;
}

// Hanle http server root request
void rootPageHandler()
{
  String response_message = "<!DOCTYPE html>";
  response_message += "<html lang='en'>";
  response_message += "<head>";
  response_message += "<title>" + String(deviceName) + "</title>";
  response_message += "<meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'>";
  response_message += "<meta charset='utf-8'>";
  response_message += "<link rel='icon' type='image/x-icon' href='favicon.ico'>";
  response_message += "<link rel='stylesheet' href='styles.css'>";
  response_message += "<style>body {width: 100%;height: 100%;padding: 25px;}</style>";
  response_message += "</head>";

  response_message += "<body class='bg-light'>";

  response_message += "<h1>vMix tally " + String(settings.tallyNumber) + "</h1>";
  response_message += "<div data-role='content' class='row'>";

  response_message += "<div class='col-md-6'>";
  response_message += "<h2>Settings</h2>";
  response_message += "<form action='/save' method='post' enctype='multipart/form-data' data-ajax='false' class='border'>";

  response_message += "<div class='form-group row'>";
  response_message += "<label for='ssid' class='col-sm-4 col-form-label'>SSID</label>";
  response_message += "<div class='col-sm-8'>";
  response_message += "<input id='ssid' class='form-control' type='text' size='64' maxlength='64' name='ssid' value='" + String(settings.ssid) + "'>";
  response_message += "</div></div>";

  response_message += "<div class='form-group row'>";
  response_message += "<label for='ssidpass' class='col-sm-4 col-form-label'>SSID password</label>";
  response_message += "<div class='col-sm-8'>";
  //response_message += "<input id='ssidpass' class='form-control' type='text' size='64' maxlength='64' name='ssidpass' value='" + String(settings.pass) + "'>";
  response_message += "<input id='ssidpass' class='form-control' type='password' size='64' maxlength='64' name='ssidpass' value=''>";
  response_message += "<label>(leave blank to leave unchanged.)</label>";
  response_message += "</div></div>";

  response_message += "<div class='form-group row'>";
  response_message += "<label for='hostname' class='col-sm-4 col-form-label'>vMix hostname</label>";
  response_message += "<div class='col-sm-8'>";
  response_message += "<input id='hostname' class='form-control' type='text' size='64' maxlength='64' name='hostname' value='" + String(settings.hostName) + "'>";
  response_message += "</div></div>";

  response_message += "<div class='form-group row'>";
  response_message += "<label for='inputnumber' class='col-sm-4 col-form-label'>Input number (1-1000)</label>";
  response_message += "<div class='col-sm-8'>";
  response_message += "<input id='inputnumber' class='form-control' type='number' size='64' min='0' max='1000' name='inputnumber' value='" + String(settings.tallyNumber) + "'>";
  response_message += "</div></div>";

  response_message += "<input type='submit' value='SAVE' class='btn btn-primary'></form>";
  response_message += "</div>";

  response_message += "<div class='col-md-6'>";
  response_message += "<h2>Device information</h2><div class='border'>";
  response_message += "<table class='table'><tbody>";

  char ip[13];
  sprintf(ip, "%d.%d.%d.%d", WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], WiFi.localIP()[3]);
  response_message += "<tr><th>IP</th><td>" + String(ip) + "</td></tr>";

  response_message += "<tr><th>MAC</th><td>" + String(WiFi.macAddress()) + "</td></tr>";
  response_message += "<tr><th>Signal Strength</th><td>" + String(WiFi.RSSI()) + " dBm</td></tr>";
  response_message += "<tr><th>Device Name</th><td>" + String(deviceName) + "</td></tr>";

  if (WiFi.status() == WL_CONNECTED)
  {
    response_message += "<tr><th>Status</th><td>Connected</td></tr>";
  }
  else
  {
    response_message += "<tr><th>Status</th><td>Disconnected</td></tr>";
  }

  if (apEnabled)
  {
    sprintf(ip, "%d.%d.%d.%d", WiFi.softAPIP()[0], WiFi.softAPIP()[1], WiFi.softAPIP()[2], WiFi.softAPIP()[3]);
    response_message += "<tr><th>AP</th><td>Active (" + String(ip) + ")</td></tr>";
  }
  else
  {
    response_message += "<tr><th>AP</th><td>Inactive</td></tr>";
  }
  response_message += "</tbody></table>";
  response_message += "</div>";
  response_message += "</div>";
  response_message += "</div>";

  response_message += "</body>";
  response_message += "</html>";

  httpServer.sendHeader("Connection", "close");
  httpServer.send(200, "text/html", String(response_message));
}

// Settings POST handler
void handleSave()
{
  bool doRestart = false;

  httpServer.sendHeader("Location", String("/"), true);
  httpServer.send(302, "text/plain", "Redirected to: /");

  if (httpServer.hasArg("ssid"))
  {
    if (httpServer.arg("ssid").length() <= SsidMaxLength)
    {
      httpServer.arg("ssid").toCharArray(settings.ssid, SsidMaxLength);
      doRestart = true;
    }
  }

  if (httpServer.hasArg("ssidpass"))
  {
    if (httpServer.arg("ssidpass").length() <= PassMaxLength
        && httpServer.arg("ssidpass").length() > 0)
    {
      httpServer.arg("ssidpass").toCharArray(settings.pass, PassMaxLength);
      doRestart = true;
    }
  }

  if (httpServer.hasArg("hostname"))
  {
    if (httpServer.arg("hostname").length() <= HostNameMaxLength)
    {
      httpServer.arg("hostname").toCharArray(settings.hostName, HostNameMaxLength);
      doRestart = true;
    }
  }

  if (httpServer.hasArg("inputnumber"))
  {
    if (httpServer.arg("inputnumber").toInt() > 0 and httpServer.arg("inputnumber").toInt() <= TallyNumberMaxValue)
    {
      settings.tallyNumber = httpServer.arg("inputnumber").toInt();
      doRestart = true;
    }
  }

  if (doRestart == true)
  {
    restart();
  }
}

// Connect to WiFi
void connectToWifi()
{
  Serial.println();
  Serial.println("------------");
  Serial.println("Connecting to WiFi");
  Serial.print("SSID: ");
  Serial.println(settings.ssid);
  //Serial.print("SSID Password: ");
  //Serial.println(settings.pass);

  int timeout = 15;

  WiFi.mode(WIFI_STA);
  WiFi.hostname(deviceName);
  WiFi.begin(settings.ssid, settings.pass);

  Serial.print("Waiting for connection.");
  while (WiFi.status() != WL_CONNECTED and timeout > 0)
  {
    delay(1000);
    timeout--;
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Success!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Device name: ");
    Serial.println(deviceName);
    Serial.println("------------");
  }
  else
  {
    if (WiFi.status() == WL_IDLE_STATUS)
      Serial.println("Idle");
    else if (WiFi.status() == WL_NO_SSID_AVAIL)
      Serial.println("No SSID Available");
    else if (WiFi.status() == WL_SCAN_COMPLETED)
      Serial.println("Scan Completed");
    else if (WiFi.status() == WL_CONNECT_FAILED)
      Serial.println("Connection Failed");
    else if (WiFi.status() == WL_CONNECTION_LOST)
      Serial.println("Connection Lost");
    else if (WiFi.status() == WL_DISCONNECTED)
      Serial.println("Disconnected");
    else
      Serial.println("Unknown Failure");

    Serial.println("------------");
    apStart();
  }
}

// Connect to vMix instance
void connectTovMix()
{
  Serial.print("Connecting to vMix on ");
  Serial.print(settings.hostName);
  Serial.print("...");

  if (client.connect(settings.hostName, port))
  {
    Serial.println(" Connected!");
    Serial.println("------------");

    tallySetOff();

    // Subscribe to the tally events
    client.println("SUBSCRIBE TALLY");
  }
  else
  {
    Serial.println(" Not found!");
  }
}

void restart()
{
  saveSettings();

  Serial.println();
  Serial.println();
  Serial.println("------------");
  Serial.println("------------");
  Serial.println("RESTART");
  Serial.println("------------");
  Serial.println("------------");
  Serial.println();
  Serial.println();

  start();
}

void start()
{
  tallySetConnecting();

  loadSettings();
  sprintf(deviceName, "vMix_Tally_%d", settings.tallyNumber);
  sprintf(apPass, "%s%s", deviceName, "_access");

  connectToWifi();

  if (WiFi.status() == WL_CONNECTED)
  {
    connectTovMix();
  }
}

void setup()
{
  Serial.begin(9600);
  EEPROM.begin(512);
  SPIFFS.begin();

  pinMode(PREVIEW_LED, OUTPUT);
  pinMode(PROGRAM_LED, OUTPUT);
  pinMode(PROGRAM_LED2, OUTPUT);
  pinMode(STANDBY_LED, OUTPUT);

  httpServer.on("/", HTTP_GET, rootPageHandler);
  httpServer.on("/save", HTTP_POST, handleSave);
  httpServer.serveStatic("/", SPIFFS, "/", "max-age=315360000");
  httpServer.begin();

  flashLeds();
  start();
}

void flashLeds()
{ 
  digitalWrite(STANDBY_LED, HIGH);

  for (int8_t i = 0; i < 2; i++)
  {
    delay(250);
    cycleLed(STANDBY_LED,PREVIEW_LED);
    delay(250);
    cycleLed(PREVIEW_LED,PROGRAM_LED);
    delay(250);
    cycleLed(PROGRAM_LED,PROGRAM_LED2);
    delay(250);
    cycleLed(PROGRAM_LED2,STANDBY_LED);
  }
  delay(250);
  ledSetOff();
}

void cycleLed(int LED1, int LED2)
{
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, HIGH);
}

void loop()
{
  httpServer.handleClient();

  while (client.available())
  {
    String data = client.readStringUntil('\r\n');
    handleData(data);
  }

  if (!client.connected() && !apEnabled && millis() > lastCheck + interval)
  {
    tallySetConnecting();

    client.stop();

    connectTovMix();
    lastCheck = millis();
  }
}
