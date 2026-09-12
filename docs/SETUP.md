# Setup and commissioning

## Wiring reference

Logical GPIO numbers are used below; they are not physical connector positions.

| Source | Destination | Function |
| --- | --- | --- |
| PIR OUT | C3 GPIO5 | Motion input |
| Vibration DO/OUT | C3 GPIO2 | Disturbance input |
| LDR divider midpoint | C3 GPIO3 | ADC input |
| C3 GPIO6 TX | Camera GPIO3 / U0R RX | UART command |
| Camera GPIO1 / U0T TX | C3 GPIO7 RX | UART return connection |
| C3 GPIO10 | Camera GPIO2 | Capture trigger |
| C3 GPIO0 | Status LED circuit | Indicator; verify resistor and polarity |
| Regulated 5 V | Supported 5 V input of both boards | Board power |
| Common GND | Both boards and sensors | Signal reference |

LDR topology: 3.3 V → LDR → ADC midpoint → 10 kΩ → GND. Confirm that covering the LDR lowers the reading.

Verify exact sensor supply ratings, output levels, board boot-pin behavior and connector order before wiring. Avoid having the C3 and a USB serial programmer drive the camera UART simultaneously. The schematic's recommended decoupling is not a measurement of installed components.

## Toolchain

Use an Arduino-compatible ESP32 toolchain with the board target matching each physical board.

| Sketch | Dependencies |
| --- | --- |
| controller/controller.ino | Arduino ESP32 core, UniversalTelegramBot, ArduinoJson |
| camera/camera.ino | Arduino ESP32 core with esp_camera and esp_http_server |

Core/library versions and board settings have not been pinned or build-validated. Record them with the first successful builds.

## Configuration and upload

1. Preserve a recoverable copy of the installed firmware/source.
2. Copy secrets.example.h to secrets.h in each sketch directory.
3. Set WIFI_SSID, WIFI_PASSWORD, TELEGRAM_BOT_TOKEN and TELEGRAM_CHAT_ID. Use your own bot and destination chat; establish a conversation with the bot before testing.
4. Open controller.ino, select the actual C3 board/port, install dependencies and compile.
5. Record the compiler output and versions, then upload using the board's documented procedure.
6. Build camera.ino separately for the matching AI Thinker ESP32-CAM target and follow its programming procedure.
7. Restore normal boot connections and controller wiring. Monitor serial output at 115200 baud.
8. Keep the LDR exposed during startup calibration.
9. Obtain the camera IP from its online Telegram message or the router client list.

Never commit secrets.h. The chat ID selects a notification destination; it does not authenticate the web interface.

## Local interface

| URL | Purpose |
| --- | --- |
| http://CAMERA_IP/ | Dashboard |
| http://CAMERA_IP/stream | Video stream |
| http://CAMERA_IP/capture | Manual capture/send action |

Use the ESP32-CAM address, not the C3 address, from a reachable local network. The capture GET endpoint causes an action. No web authentication is implemented; do not expose the interface publicly.

## Troubleshooting

| Symptom | Checks |
| --- | --- |
| Resets during capture | Supply under load, ground, wiring and flash activity |
| Text but no photograph | Camera initialization/Wi-Fi, UART crossing, trigger and cooldown |
| Wrong caption | UART baud, simultaneous events and trigger-only behavior |
| No cover detection | ADC baseline, divider orientation, daylight floor and LDR placement |
| False alerts | Sensor mounting, vibration waveform and lighting changes |
| Dashboard unavailable | Camera IP, network reachability and server startup |
| Stream blocks capture | Shared server/camera resources; test concurrent requests |
| Compilation failure | Exact compiler error, selected board, core and library versions |

Complete the validation plan before replacing the deployed firmware.
