# Building Guard
### ESP32-Based Surveillance System with Tamper Detection and Telegram Alerts

Building Guard was designed and built to detect motion, physical disturbance, and possible attempts to cover a camera. An ESP32-C3 sensor controller is combined with an ESP32-CAM so that text notifications and captured images can be delivered through Telegram.

The hardware, firmware, and sensor interfaces were developed and integrated by **Abdulhamid Abdulkadir**. The complete prototype was tested and submitted to the University of Ilorin laboratory, where it is being used.

## Project Overview

Video surveillance is combined with sensor-based event detection to provide notifications when movement or possible camera interference is detected.

Three conditions are monitored:

- **Motion:** movement within the PIR sensor’s detection area.
- **Physical disturbance:** vibration associated with handling or interference.
- **Possible camera covering:** a sustained reduction in light measured near the camera.

When an event is qualified, a Telegram text notification is sent and an image capture is requested from the ESP32-CAM.

## Features

- PIR-based motion detection.
- Vibration sensing with debounce and event qualification.
- Adaptive light monitoring for possible camera-cover detection.
- Telegram text and photographic alerts.
- Local browser-based video streaming.
- Manual image capture through a web dashboard.
- Independent cooldowns for each sensor event type.
- UART communication between controllers.
- An additional digital camera-trigger connection.
- LED status and event indication.
- Wi-Fi reconnection logic in the current firmware.

## System Architecture

Processing is distributed between two controllers:

| Controller | Assigned functions |
|---|---|
| **ESP32-C3** | Sensor sampling, event qualification, status indication, Telegram text notification, and camera-trigger generation |
| **ESP32-CAM** | Image capture, Telegram image delivery, local video streaming, and web-dashboard hosting |

Event types are transmitted from the ESP32-C3 over UART. An additional capture request is provided through a separate digital connection.

Both controllers are connected to Wi-Fi for Telegram communication. The local dashboard is accessed through the ESP32-CAM’s assigned IP address.

## Hardware Components

| Component | Purpose |
|---|---|
| ESP32-C3 | Sensor processing and control |
| ESP32-CAM | Image capture and video streaming |
| PIR sensor | Motion detection |
| Digital vibration sensor | Physical-disturbance detection |
| LDR | Light measurement for possible obstruction detection |
| 10 kΩ resistor | LDR voltage divider |
| Status LED | Operating and event indication |
| Regulated 5 V supply | System power |
| Decoupling components | Supply filtering |

## Schematic Diagram

![Building Guard schematic diagram](docs/schematic/building-guard-schematic.png)

[View SVG diagram](docs/schematic/building-guard-schematic.svg) · [View original circuit drawing](docs/schematic/original-submitted-schematic.jpg)

## Pin Connections

The following GPIO assignments are used in the controller firmware. Logical GPIO numbers are listed rather than physical connector positions.

| Connection | ESP32-C3 pin | Connected device |
|---|---|---|
| Motion input | GPIO5 | PIR output |
| Vibration input | GPIO2 | Vibration sensor output |
| Light input | GPIO3 | LDR divider midpoint |
| UART transmit | GPIO6 | ESP32-CAM GPIO3 / RX |
| UART receive | GPIO7 | ESP32-CAM GPIO1 / TX |
| Camera trigger | GPIO10 | ESP32-CAM GPIO2 |
| Status indicator | GPIO0 | LED circuit |

A common ground connection is provided between both controllers and the sensors.

## Detection Logic

### Motion Detection

The PIR output is monitored by the ESP32-C3. When motion is detected and the motion cooldown has elapsed, a notification is sent and a camera capture is requested.

### Physical-Tamper Detection

Signal transitions from the vibration sensor are counted through an interrupt. Closely spaced transitions are rejected through debouncing.

An event is qualified when the required number of accepted transitions is recorded within the configured evaluation window. Detection behavior is also affected by sensor sensitivity and mechanical mounting.

### Possible Camera-Cover Detection

An initial light baseline is established during startup. Subsequent LDR readings are compared against this baseline.

A possible cover event is qualified when:

1. The baseline is above the minimum light threshold.
2. The measured reading is below a configured fraction of the baseline.
3. The reduction is sustained across the required consecutive samples.

Gradual changes in illumination are accommodated through baseline adjustment. Because illumination is measured at the LDR rather than within the camera image, detection is affected by sensor placement. Sudden room-light changes may be interpreted as covering, while lens-only obstruction may remain undetected if the LDR is exposed.

### Alert Cooldowns

An independent cooldown is applied to each controller event type to reduce repeated notifications. Automatic camera captures are also limited by a separate capture interval.

## Telegram Demonstration

A captured photograph and tamper notification from the working prototype are shown below.

![Telegram image and tamper alert](docs/images/telegram-tamper-alert.jpg)

A video demonstration and additional installation photographs will be added when laboratory access is available.

## Software and Libraries

| Firmware | Dependencies |
|---|---|
| ESP32-C3 controller | Arduino ESP32 core, WiFi, WiFiClientSecure, HardwareSerial, UniversalTelegramBot, ArduinoJson |
| ESP32-CAM | Arduino ESP32 core, esp_camera, esp_http_server, WiFi, WiFiClientSecure |

The firmware is organized into separate controller and camera sketches. Private credentials are stored outside the main source files.

## Configuration

A `secrets.example.h` file is provided in each firmware directory.

Before compilation:

1. A copy should be saved as `secrets.h` in the same directory.
2. The Wi-Fi SSID and password should be entered.
3. The Telegram bot token and destination chat ID should be entered.
4. Each sketch should be compiled and uploaded to its corresponding board.

The `secrets.h` files are excluded from version control and should be kept private.

Detailed wiring, commissioning, and troubleshooting procedures are provided in the [setup guide](docs/SETUP.md).

## Local Web Interface

The dashboard is accessed through the ESP32-CAM’s IP address after a network connection has been established.

| Address | Function |
|---|---|
| `http://CAMERA_IP/` | Dashboard |
| `http://CAMERA_IP/stream` | Video stream |
| `http://CAMERA_IP/capture` | Manual capture and Telegram delivery |

`CAMERA_IP` should be replaced with the address assigned to the camera board.

## Testing and Development

The original integrated prototype was tested before submission to the laboratory. Sensor interfacing, event processing, inter-controller communication, camera operation, and Telegram notification delivery were integrated into the completed system.

Subsequent firmware cleanup and Wi-Fi recovery improvements are included in this repository. A structured procedure for checking system behavior following modifications is provided in the [test plan](docs/TEST_PLAN.md).

## Current Limitations

- Wi-Fi and internet access are required for Telegram delivery.
- Events are not stored persistently for delivery after an outage.
- Separate photographs are not guaranteed for closely spaced events because a single pending camera capture is used.
- Light-based obstruction detection is affected by illumination and sensor placement.
- Authentication is not currently provided for the local dashboard.
- Server certificates are not currently verified during Telegram TLS connections.
- GSM fallback is not included.

## Planned Improvements

- Queued handling of closely spaced events.
- Persistent storage during network outages.
- TLS certificate verification.
- Dashboard authentication.
- Camera-health and supply monitoring.
- Measurement of notification latency and long-duration operation.

## Documentation

| Document | Contents |
|---|---|
| [Setup guide](docs/SETUP.md) | Wiring, configuration, commissioning, and troubleshooting |
| [Engineering notes](docs/ENGINEERING_NOTES.md) | Detection logic, timing parameters, and communication behavior |
| [Test plan](docs/TEST_PLAN.md) | Functional checks and result-recording procedures |
| [Evidence checklist](docs/EVIDENCE_CHECKLIST.md) | Available project evidence and planned additions |

## Repository Structure

```text
firmware/
├── controller/
│   ├── controller.ino
│   └── secrets.example.h
└── camera/
    ├── camera.ino
    └── secrets.example.h

docs/
├── images/
├── schematic/
├── SETUP.md
├── ENGINEERING_NOTES.md
├── TEST_PLAN.md
└── EVIDENCE_CHECKLIST.md
```

## Author

**Abdulhamid Abdulkadir**  
Electrical and Electronics Engineering  
University of Ilorin  

[GitHub: DevAbdul-web](https://github.com/DevAbdul-web)

## License

Terms of use are provided in [LICENSE](LICENSE).
