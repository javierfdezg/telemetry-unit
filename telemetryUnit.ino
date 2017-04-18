#include <SPI.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <avr/sleep.h>

SoftwareSerial mySerial(8, 7);
Adafruit_GPS GPS(&mySerial);

// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console
// Set to 'true' if you want to debug and listen to the raw GPS sentences
#define GPSECHO  true
/* set to true to only log to SD when GPS has a fix, for debugging, keep it false */
#define LOG_FIXONLY false

void useInterrupt(boolean); // Func prototype keeps Arduino 0023 happy

// Set the pins used
#define chipSelect 10
#define ledPin 13

////////////////////////////
// Lof File related stuff //
////////////////////////////
#define LOG_FILE_PREFIX "gpslog"
#define MAX_LOG_FILES 100
#define LOG_FILE_SUFFIX "csv"
char logFileName[13];

#define LOG_COLUMN_COUNT 10
char * log_col_names[LOG_COLUMN_COUNT] = {
  "longitude",
  "latitude",
  "altitude",
  "speed",
  "course",
  "date",
  "time",
  "satellites",
  "fix",
  "fixquality"
};

//////////////////////
// Log Rate Control //
//////////////////////
#define LOG_RATE 1000 // Log every second
unsigned long lastLog = 0; // Global var to keep of last time we logged

// blink out an error code
void error(uint8_t errno) {
  while(1) {
    uint8_t i;
    for (i=0; i<errno; i++) {
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
      delay(100);
    }
    for (i=errno; i<10; i++) {
      delay(200);
    }
  }
}

void setup() {
  // connect at 115200 so we can read the GPS fast enough and echo without dropping chars
  // also spit it out
  Serial.begin(115200);
  Serial.println("\r\nTelemetry unit");
  pinMode(ledPin, OUTPUT);

  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {     
    Serial.println("Card init. failed!");
    error(2);
  }
  char filename[15];
  strcpy(filename, "GPSLOG00.TXT");
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i/10;
    filename[7] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }

  logfile = SD.open(filename, FILE_WRITE);
  if( ! logfile ) {
    Serial.print("Couldnt create ");
    Serial.println(filename);
    error(3);
  }
  Serial.print("Writing to ");
  Serial.println(filename);

  // connect to the GPS at the desired rate
  GPS.begin(9600);

  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);   // 100 millihertz (once every 10 seconds), 1Hz or 5Hz update rate

  // Turn off updates on antenna status, if the firmware permits it
  GPS.sendCommand(PGCMD_NOANTENNA);

  // Force usage of interruptions for GPS read
  useInterrupt(true);

  Serial.println("Ready!");
}


// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
}

void useInterrupt(boolean v) {
  // v is always true
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  }
}

void loop() {
  // If it's been LOG_RATE milliseconds since the last log:
  if ((lastLog + LOG_RATE) <= millis())
  {
    // if a sentence is received, we can check the checksum, parse it...
    if (GPS.newNMEAreceived() && GPS.parse(GPS.lastNMEA())) {

      if (LOG_FIXONLY && !GPS.fix) {
        Serial.println("No Fix");
        return;
      }

      if (logGPSData()) // Log the GPS data
      {
        lastLog = millis(); // Update the lastLog variable
      }
      else // If we failed to log GPS
      { // Print an error, don't update lastLog
        Serial.println("Failed to log new GPS data.");
      }

    }
  }
}

byte logGPSData()
{
  if (GPSECHO)
  {
    Serial.print(GPS.longitudeDegrees, 4);
    Serial.print(',');
    Serial.print(GPS.latitudeDegrees, 4);
    Serial.print(',');
    Serial.print(GPS.altitude, 1);
    Serial.print(',');
    Serial.print(GPS.speed, 1);
    Serial.print(',');
    Serial.print(GPS.angle, 1);
    Serial.print(',');
    Serial.print(GPS.day, DEC); Serial.print('/');
    Serial.print(GPS.month, DEC); Serial.print("/20");
    Serial.print(GPS.year, DEC);
    Serial.print(',');
    Serial.print(GPS.hour, DEC); Serial.print(':');
    Serial.print(GPS.minute, DEC); Serial.print(':');
    Serial.print(GPS.seconds, DEC); Serial.print('.');
    Serial.print(GPS.milliseconds);
    Serial.print(',');
    Serial.print((int) GPS.satellites);
    Serial.print(',');
    Serial.print((int)GPS.fix);
    Serial.print(',');
    Serial.print((int)GPS.fixquality);
    Serial.println();
  }

  File logFile = SD.open(logFileName, FILE_WRITE);
  if (logFile)
  {
    logFile.print(GPS.longitudeDegrees, 4); logFile.print(GPS.lon);
    logFile.print(',');
    logFile.print(GPS.latitudeDegrees, 4); logFile.print(GPS.lat);
    logFile.print(',');
    logFile.print(GPS.altitude, 1);
    logFile.print(',');
    logFile.print(GPS.speed, 1);
    logFile.print(',');
    logFile.print(GPS.angle, 1);
    logFile.print(',');
    logFile.print(GPS.day, DEC); logFile.print('/');
    logFile.print(GPS.month, DEC); logFile.print("/20");
    logFile.print(GPS.year, DEC);
    logFile.print(',');
    logFile.print(GPS.hour, DEC); logFile.print(':');
    logFile.print(GPS.minute, DEC); logFile.print(':');
    logFile.print(GPS.seconds, DEC); logFile.print('.');
    logFile.print(GPS.milliseconds);
    logFile.print(',');
    logFile.print((int) GPS.satellites);
    logFile.print(',');
    logFile.print((int)GPS.fix);
    logFile.print(',');
    logFile.print((int)GPS.fixquality);
    logFile.println();
    logFile.close();

    return 1; // Return success
  }

  return 0; // If we failed to open the file, return fail
}

// printHeader() - prints our column names to the top of our log file
void printHeader()
{
  File logFile = SD.open(logFileName, FILE_WRITE); // Open the log file

  if (logFile) // If the log file opened, print our column names to the file
  {
    int i = 0;
    for (; i < LOG_COLUMN_COUNT; i++)
    {
      logFile.print(log_col_names[i]);
      if (i < LOG_COLUMN_COUNT - 1) // If it's anything but the last column
        logFile.print(','); // print a comma
      else // If it's the last column
        logFile.println(); // print a new line
    }
    logFile.close(); // close the file
  }
}

// updateFileName() - Looks through the log files already present on a card,
// and creates a new file with an incremented file index.
void updateFileName()
{
  int i = 0;
  for (; i < MAX_LOG_FILES; i++)
  {
    memset(logFileName, 0, strlen(logFileName)); // Clear logFileName string
    // Set logFileName to "gpslogXX.csv":
    sprintf(logFileName, "%s%d.%s", LOG_FILE_PREFIX, i, LOG_FILE_SUFFIX);
    if (!SD.exists(logFileName)) // If a file doesn't exist
    {
      break; // Break out of this loop. We found our index
    }
    else // Otherwise:
    {
      Serial.print(logFileName);
      Serial.println(" exists"); // Print a debug statement
    }
  }
  Serial.print("File name: ");
  Serial.println(logFileName); // Debug print the file name
}


/* End code */
