// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

// Publish 1 message every 2 seconds
#define TELEMETRY_FREQUENCY_MILLISECS 2000

static const char* host = "IOT_CONFIG_IOTHUB_FQDN";
static const char* device_id = "IOT_CONFIG_DEVICE_ID";
static const char* device_key = "IOT_CONFIG_DEVICE_KEY";
static const int port = 8883;

// include SD card library
#include <SPI.h>
#include <SD.h>

// http://easycoding.tn/tuniot/demos/code/
// D2 -> SDA
// D1 -> SCL      display( address of display, SDA,SCL)
#include "SSD1306.h"
SSD1306  display(0x3C, 4, 5);

// declare functions
void sendToDisplay(int col, int row, String data);
void sendToDisplay(int col, int row, int len, String data);

//
// all support functions
//
void sendToDisplay(int col, int row, String data)
{
  // display is 64 x 128. 64 is rows. 
  // 5 rows with font of 10. first row is yellow do add 2
  if (row != 0) 
  {
    row = row * 12.8;
  }
  if(row > 64)
  {
    row = 64;
  }
  
	display.drawString(col, row, data);
	display.display();
}


void sendToDisplay(int col, int row,int len, String data)
{
	display.drawStringMaxWidth(col, row,len, data);
	display.display();
}

void printDirectory(File dir, int numTabs) {
  while (true) {

    File entry = dir.openNextFile();
    if (!entry) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) { Serial.print('\t'); }
    Serial.print(entry.name());
    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.print(entry.size(), DEC);
      time_t cr = entry.getCreationTime();
      time_t lw = entry.getLastWrite();
      struct tm* tmstruct = localtime(&cr);
      Serial.printf("\tCREATION: %d-%02d-%02d %02d:%02d:%02d", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
      tmstruct = localtime(&lw);
      Serial.printf("\tLAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
    entry.close();
  }
}


static char* createJsonData(String devId, String key, float temp, float humidity, String lat, String lon)
{

  // create json object
  DynamicJsonDocument doc(1024);
  String outdata;
  
  doc["deviceId"] = devId;
  doc["keyid"] = key ;
  doc["temperature"] = temp;
  doc["humidity"] = humidity;
  doc["lat"] = lat;
  doc["lon"] = lon;

  //doc["sensor"] = sensor;

  serializeJson(doc, outdata);
  Serial.println("  " + outdata);

  char *cstr = new char[outdata.length() + 1];
  strcpy(cstr, outdata.c_str());
  return cstr;

}

static void initializeTime()
{
  //https://cplusplus.com/reference/ctime/mktime/
  Serial.print("Setting time using SNTP");
  
  // this sets the time and timezone
  configTime(passData[7].toInt() * ONE_HOUR_IN_SECS , 0, NTP_SERVERS);
  time_t now = time(NULL);
  while (now < 1510592825)
  {
    delay(500);
    Serial.print(".");
    now = time(NULL);
  }
  Serial.println("done!");

  
  // initialize google wifi location with api key
  Serial.println("Connecting to Geo location API");
  WifiLocation location( passData[10] );

  //  get lat lon of device
  location_t loc = location.getGeoFromWiFi ();

  Serial.println ("Location request data");
  Serial.println (location.getSurroundingWiFiJson () + "\n");
  Serial.println ("Location: " + String (loc.lat, 7) + "," + String (loc.lon, 7));
  
  Serial.println ("Accuracy: " + String (loc.accuracy));
  Serial.println ("Geolocation result: " + location.wlStatusStr (location.getStatus ())); 

  passData[8] = String (loc.lat, 7);
  passData[9] = String (loc.lon, 7);

  Serial.println("Geo location Complete");
  
}


static String getCurrentLocalTimeString()
{
  char buffer[80]; // declare a buffer to store the formatted string

  struct tm * timeinfo;
  time_t now = time(NULL);  

  //Serial.print("now: ");
  //Serial.println(ctime(&now));
  
  timeinfo = localtime(&now); // convert it to local time structure
  strftime(buffer, 80, "%Y-%m-%d %H:%M:%S", timeinfo); // format it according to your preference
  //Serial.println(buffer);

  //String ret = String(ctime(&now));
  return buffer;

}


void getSDData(String *passData)
{
  //
  // this function will get the WiFiFile.txt from the SD card and read in the required values
  //
	String str, netid, pwd, deviceId, interval, hostname, sas, timedelay, timeoffset, gkey;

	File dataFile;
	Serial.println("In getSDData");

	// initialize sd card 
	// for nodemcu use begin() else use begin(4)
	Serial.print("Initializing SD card...");

	if (!SD.begin(SS)) {
		Serial.println("initialization failed!");
		return;
	}
	Serial.println("initialization done.");

  //printDirectory(SD.open("/") , 0);
	// open the file. note that only one file can be open at a time,
	// so you have to close this one before opening another.
  if(!SD.exists("WiFiFile.txt"))
  {
    Serial.println("File not found");
  }
	dataFile = SD.open("/WiFiFile.txt");
	int index = 0;

	if (dataFile)
	{
		Serial.println("data from sd card");
		display.clear();
		sendToDisplay(0, 0, "Data From Card");

		while (dataFile.available())
		{
			if (dataFile.find("SSID:"))
			{
				str = dataFile.readStringUntil('|');
				netid = str;
				Serial.println(netid);
				sendToDisplay(0, 1, netid);
			}
			if (dataFile.find("PASSWORD:"))
			{
				str = dataFile.readStringUntil('|');
				pwd = str;
				//Serial.println(pwd);
				//sendToDisplay(0,30, pwd);
			}
			if (dataFile.find("DEVICEID:"))
			{
				str = dataFile.readStringUntil('|');
				deviceId = str;
				Serial.println(deviceId);
				sendToDisplay(0,2, deviceId);
			}
			
			if (dataFile.find("HOSTNAME:"))
			{
				str = dataFile.readStringUntil('|');
				hostname = str;
				Serial.println(hostname);
				sendToDisplay(0, 3, hostname);
			}
			if (dataFile.find("SAS:"))
			{
				str = dataFile.readStringUntil('|');
				sas = str;
				//Serial.println(sas);
			}
			if (dataFile.find("DELAY:"))
			{
				str = dataFile.readStringUntil('|');
				timedelay = str;
				Serial.println(timedelay);
			}
      	if (dataFile.find("INTERVAL:"))
			{
				str = dataFile.readStringUntil('|');
				 interval = str;
				Serial.println(interval);
			}
       if (dataFile.find("TIMEOFFSET:"))
			{
				str = dataFile.readStringUntil('|');
				timeoffset = str;
				Serial.println(timeoffset);
			}
       if (dataFile.find("GOOGLEKEY:"))
			{
				str = dataFile.readStringUntil('|');
				gkey = str;
				//Serial.println(gkey);
			}
		}

		// close the file
		dataFile.close();
    delay(1000);
	}

	passData[0] = netid;
	passData[1] = pwd;
	passData[2] = deviceId;
	passData[3] = hostname;
	passData[4] = sas;
	passData[5] = timedelay;
	passData[6] = interval;
  passData[7] = timeoffset;

  // placeholder for lat lon
  passData[8] = "";
  passData[9] = "";

  passData[10] = gkey;

  delay(500);
	 
}


static void connectToWiFi()
{
  String netid, password;

  Serial.begin(115200);
  Serial.println();

  // get data from SD card
  getSDData(passData);
  netid = passData[0];
  password = passData[1];
  device_id = passData[2].c_str();
  
  host = passData[3].c_str();
  Serial.print("Host :");
  Serial.println(host);

  device_key = passData[4].c_str();
  
  // get delay between readings
  timedelay = passData[5].toInt();

  // display wifi connet on screen
  display.clear();
  sendToDisplay(0,0,"AZURE ESP8266 Weather");

  sendToDisplay(0,1,"Connecting to WIFI");
  sendToDisplay(0,2,"SSID : ");
  sendToDisplay(30,2,netid);

  Serial.print("Connecting to WIFI SSID ");
  Serial.println(netid);

  WiFi.mode(WIFI_STA);
  WiFi.begin((const char*)netid.c_str(), (const char*)password.c_str());
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connected, IP address: " );
  Serial.println( WiFi.localIP() );

  sendToDisplay(0,3,"Connected IP:");
  sendToDisplay(65,3,WiFi.localIP().toString());
   
}
