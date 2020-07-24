// GPS Lib
#include <TinyGPS++.h>
// Software Serial Lib
#include <SoftwareSerial.h>
// Arduino Lib
#include <Arduino.h>
// SPI Lib
#include <SPI.h>
// SD Lib
#include <SD.h>
// WiFi Lib (to turn off WiFi)
#include <ESP8266WiFi.h>
// pins and baud of GPS
static const int RXPin = 4, TXPin = 5;
static const uint32_t GPSBaud = 9600;
// The TinyGPS++ object
TinyGPSPlus gps;
// SS the GPS pins
SoftwareSerial ss(RXPin, TXPin);
//SD Reader CS pin
const int chipSelect = 15;
// Var for file name if one is not set later
String logname = "no-name.gpx";
// var File is dataFile
File dataFile;
// vars for ISO8601 format
String encodedDateTime;
String gpx_year;
String gpx_month;
String gpx_day;
String gpx_hour;
String gpx_minute;
String gpx_second;
//var for led pattern
String event ="";
// var for checking if GPS is working
int delay_sucks = 0;
int gps_device = 0;
// timer for GPS logging (debug can't be used with GPS)
unsigned long previousMillis = 0;
const long interval = 1000; // 1000ms = 1 second
// debug level
// 1 is GPS simple info | Standard
// 2 is 1+ Speed and HDOP and Sats | Nosiy
// 3 is 1+2 + TinyGPS++ debug | Deafering
int debug_level = 3;

// Start of setup
void setup()
{
  // Turn off WIFI, double to make sure.
  WiFi.persistent( false );
  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  delay(1);
  // read Serial Speed
  Serial.begin(115200);
  while (!Serial)
  {
    // wait for Serial to come up
    delay(1000);
  }
  Serial.println("");
  // read speed of GPS
  ss.begin(GPSBaud);
  // setup LEDs
  pinMode(2, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  // ensure LEDs off
  digitalWrite(2, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("Debug Level:");
  Serial.println(debug_level);

  Serial.println("Initializing SD card...");
  check_SD(); // checks that SD is working, will keep retrying untill true
  Serial.println("card initialized.");

  Serial.println("Initializing GPS...");
  check_GPS(); // checks that GPS is working, will keep retrying untill true
  Serial.println("GPS initialized.");

  Serial.print("logging to File: ");
  Serial.println(logname); // the name of the log file.
  // makes the log file on the SD card, adds GPX heading
  if (!SD.exists(logname)) {
    dataFile = SD.open(logname, FILE_WRITE);
    dataFile.print(F(
                     "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
                     "<gpx version=\"1.1\" creator=\"MrBlueSea\" xmlns=\"http://www.topografix.com/GPX/1/1\" \r\n"
                     "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\r\n"
                     "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd\">\r\n"
                     "\t<trk>\r\n<trkseg>\r\n"));  //heading of gpx file
    dataFile.print(F("</trkseg>\r\n</trk>\r\n</gpx>\r\n"));
    dataFile.close();
  }
}

void check_SD()
{
  // keep checking the SD reader for valid SD card/format
  while (!SD.begin(chipSelect))
  {
    led_pattern("SD Error"); // sends fail code to LEDs
    Serial.println("SD Card Failed, Will Retry...");
  }
}

void check_GPS()
{
  while (gps_device == 0) // while the GPS device is not "working" it will keep retrying
  {
    if (ss.available() > 0) // ss need to be avaiable
    {
      gps.encode(ss.read()); // encode any GPS data for us to check with
      if (gps.satellites.isUpdated()) // check the GPS Satellites are up to date
      {
        if (gps.satellites.value() < 3) // get at least 3 or fail
        {
          Serial.println("Not Enough Satellites");
          Serial.print(F("SATELLITES Fix Age="));
          Serial.print(gps.satellites.age());
          Serial.print(F("ms Value="));
          Serial.println(gps.satellites.value());
          led_pattern("GPS Error");
        }
        else {
          Serial.print("satellites = ");
          Serial.println(gps.satellites.value());
          //makes the log name the gps DateTime, we need GPS to do this this is why it's not done before
          logname = String(gps.date.day())+String(gps.date.month())+String(gps.date.year())+String(".gpx");
          gps_device = 1; // everything is good
        }
      }
    }
  }
}


// LED patterns to debug without Serial
void led_pattern(String event)
{
  Serial.print("Debug of event: ");
  Serial.print(event);
  Serial.println("");
  if (event == "SD Error")
  {// LED near USB port (LED_BUILTIN/GPIO16) will flash if SD Error
    digitalWrite(LED_BUILTIN, HIGH);
    digitalWrite(2, LOW);
    delay(1000);
    digitalWrite(2, HIGH);
    delay(1000);
  }
  if (event == "GPS Error")
  {// LED near WiFi antenna (GPIO2/D4) will flash if GPS error
    digitalWrite(2, HIGH);
    digitalWrite(LED_BUILTIN, LOW);
    delay(1000);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(1000);
  }
  if (event == "GPS Lost")
  {// Both LED Alter flash if GPS lost

    do {
      if ( (delay_sucks % 2) == 0)
      {
        digitalWrite(2, HIGH);
        digitalWrite(LED_BUILTIN, LOW);
      }
      else
      {
        digitalWrite(2, LOW);
        digitalWrite(LED_BUILTIN, HIGH);
      }
    } while(delay_sucks++ < 50000);
  }
}


void debug()
{
  // Debug info
  if (debug_level >= 1) {
    Serial.println("--Debug START--");
    // Log's name
    Serial.println("LOG:");
    Serial.println(logname);
    // lattitude
    Serial.println("LAT:");
    Serial.println(gps.location.lat(),7);
    // Longitude
    Serial.println("LNG:");
    Serial.println(gps.location.lng(),7);
    // Encoded DateTime
    Serial.println("DateTime:");
    Serial.println(encodedDateTime);
    // Altitude
    Serial.println("Alt: ");
    Serial.println(gps.altitude.meters());
  }
  if ((debug_level > 1) && (debug_level <= 2) || (debug_level == 3)) {
    // Speed
    Serial.println("Speed: ");
    Serial.println(gps.speed.mph());
    // Horizontal dilution of precision
    Serial.println("hdop: ");
    Serial.println(gps.hdop.hdop());
    // Number of satellites
    Serial.println("Sats: ");
    Serial.println(gps.satellites.value());
  }
  if (debug_level >= 3) {
    // More Debug info; see TinyGPS++ Docs
    Serial.println("--Verboose Here--");
    Serial.print(F("DIAGS      Chars="));
    // the total number of characters received by the object
    Serial.print(gps.charsProcessed());
    // the number of $GPRMC or $GPGGA sentences that had a fix
    Serial.print(F(" Sentences-with-Fix="));
    Serial.print(gps.sentencesWithFix());
    // the number of sentences of all types that failed the checksum test
    Serial.print(F(" Failed-checksum="));
    Serial.print(gps.failedChecksum());
    // the number of sentences of all types that passed the checksum test
    Serial.print(F(" Passed-checksum="));
    Serial.println(gps.passedChecksum());
  }
  if (debug_level >= 1) {
    Serial.println("--Debug END--");
  }
}

void loop()
{
  // We need SS to read GPS
  if (ss.available() > 0)
  {
    gps.encode(ss.read()); // Readu the GPS data
    logger(); // Run the logger
  }
}

void logger()
{
  unsigned long currentMillis = millis(); // delay start
  // Turn Both LEDs off
  digitalWrite(2, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);

  if (currentMillis - previousMillis >= interval) { // Check if delay has finished
    debug();
    // save the reset delay timer
    previousMillis = currentMillis;
    if (int(gps.hdop.hdop()) < 5 && int(gps.location.lat()) != 0 && int(gps.location.lng()) != 0) // make sure GPS data is accrucate
    {
      Serial.println("Logging to File");
      // encoded date time maker/fixer
      // if the month/day/hour/minute/second is less 10, preppend 0 (01/02/03/04/05/06/07/09 ISO8601 formatting)
      // seems as this is used in the GPX file they are assigned to gpx_* relative vars

      gpx_year = gps.date.year(); // No GPS year fix required never less than 10

      if (gps.date.month() < 10) {
        gpx_month = String("0") + String(gps.date.month());
      } else {
        gpx_month = gps.date.month();
      }
      if (gps.date.day() < 10) {
        gpx_day = String("0") + String(gps.date.day());
      } else {
        gpx_day = gps.date.day();
      }
      if (gps.time.hour() < 10) {
        gpx_hour = String("0") + String(gps.time.hour());
      } else {
        gpx_hour = gps.time.hour();
      }
      if (gps.time.minute() < 10) {
        gpx_minute = String("0") + String(gps.time.minute());
      } else {
        gpx_minute = gps.time.minute();
      }
      if (gps.time.second() < 10) {
        gpx_second = String("0") + String(gps.time.second());
      } else {
        gpx_second = gps.time.second();
      }

      // put the vars all together (ISO8601 format of course)

      encodedDateTime =
        String(gpx_year)
        + String("-")
        + String(gpx_month)
        + String("-")
        + String(gpx_day)
        + String("T")
        + String(gpx_hour)
        + String(":")
        + String(gpx_minute)
        + String(":")
        + String(gpx_second)
        + String("Z"); // Z is UTC aka Zulu time

      // Turn LEDs On (feedback for no serial monitor)
      digitalWrite(2, LOW);
      digitalWrite(LED_BUILTIN, LOW);

      // get the log file ready for writing
      dataFile = SD.open(logname, FILE_WRITE);
      // get the log file length, this is not nothing as we added the GPX heading
      unsigned long filesize = dataFile.size();
      filesize -= 27; // backup 27 to not overwrite the GPX ending
      dataFile.seek(filesize); // go to the correct writing section
      // log the lattitude
      dataFile.print(F("<trkpt lat=\""));
      dataFile.print(gps.location.lat(), 7);
      // log the Longitude
      dataFile.print(F("\" lon=\""));
      dataFile.print(gps.location.lng(), 7);
      dataFile.println(F("\">"));
      // log the encoded time
      dataFile.print(F("<time>"));
      dataFile.print(encodedDateTime);
      dataFile.println(F("</time>"));
      // log the elevation
      dataFile.print(F("<ele>"));
      dataFile.print(gps.altitude.meters());
      dataFile.print(F("</ele>\r\n<hdop>"));
      // log the hdop (Horizontal dilution of precision)
      dataFile.print(gps.hdop.hdop(), 3);
      dataFile.println(F("</hdop>\r\n</trkpt>"));
      // end the entry
      dataFile.print(F("</trkseg>\r\n</trk>\r\n</gpx>\r\n"));
      // close the log
      dataFile.close();

    }
    else
    {
      led_pattern("GPS Lost");
      Serial.println("!!!!! -NOT Logging to File- !!!!!");
      Serial.println("Why?");
      if (int(gps.hdop.hdop()) >= 5) {
        Serial.println("HDOP is over 5");
        Serial.println("HDOP: " + String(gps.hdop.hdop()));
      }
      if (int(gps.location.lat()) == 0) {
        Serial.println("LAT is 0");
        Serial.println(gps.location.lat());
      }
      if (int(gps.location.lng()) == 0) {
        Serial.println("LNG is 0");
        Serial.println(gps.location.lng());
      }
    }
  }
}
