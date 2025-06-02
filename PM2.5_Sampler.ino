#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>
#include <RTClib.h>
#include <SoftwareSerial.h>
// Initialize BMP180
Adafruit_BMP085 bmp;
RTC_DS3231 rtc;

SoftwareSerial megaSerial(0, 1);
// Initialize DHT11
#define DHTPIN1 6
#define DHTPIN2 7
#define DHTTYPE DHT11
DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);

// Initialize flow rate 
#define ADP2100_ADDR 0x25

const int buttonBack = 2;
const int buttonUp = 3;
const int buttonDown = 4;
const int buttonEnter = 5;
const int relayPin = 8;  // Pin for the relay

// LCD and SD card setup
LiquidCrystal_I2C lcd(0x27, 20, 4);
const int chipSelect = 53;
File dataFile;

// Timing variables
unsigned long lastLCDUpdate = 0;
unsigned long lastSDLogTime = 0;
const unsigned long lcdUpdateInterval = 3000;  // 2 seconds
const unsigned long sdLogInterval = 60000;     // 60 seconds (1 minute)

float lastFlowRate = 0;
float totalVolume = 0;
unsigned long lastVolumeCalcTime = 0;

int menuIndex = 0;
const int maxMenuIndex = 2;
boolean inSubMenu = false;  // Added inSubMenu declaration
int downloadOption = 0; 

unsigned long timerDuration = 0;
unsigned long startTime = 0;
unsigned long machineStartTime = 0;
unsigned long totalMachineRunningTime = 0;
boolean timerActive = false;
boolean machineRunning = false;
int settingMode = 0;
int enterPressCount = 0;
unsigned long lastEnterPressTime = 0;

void setup() {
  // Start Serial
  Serial.begin(9600);
  megaSerial.begin(9600);
  Wire.begin();
  lcd.begin(20, 4);
  lcd.backlight();

  // Initialize buttons
  pinMode(buttonBack, INPUT_PULLUP);
  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buttonDown, INPUT_PULLUP);
  pinMode(buttonEnter, INPUT_PULLUP);
  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, LOW);

  // Initialize BMP180
  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP180 sensor, check wiring!");
    while (1) {}
  }

  // Initialize DHT11
  dht1.begin();
  dht2.begin();

  // Initialize SD Card
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    while (1) {}
  }

  // Create or open the data file
  dataFile = SD.open("datalog.csv", FILE_WRITE);
  if (!dataFile) {
    Serial.println("Error opening datalog.csv");
  } else {
    dataFile.println("Timestamp,Temperature(BMP180),Pressure,AT(DHT11),FT(DHT11),Flow Rate,Volume");
    dataFile.close();
  }

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }

  if (rtc.lostPower()) {
  Serial.println("RTC lost power, setting the time...");
  // Set the time to a specific date and time (e.g., March 8, 2025, 12:00:00)
  rtc.adjust(DateTime(2025, 3, 8, 17, 45, 0));
  // Alternatively, you can use the compile time with a different syntax:
  //rtc.adjust(DateTime(_DATE, __TIME_));
 // rtc.adjust(DateTime(F(_DATE), F(TIME_)));

}

  displayStartupScreen();
  displayHomeScreen();
}

void loop() {
  unsigned long currentMillis = millis();

  // Update LCD every 2 seconds
  if (currentMillis - lastLCDUpdate >= lcdUpdateInterval) {
    displayHomeScreen();
    lastLCDUpdate = currentMillis;
  }

  // Log data to SD card every minute
  if (currentMillis - lastSDLogTime >= sdLogInterval) {
    logDataToSD();
    lastSDLogTime = currentMillis;
  }

  // Handle button inputs (menu navigation)
  handleButtonInputs();
}

void displayStartupScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("GREENTECHINSTRUMENTS");
  lcd.setCursor(0, 1);
  lcd.print("  FINE PARTICULATE ");
  lcd.setCursor(0, 2);
  lcd.print("      SAMPLAR ");
  lcd.setCursor(0, 3);
  lcd.print("    PM 2.5 MFC ");
  delay(1000);  // Display for 1 second
}

void displayHomeScreen() {
  // Read BMP180 sensor
  float temperatureBMP = bmp.readTemperature();
  float pressure = bmp.readPressure() / 100.0F;

  // Read DHT11 sensor
  float temperatureDHT1 = dht1.readTemperature();
  float temperatureDHT2 = dht2.readTemperature();

  // Get current time
  unsigned long currentTime = millis();

  // Calculate Flow Rate
  Wire.beginTransmission(ADP2100_ADDR);
  Wire.write(0x36);
  Wire.write(0x1E);
  Wire.endTransmission();

  delay(10);  // Wait for the sensor to take a measurement

  // Request 6 bytes of data (2 for pressure, 1 for CRC, 2 for temperature, 1 for CRC)
  Wire.requestFrom(ADP2100_ADDR, 6);

  if (Wire.available() == 6) {
    uint16_t pressure2 = Wire.read() << 8 | Wire.read();  // Read 16-bit pressure
    uint8_t crc1 = Wire.read();  // Read CRC for pressure
    
    uint16_t temperature = Wire.read() << 8 | Wire.read();  // Read 16-bit temperature
    uint8_t crc2 = Wire.read();  // Read CRC for temperature
    
    float pressurePa = pressure2 / 50.0;   // Scale factor for pressure
    float temperatureC = temperature / 200.0;  // Scale factor for temperature
    float flowRate = sqrt(pressurePa) * 0.783;
    
    // Calculate volume since last measurement
    if (lastVolumeCalcTime > 0) {
      // Calculate elapsed time in minutes
      float elapsedTime = (currentTime - lastVolumeCalcTime) / 60000000.0;
      
      // Calculate volume: average flow rate * elapsed time
      float averageFlowRate = (flowRate + lastFlowRate) / 2.0;
      float volumeIncrement = averageFlowRate * elapsedTime;
      
      // Add to total volume
      totalVolume += volumeIncrement;
    }
    
    // Update last flow rate and time for next calculation
    lastFlowRate = flowRate;
    lastVolumeCalcTime = currentTime;

    // Display on LCD
    lcd.clear();
    lcd.setCursor(0, 3);
    lcd.print("BT:");
    lcd.print(temperatureBMP);
    lcd.print("C");

    lcd.setCursor(10, 3);
    lcd.print("BP:");
    lcd.print(pressure);
    lcd.print("h");

    lcd.setCursor(0, 2);
    lcd.print("AT:");
    lcd.print(temperatureDHT1);
    lcd.print("C");

    lcd.setCursor(10, 2);
    lcd.print("FT:");
    lcd.print(temperatureDHT2);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("FR:");
    lcd.print(flowRate);
    lcd.print("L/M");

    lcd.setCursor(11, 1);
    lcd.print(" V:");
    lcd.print(totalVolume, 2);  // Display with 2 decimal places
    lcd.print("L");

    lcd.setCursor(0, 0);
    lcd.print("GREENTECHINSTRUMENTS");
  }
}


void logDataToSD() {
  // Read sensor data
  float temperatureBMP = bmp.readTemperature();
  float pressure = bmp.readPressure() / 100.0F;
  float temperatureDHT1 = dht1.readTemperature();
  float temperatureDHT2 = dht2.readTemperature();

  // Calculate flow rate
  Wire.beginTransmission(ADP2100_ADDR);
  Wire.write(0x36);
  Wire.write(0x1E);
  Wire.endTransmission();
  delay(10);
  Wire.requestFrom(ADP2100_ADDR, 6);

  float flowRate = 0;
  if (Wire.available() == 6) {
    uint16_t pressure2 = Wire.read() << 8 | Wire.read();
    uint8_t crc1 = Wire.read();
    uint16_t temperature = Wire.read() << 8 | Wire.read();
    uint8_t crc2 = Wire.read();

    float pressurePa = pressure2 / 40.0;
    flowRate = pressurePa + 3.51;
    
    // Note: We're not calculating volume here because we're using the 
    // continuously updated totalVolume from displayHomeScreen()
  }

  // Get current timestamp
  DateTime now = rtc.now();
  String timestamp = String(now.year()) + "/";
  
  if (now.month() < 10) timestamp += "0";
  timestamp += String(now.month()) + "/";
  
  if (now.day() < 10) timestamp += "0";
  timestamp += String(now.day()) + " ";
  
  if (now.hour() < 10) timestamp += "0";
  timestamp += String(now.hour()) + ":";
  
  if (now.minute() < 10) timestamp += "0";
  timestamp += String(now.minute()) + ":";
  
  if (now.second() < 10) timestamp += "0";
  timestamp += String(now.second());

  // Create data string with fixed decimal places for consistency
  String dataString = timestamp + "," +
                    String(temperatureBMP, 2) + "," +
                    String(pressure, 2) + "," +
                    String(temperatureDHT1, 2) + "," +
                    String(temperatureDHT2, 2) + "," +
                    String(flowRate, 2) + "," +
                    String(totalVolume, 2);
  
  Serial.println("" + dataString);
  megaSerial.println(dataString);
  
  // Log to SD card
  dataFile = SD.open("datalog.csv", FILE_WRITE);
  if (dataFile) {
    dataFile.print(timestamp);
    dataFile.print(",");
    dataFile.print(temperatureBMP);
    dataFile.print(",");
    dataFile.print(pressure);
    dataFile.print(",");
    dataFile.print(temperatureDHT1);
    dataFile.print(",");
    dataFile.print(temperatureDHT2);
    dataFile.print(",");
    dataFile.print(flowRate);
    dataFile.print(",");
    dataFile.println(totalVolume);
    dataFile.close();
  } else {
    Serial.println("Error opening datalog.csv");
  }
}

void handleButtonInputs() {
  // Read button states
  if (digitalRead(buttonBack) == LOW) {
    delay(200);  // Debounce delay
    if (inSubMenu) {
      inSubMenu = false;
      displayMenu();
    } else {
      displayHomeScreen();
    }
  } else if (digitalRead(buttonUp) == LOW) {
    delay(200);  // Debounce delay
    if (!inSubMenu) {
      menuIndex = (menuIndex > 0) ? menuIndex - 1 : maxMenuIndex;
      displayMenu();
    }
  } else if (digitalRead(buttonDown) == LOW) {
    delay(200);  // Debounce delay
    if (!inSubMenu) {
      menuIndex = (menuIndex < maxMenuIndex) ? menuIndex + 1 : 0;
      displayMenu();
    }
  } else if (digitalRead(buttonEnter) == LOW) {
    delay(200);  // Debounce delay
    if (!inSubMenu) {
      inSubMenu = true;
      handleMenuSelection();
    }
  }
}

void displayMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Menu:");
  lcd.setCursor(1, 1);
  lcd.print(menuIndex == 0 ? ">" : " ");
  lcd.print("Time");
  lcd.setCursor(1, 2);
  lcd.print(menuIndex == 1 ? ">" : " ");
  lcd.print("Offset");
  lcd.setCursor(1, 3);
  lcd.print(menuIndex == 2 ? ">" : " ");
  lcd.print("Data Download");
}

void handleMenuSelection() {
  displaySubMenu(menuIndex);
  executeOption(menuIndex);
}

void displaySubMenu(int index) {
  lcd.clear();
  switch (index) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.print("Set Time");
      // Display time setting interface
      break;
    case 1:
      lcd.setCursor(0, 0);
      lcd.print("Set Timer Offset");
      // Display timer offset interface
      break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("Data Download");
      // Display download options
      break;
  }
}

void executeOption(int index) {
  switch (index) {
    case 0:
      option1();
      break;
    case 1:
      option2();
      break;
    case 2:
      option3();
      break;
  }
}

void option1() {
  DateTime now = rtc.now();

  // Display the date
  lcd.setCursor(0, 1);
  lcd.print("Date: ");
  lcd.print(now.year(), DEC);
  lcd.print('/');
  if (now.month() < 10) lcd.print('0');
  lcd.print(now.month(), DEC);
  lcd.print('/');
  if (now.day() < 10) lcd.print('0');
  lcd.print(now.day(), DEC);

  // Display the time
  lcd.setCursor(0, 2);
  lcd.print("Time: ");
  if (now.hour() < 10) lcd.print('0');
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  if (now.minute() < 10) lcd.print('0');
  lcd.print(now.minute(), DEC);
  lcd.print(':');
  if (now.second() < 10) lcd.print('0');
  lcd.print(now.second(), DEC);

  delay(1000);  // Delay for 1 second before returning to the menu
  lcd.setCursor(0, 0);
  lcd.print("GREENTECHINSTRUMENTS");
}

void option2() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Timer Offset ");

  lcd.setCursor(0, 2);
  lcd.print("Set Timer: 00:00:00");

  lcd.setCursor(0, 3);
  lcd.print("Setting: Seconds");

  while (inSubMenu) {
    if (digitalRead(buttonBack) == LOW) {
      delay(200);  // Debounce delay
      inSubMenu = false;
      displayMenu();
      return;
    }

    // Handle up button to increase timer value
    if (digitalRead(buttonUp) == LOW) {
      if (settingMode == 0) { // seconds
        timerDuration += 1;
      } else if (settingMode == 1) { // minutes
        timerDuration += 60;
      } else if (settingMode == 2) { // hours
        timerDuration += 3600;
      }

      int hours = timerDuration / 3600;
      int minutes = (timerDuration % 3600) / 60;
      int seconds = timerDuration % 60;

      lcd.setCursor(0, 2);
      lcd.print("Set Timer: ");
      if (hours < 10) lcd.print("0");
      lcd.print(hours);
      lcd.print(":");
      if (minutes < 10) lcd.print("0");
      lcd.print(minutes);
      lcd.print(":");
      if (seconds < 10) lcd.print("0");
      lcd.print(seconds);

      delay(200); // Button debounce delay
    }

    // Handle enter button to switch between seconds, minutes, and hours
    if (digitalRead(buttonEnter) == LOW) {
      unsigned long currentTime = millis();
      if (currentTime - lastEnterPressTime < 500) {
        enterPressCount++;
      } else {
        enterPressCount = 1;
      }
      lastEnterPressTime = currentTime;

      if (enterPressCount == 2) {
        settingMode = (settingMode + 1) % 3;

        lcd.setCursor(0, 3);
        lcd.print("Setting: ");
        if (settingMode == 0) lcd.print("Seconds");
        if (settingMode == 1) lcd.print("Minutes");
        if (settingMode == 2) lcd.print("Hours  ");
      } else if (enterPressCount == 3) {
        if (!timerActive) {
          timerActive = true;
          startTime = millis() / 1000;
          if (!machineRunning) {
            machineRunning = true;
            machineStartTime = millis() / 1000;
          }
        } else {
          timerActive = false;
          digitalWrite(relayPin, LOW); // Turn off relay when timer is stopped
        }

        lcd.setCursor(0, 1);
        lcd.print("Timer: ");
        lcd.print(timerActive ? "Running " : "Stopped ");
      }
      delay(200); // Button debounce delay
    }

    // Update the timer if active
    if (timerActive) {
      unsigned long elapsedTime = (millis() / 1000) - startTime;
      int remainingTime = timerDuration - elapsedTime;

      if (remainingTime <= 0) {
        if (digitalRead(relayPin) == LOW) {
          digitalWrite(relayPin, HIGH); // Turn on the relay if it's off
        } else {
          digitalWrite(relayPin, LOW); // Turn off the relay if it's on
          totalMachineRunningTime += (millis() / 1000) - machineStartTime;
          machineRunning = false;
        }
        timerActive = false;

        lcd.setCursor(0, 1);
        lcd.print("Timer: Stopped ");
      } else {
        int hours = remainingTime / 3600;
        int minutes = (remainingTime % 3600) / 60;
        int seconds = remainingTime % 60;

        lcd.setCursor(0, 1);
        lcd.print("Running: ");
        if (hours < 10) lcd.print("0");
        lcd.print(hours);
        if (minutes < 10) lcd.print(":0");
        else lcd.print(":");
        lcd.print(minutes);
        if (seconds < 10) lcd.print(":0");
        else lcd.print(":");
        lcd.print(seconds);
      }
    }

    // Update machine running time based on relay state
    if (digitalRead(relayPin) == LOW) {
      if (!machineRunning) {
        machineRunning = true;
        machineStartTime = millis() / 1000;
      }

      unsigned long machineElapsedTime = totalMachineRunningTime + ((millis() / 1000) - machineStartTime);

      int hours = machineElapsedTime / 3600;
      int minutes = (machineElapsedTime % 3600) / 60;
      int seconds = machineElapsedTime % 60;

      lcd.setCursor(0, 0);
      lcd.print("TT: ");
      if (hours < 10) lcd.print("0");
      lcd.print(hours);
      if (minutes < 10) lcd.print(":0");
      else lcd.print(":");
      lcd.print(minutes);
      if (seconds < 10) lcd.print(":0");
      else lcd.print(":");
      lcd.print(seconds);
    } else {
      if (machineRunning) {
        totalMachineRunningTime += (millis() / 1000) - machineStartTime;
        machineRunning = false;
      }
    }
  }
}

void option3() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Download Data?");
  lcd.setCursor(0, 1);
  lcd.print("Yes");
  lcd.setCursor(0, 2);
  lcd.print("No");

  while (inSubMenu) {
    if (digitalRead(buttonBack) == LOW) {
      delay(200);  // Debounce delay
      inSubMenu = false;
      displayMenu();
      return;
    }

    // Handle up button to toggle between Yes and No
    if (digitalRead(buttonUp) == LOW) {
      downloadOption = !downloadOption;
      lcd.setCursor(0, 1);
      lcd.print(downloadOption == 1 ? ">Yes" : " Yes");
      lcd.setCursor(0, 2);
      lcd.print(downloadOption == 0 ? ">No" : " No");
      delay(200); // Button debounce delay
    }

    // Handle enter button to confirm selection
    if (digitalRead(buttonEnter) == LOW) {
      if (downloadOption == 1) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Downloading...");
        delay(2000); // Simulate download process
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Download Complete");
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Download Cancelled");
      }
      delay(2000);
      inSubMenu = false;
      displayMenu();
      return;
    }
  }
}
