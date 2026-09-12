# Engineering Notes

## Ownership and deployment

Abdulhamid Abdulkadir designed and built the complete Building Guard system. The completed physical unit was submitted to and deployed in a University of Ilorin laboratory.

## Verified submitted implementation

- Motion detection using a PIR sensor
- Physical-tamper detection using a vibration sensor
- Lens-cover detection using an LDR
- ESP32-C3 sensor controller
- ESP32-CAM image capture
- Telegram text and photographic alerts
- Local network live stream
- Web-dashboard manual capture
- Per-event cooldowns
- UART event message plus digital backup trigger

## Refactor status

The firmware in this repository is a cleaned revision derived from the working submitted code. It adds safer credential organization, clearer naming, periodic Wi-Fi recovery, smaller functions, and documentation.

Because the physical unit is held in the laboratory, the refactored revision has not yet completed regression testing on the installed hardware. The original implementation is evidenced by the Telegram screenshot; the revised code should be validated using the accompanying test plan before installation.

## Design decisions

### Two-controller architecture

The ESP32-C3 handles frequent sensor sampling and filtering. The ESP32-CAM handles camera capture, streaming, and image delivery. This separates sensor timing from camera memory and networking work.

### Dual camera trigger

The UART message communicates the event type. A separate digital pulse provides a backup capture request. If the UART message is lost, the camera may use its default event caption; this condition is included in the test plan.

### Adaptive cover detection

The LDR baseline follows gradual changes slowly. A rapid, sustained drop relative to the baseline is treated as a probable obstruction. Cover detection is disabled when the baseline is already dark, reducing night-time false positives.

### Vibration filtering

An interrupt counts debounced edges. The main loop confirms a tamper event only when the configured number of edges occurs in a fixed time window.

## Production considerations

- Replace insecure TLS mode with verified certificates.
- Protect the local stream with authentication before exposing it beyond a trusted network.
- Add persistent offline event storage.
- Add brownout, battery, and camera-health monitoring.
- Validate power integrity and decoupling under Wi-Fi transmission current peaks.

