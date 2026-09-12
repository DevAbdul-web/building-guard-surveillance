# Engineering design notes

## Scope

The complete Building Guard prototype was designed, built and tested by Abdulhamid Abdulkadir before submission to the University of Ilorin laboratory, where it is being used. The detection logic, communication interfaces and operating parameters of the repository firmware are documented below. Subsequent firmware cleanup and Wi-Fi recovery improvements are included in the published code.

## Detection logic

### Motion

PIR HIGH qualifies a motion event, subject to its cooldown. A continuously asserted input can produce another alert after cooldown; the logic is not rising-edge-only.

### Vibration

The interrupt uses CHANGE and counts transitions spaced at least 15 ms apart. Two accepted edges in an evaluation window qualify an event. Two edges need not mean two impacts: both transitions of one sensor pulse can count. Validate the actual waveform and mounting.

### Possible covering

Startup calibration waits three seconds and averages ten ADC readings spaced 100 ms apart. During monitoring, the baseline must exceed 150 raw ADC counts, and the reading must remain below 35% of baseline for three samples.

When no qualifying drop is present:
`baseline = 0.98 × baseline + 0.02 × reading`

The baseline is held during qualifying darkness. Persistent covering may generate repeated alerts after cooldown. The LDR measures local illumination, not optical image obstruction or calibrated lux.

## Firmware parameters

| Parameter | Value | Meaning |
| --- | --- | --- |
| SENSOR_SAMPLE_MS | 100 ms | Nominal polling interval |
| ALERT_COOLDOWN_MS | 10,000 ms | Per-type C3 cooldown |
| VIBRATION_DEBOUNCE_MS | 15 ms | Minimum edge spacing |
| VIBRATION_WINDOW_MS | 800 ms | Evaluation window |
| VIBRATION_MIN_HITS | 2 | Accepted edges required |
| LDR_EMA_ALPHA | 0.02 | Baseline weight |
| LDR_COVER_RATIO | 0.35 | Relative threshold |
| LDR_DAYLIGHT_FLOOR | 150 | Raw ADC baseline floor |
| LDR_CONFIRM_READS | 3 | Consecutive qualifying readings |
| CAMERA_TRIGGER_MS | 500 ms | Nominal trigger pulse |
| CAPTURE_COOLDOWN_MS | 8,000 ms | Camera automatic capture interval |
| WIFI_RETRY_MS | 15,000 ms | Reconnection attempt interval |

Synchronous network calls and UART reads can delay processing. Three samples do not establish a guaranteed 300 ms response.

## Board interface

UART is 115200 baud, 8N1, with newline-terminated ASCII commands.

| Command | Camera caption |
| --- | --- |
| MOTION | ALERT: Motion detected |
| TAMPER | TAMPER ALERT: Physical disturbance detected |
| COVERED | TAMPER ALERT: Camera lens covered |

C3 TX GPIO6 connects to camera RX GPIO3. Camera TX GPIO1 connects to C3 RX GPIO7. Camera Serial also emits diagnostic output; there is no structured acknowledgement protocol.

C3 GPIO10 drives camera GPIO2 as an additional capture request. This rising edge carries no event type, so its caption can be the current/default caption. UART and trigger processing do not implement transactional deduplication; test their interaction.

## Concurrency and failure behavior

The C3 evaluates motion, vibration and cover conditions in that order, with independent cooldowns. The camera stores one pending flag and one caption, not a queue. Closely spaced commands can replace captions and do not guarantee one photograph per event.

Streaming, manual capture and automatic capture share camera/server resources. Concurrent operation needs testing.

Offline notifications may be skipped and are not persistently replayed. Wi-Fi recovery attempts do not guarantee delivery. Measure supply integrity during camera flash and wireless transmission rather than assuming the diagram establishes a power budget.

## Release requirements

Build settings and regression results should be recorded whenever firmware changes are introduced. Certificate verification, web authentication and durable event handling are planned for broader deployment. Credentials should be kept outside tracked source files. Quantitative accuracy, latency and uptime claims should be supported by repeatable measurements and retained evidence.
