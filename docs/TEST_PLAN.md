# Building Guard Hardware Test Plan

Record the firmware commit, date, tester, supply voltage, network condition, and observed result for every run.

| ID | Test | Procedure | Expected result |
|---|---|---|---|
| BG-01 | Startup | Power both controllers with sensors unobstructed | Controllers initialize, LDR calibrates, status heartbeat begins, and online message is sent |
| BG-02 | Motion | Move through the PIR field of view | Motion text and correctly captioned photograph reach Telegram |
| BG-03 | Vibration rejection | Tap the enclosure once lightly | A single unconfirmed edge does not create a tamper alert |
| BG-04 | Physical tamper | Produce two or more valid vibration edges inside the configured window | Physical-tamper text and photograph reach Telegram |
| BG-05 | Gradual darkness | Reduce room lighting slowly | Baseline adapts without producing a lens-cover alarm |
| BG-06 | Lens cover | Cover the LDR/camera rapidly for the required consecutive samples | Lens-cover text and correctly captioned photograph reach Telegram |
| BG-07 | Dark-room behavior | Start with ambient value below the daylight floor | Cover classification remains disabled and does not create repeated false alarms |
| BG-08 | Cooldown | Repeat the same event inside ten seconds | Duplicate event is suppressed |
| BG-09 | Independent cooldown | Trigger two different event types close together | Each event uses its own cooldown and can be reported |
| BG-10 | Live stream | Open the controller address from the same network | Browser displays the camera stream |
| BG-11 | Manual capture | Select Capture and Send to Telegram | A manually captioned photograph reaches Telegram |
| BG-12 | Wi-Fi recovery | Disconnect and restore Wi-Fi | Local sensing continues and both controllers reconnect automatically |
| BG-13 | Event labeling | Trigger each sensor separately | Photograph caption matches MOTION, TAMPER, or COVERED |
| BG-14 | Restart | Restart both controllers | System returns to monitoring without unsafe output behavior |

## Evidence to retain

- Serial logs for every test
- Telegram screenshots with timestamps
- Short videos of the physical trigger and received notification
- Photograph of the installed device
- Notes describing deviations and corrective actions

