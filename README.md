# matter_pwm_control
ESP32-C6 Matter PWM Controller , To control the pwm of the gpio 
This project implements a Matter protocol smart device that provides three independent PWM-controlled dimmable lights. Each channel operates at 5kHz with 8-bit resolution (0-255), making it perfect for LED dimming applications. The device appears as three separate dimmable lights in your Matter ecosystem, allowing independent control of each channel through Google Home, Apple Home, or any Matter-compatible controller.
Built on the ESP32-C6 with ESP-Matter SDK, this controller demonstrates professional-grade Matter device development with clean architecture and robust PWM control.

Connect Hardware

1. Connect your ESP32-C6 board to your computer via USB
2. Identify the serial port (usually `/dev/ttyUSB0` on Linux or `/dev/ttyACM0`)
3. Connect your LEDs to GPIO 4, 5, and 6


Software Requirements

1. **ESP-IDF v5.0 or later**
   - Install from: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/

2. **ESP-Matter SDK**
   - Install from: https://github.com/espressif/esp-matter

3. **Python 3.8+** (included with ESP-IDF)

4. **Git** (for cloning repositories)
5. ### Environment Setup

Make sure you have ESP-IDF and ESP-Matter properly installed and the environment variables set:

```bash
# Source ESP-IDF
. $HOME/esp/esp-idf/export.sh

# Source ESP-Matter
. $HOME/esp/esp-matter/export.sh
```
### Step 1: Clone or Navigate to Project

If this is a standalone project:
```bash
git clone <your-repo-url>
cd pwm_matter
```

If already in ESP-Matter examples directory:
```bash
cd $HOME/esp/esp-matter/examples/pwm_matter
```

### Step 2: Set Up Environment

```bash
# Load ESP-IDF tools and environment
. $HOME/esp/esp-idf/export.sh

# Load ESP-Matter environment
. $HOME/esp/esp-matter/export.sh
```

### Step 3: Configure the Project

```bash
# Set target to ESP32-C6
idf.py set-target esp32c6

# (Optional) Customize configuration
idf.py menuconfig
```

### Step 4: Build the Firmware

```bash
idf.py build
```


This will compile the project. You should see:
```
Project build complete. To flash, run:
idf.py flash
```

### Step 5: Connect Hardware

1. Connect your ESP32-C6 board to your computer via USB
2. Identify the serial port (usually `/dev/ttyUSB0` on Linux or `/dev/ttyACM0`)
3. Connect your LEDs to GPIO 4, 5, and 6
4. (Optional) Connect WS2812 RGB LED to GPIO 8

### Step 6: Flash the Firmware

```bash
# Flash and open serial monitor in one command
idf.py -p /dev/ttyUSB0 flash monitor
```

Or separately:
```bash
# Flash only
idf.py -p /dev/ttyUSB0 flash

# Monitor only
idf.py -p /dev/ttyUSB0 monitor
```

**Note**: Replace `/dev/ttyUSB0` with your actual port (check with `ls /dev/tty*`)

### Step 7: Commission the Device

After flashing, the serial monitor will display:

```
╔═══════════════════════════════════════════════════╗
║ ESP32-C6 Matter 3-Channel PWM Controller          ║
╠═══════════════════════════════════════════════════╣
║ PWM Configuration:                                ║
║   Channel 0 -> GPIO 4  (Dimmable Light 1)         ║
║   Channel 1 -> GPIO 5  (Dimmable Light 2)         ║
║   Channel 2 -> GPIO 6  (Dimmable Light 3)         ║
╚═══════════════════════════════════════════════════╝

Manual pairing code: XXXXX-XXXXX-XXXX
QR Code: MT:XXXXXXXXXXXXXXXXXX
```

**Commissioning Steps:**

1. **Open your Matter app:**
   - Google Home (Android)
   - Apple Home (iOS)
   - Any Matter-compatible app

2. **Add new device:**
   - Tap "Add Device" or "+"
   - Select "Matter" device
   
3. **Scan QR code:**
   - Use the QR code string from serial monitor
   - Format: `MT:XXXXXXXXXXXXXXXXXX`
   - Or manually enter the pairing code

4. **Wait for commissioning:**
   - Device will connect to WiFi
   - Three dimmable lights will appear
   - Each represents one PWM channel

5. **Name your devices:**
   - "PWM Light 1" → GPIO 4

### Step 8: Control Your Lights
   - **Via App:**
- Each light has ON/OFF toggle
- Slider for brightness (0-100%)
- Changes reflect in real-time



##  Monitoring and Debugging

### View Serial Output

```bash
idf.py monitor
```

You'll see detailed logs:
```
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 PWM Channel 0 (GPIO 4): POWER ON
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
PWM Channel 0 (GPIO 4): duty = 127/255 (50.0%)
```

### Exit Monitor

Press `Ctrl+]` to exit the monitor.

### Erase Flash (if needed)

```bash
idf.py -p /dev/ttyUSB0 erase-flash
```


#  Project Structure

```
pwm_matter/
├── main/
│   ├── app_main.cpp           - Main application and Matter setup
│   ├── app_driver.cpp         - PWM and RGB LED driver implementation
│   ├── app_driver.h           - Driver interface definitions
│   ├── app_priv.h             - Private definitions
│   ├── idf_component.yml      - Component dependencies
│   └── CMakeLists.txt         - Main component build config
├── CMakeLists.txt             - Top-level build configuration
├── partitions.csv             - Partition table
├── sdkconfig.defaults         - Default configuration
├── sdkconfig.defaults.esp32c6 - ESP32-C6 specific config
└── README.md                  - This file
```




## Technical Specifications

| Specification | Value |
|--------------|-------|
| **Target MCU** | ESP32-C6 |
| **Development Board** | ESP32-C6-DevKitC-1 |
| **PWM Channels** | 3 (GPIO 4, 5, 6) |
| **PWM Timer** | LEDC Timer 0 |
| **PWM Resolution** | 8-bit (0-255) |
| **PWM Frequency** | 5 kHz |
| **PWM Driver** | ESP-IDF LEDC |
| **RGB LED** | WS2812B via RMT |
| **RGB LED GPIO** | GPIO 8 |
| **Protocol** | Matter over WiFi |
| **Commissioning** | BLE + WiFi |
| **Matter Device Type** | Dimmable Light (x3) |

## Dependencies

This project uses the following components:

- **ESP-IDF v5.0+** - Core framework
- **ESP-Matter SDK** - Matter protocol stack
- **LEDC Driver** - PWM control
- **RMT Driver** - WS2812 RGB LED control
- **NVS Flash** - Non-volatile storage
- **WiFi** - Network connectivity
- **BLE** - Commissioning

## Customization

### Change PWM Frequency

Edit `app_driver.cpp`:
```cpp
#define PWM_FREQUENCY    5000   // Change to your desired frequency
```

### Change GPIO Pins

Edit `app_driver.h`:
```cpp
#define PWM_CHANNEL_0_GPIO 4  // Change to your pin
#define PWM_CHANNEL_1_GPIO 5
#define PWM_CHANNEL_2_GPIO 6
#define RGB_LED_GPIO 8
```

### Adjust Number of Channels

Edit `app_driver.h`:
```cpp
#define PWM_NUM_CHANNELS   3  // Increase or decrease
```

Then update the arrays in `app_driver.cpp` and loop in `app_main.cpp`.

##  Troubleshooting

### Build Fails
```bash
# Clean and rebuild
idf.py fullclean
idf.py build
```

### Cannot Flash
```bash
# Check port
ls /dev/tty*

# Use correct port
idf.py -p /dev/ttyACM0 flash
```

### Commissioning Fails
1. Ensure WiFi is 2.4GHz (Matter doesn't support 5GHz)
2. Check that BLE is enabled on your phone
3. Keep phone close to ESP32-C6 during commissioning
4. Restart the device and try again

### Device Not Responding
```bash
# Erase flash and reflash
idf.py erase-flash
idf.py flash monitor
```


