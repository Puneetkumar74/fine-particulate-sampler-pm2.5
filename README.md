# Fine Particulate Sampler PM 2.5 MFC

## Overview
An Arduino-based environmental monitoring system designed for fine particulate matter sampling with PM 2.5 measurement capabilities. This project integrates multiple sensors to monitor temperature, pressure, humidity, and flow rate with real-time data logging to SD card.

## Features
- **Multi-sensor Integration**: BMP180 (temperature/pressure), DHT11 (temperature/humidity), ADP2100 (flow rate)
- **Real-time Data Display**: 20x4 I2C LCD with live sensor readings
- **Data Logging**: Automatic CSV logging to SD card with timestamps
- **Timer Functionality**: Configurable timer with relay control
- **Menu System**: Interactive menu for configuration and data management
- **Volume Calculation**: Continuous flow rate monitoring and volume calculation

## Hardware Components

### Required Components
- Arduino Mega 2560
- BMP180 Barometric Pressure Sensor
- 2x DHT11 Temperature/Humidity Sensors
- ADP2100 Flow Rate Sensor
- 20x4 I2C LCD Display
- DS3231 Real-Time Clock Module
- SD Card Module
- Relay Module
- 4x Push Buttons (Back, Up, Down, Enter)
- Connecting wires and breadboard

### Pin Configuration
```
DHT11 Sensors: Pins 6, 7
Buttons: Pins 2-5 (Back, Up, Down, Enter)
Relay: Pin 8
SD Card: Pin 53 (CS)
I2C Devices: SDA/SCL (20/21 on Mega)
- BMP180: 0x77 (default)
- LCD: 0x27
- ADP2100: 0x25
- DS3231: 0x68 (default)
```

## Software Dependencies

### Required Libraries
Install these libraries through Arduino IDE Library Manager:
```
- Wire (Built-in)
- Adafruit_BMP085
- DHT sensor library
- LiquidCrystal_I2C
- SD (Built-in)
- RTClib
- SoftwareSerial (Built-in)
```

## Installation

1. **Clone the Repository**
   ```bash
   git clone https://github.com/yourusername/fine-particulate-sampler-pm25.git
   cd fine-particulate-sampler-pm25
   ```

2. **Install Libraries**
   - Open Arduino IDE
   - Go to Sketch → Include Library → Manage Libraries
   - Install all required libraries listed above

3. **Hardware Setup**
   - Connect components according to the pin configuration
   - Ensure proper power supply for all modules
   - Insert formatted SD card into the SD module

4. **Upload Code**
   - Open `PM25_Sampler.ino` in Arduino IDE
   - Select Arduino Mega 2560 as board
   - Select appropriate COM port
   - Upload the code

## Usage

### Initial Setup
1. Power on the device
2. The startup screen will display "GREENTECHINSTRUMENTS FINE PARTICULATE SAMPLAR PM 2.5 MFC"
3. The system will automatically start displaying sensor readings

### Main Display
The home screen shows:
- **BT**: BMP180 Temperature (°C)
- **BP**: Barometric Pressure (hPa)
- **AT**: Ambient Temperature from DHT11 (°C)
- **FT**: Filter Temperature from DHT11 (°C)
- **FR**: Flow Rate (L/M)
- **V**: Total Volume (L)

### Menu Navigation
- **Back Button**: Return to home screen or exit submenu
- **Up/Down Buttons**: Navigate menu options
- **Enter Button**: Select menu item or confirm settings

### Menu Options
1. **Time**: View current date and time from RTC
2. **Offset**: Set timer duration and control relay
3. **Data Download**: Export logged data

### Data Logging
- Data is automatically logged to `datalog.csv` on SD card every minute
- CSV format: `Timestamp,Temperature(BMP180),Pressure,AT(DHT11),FT(DHT11),Flow Rate,Volume`
- Serial output available for real-time monitoring

## File Structure
```
fine-particulate-sampler-pm25/
├── PM25_Sampler.ino          # Main Arduino sketch
├── README.md                 # This file
├── LICENSE                   # License file
├── docs/
│   ├── circuit-diagram.png   # Wiring diagram
│   ├── user-manual.md        # Detailed user manual
│   └── troubleshooting.md    # Common issues and solutions
├── examples/
│   └── sample-data.csv       # Example output data
└── images/
    ├── device-photo.jpg      # Photo of assembled device
    └── lcd-display.jpg       # LCD display examples
```

## Configuration

### RTC Time Setting
The system automatically sets the RTC time if power is lost. Modify the time in the setup function:
```cpp
rtc.adjust(DateTime(2025, 3, 8, 17, 45, 0)); // Year, Month, Day, Hour, Minute, Second
```

### Sensor Calibration
Flow rate calculation can be adjusted by modifying the scaling factors:
```cpp
float flowRate = sqrt(pressurePa) * 0.783; // Adjust multiplier as needed
```

## Troubleshooting

### Common Issues
1. **SD Card Not Detected**
   - Check SD card formatting (FAT32)
   - Verify wiring connections
   - Ensure SD card is properly inserted

2. **Sensor Reading Errors**
   - Check I2C connections
   - Verify sensor addresses
   - Ensure proper power supply

3. **LCD Display Issues**
   - Verify I2C address (default 0x27)
   - Check contrast adjustment
   - Confirm wiring connections

## Contributing
Contributions are welcome! Please feel free to submit a Pull Request. For major changes, please open an issue first to discuss what you would like to change.

### Development Guidelines
- Follow Arduino coding standards
- Comment your code thoroughly
- Test on actual hardware before submitting
- Update documentation as needed

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Acknowledgments
- Adafruit for sensor libraries
- Arduino community for inspiration and support
- Contributors who help improve this project

## Contact
- Project Maintainer: Puneet
- Email: puneetchandilya@gmail.com]
- GitHub: @puneetkumar74

## Version History
- **v1.0.0** - Initial release with basic functionality
- **v1.1.0** - Added volume calculation and improved data logging
- **v1.2.0** - Enhanced menu system and timer functionality

---
**Note**: This project is designed for educational and research purposes. Ensure proper safety measures when working with electronic components.
