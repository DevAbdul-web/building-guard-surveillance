# Verification and validation plan

## Purpose and test records

The original integrated prototype was tested before submission to the University of Ilorin laboratory. The procedures below are provided for repeatable commissioning and regression checks following firmware or hardware changes. Results should be recorded for the specific revision and hardware configuration being evaluated.

The firmware commit, board and sensor variants, supply, toolchain versions, network conditions, date and tester should be recorded. Timestamped serial logs and Telegram evidence should be retained.

## Test matrix

| ID | Procedure | Acceptance or observation |
| --- | --- | --- |
| BG-01 | Compile both sketches | Successful builds with recorded versions and board settings |
| BG-02 | Boot with LDR exposed | Calibration completes and both boards initialize |
| BG-03 | Trigger PIR alone | Motion text and matching photograph received |
| BG-04 | Observe vibration output during one tap | Record both transitions; distinguish edges from physical impacts |
| BG-05 | Apply one accepted edge, then two within one 800 ms window | One edge rejected; two qualify |
| BG-06 | Dim lighting gradually | Record baseline tracking and any false cover alerts |
| BG-07 | Rapidly obscure LDR in sufficient light | Cover classification after three qualifying samples; retain output |
| BG-08 | Cover lens while leaving LDR exposed | Characterize missed obstruction and placement limitations |
| BG-09 | Boot below daylight floor | Cover classification disabled; other inputs remain evaluated |
| BG-10 | Repeat same event within 10 seconds | Controller repeat suppressed inside cooldown |
| BG-11 | Hold motion/cover beyond cooldown | Record repeated events and timing |
| BG-12 | Trigger different sensors closely together | Compare controller events, photograph count and caption; quantify losses |
| BG-13 | Trigger camera without UART command | Capture requested; record default/current caption |
| BG-14 | Send UART and trigger together | Check duplicate, delayed or incorrectly captioned photographs |
| BG-15 | Open camera dashboard, stream and manual capture | Record operation and any request blocking |
| BG-16 | Trigger automatic capture while streaming | Record resource conflicts, stalls and recovery |
| BG-17 | Remove and restore Wi-Fi | Record reconnection, skipped notifications and sensing delays |
| BG-18 | Restart both boards, then each separately | Record boot reliability and link recovery |
| BG-19 | Measure supply during flash and transmission | Record voltage minimum and resets against exact board requirements |
| BG-20 | Run sustained monitoring | Record duration, false alerts, missed events and resets |

Known limitations require characterization, not an assumed pass. Repeat trials under recorded conditions before making performance claims.

## Result template

- Test ID:
- Date / tester:
- Firmware commit:
- Hardware / toolchain:
- Preconditions:
- Procedure / repetitions:
- Expected behavior:
- Observed behavior:
- Result: PASS / FAIL / CHARACTERIZED / BLOCKED
- Measurements / evidence paths:
- Deviations / follow-up:

## Release decision

For each release, build and functional test results should be recorded. Concurrent events, power integrity and network recovery should be reviewed. A recoverable copy of the installed firmware and source should be retained.
