# Verification and validation plan

## Current status

All tests below are pending for the published revision. The prototype screenshot is supporting evidence, not a completed regression report.

Record firmware commit, board/sensor variants, supply, toolchain/library versions, network conditions, date and tester. Preserve timestamped serial and Telegram evidence.

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

Record both successful builds and functional hardware results before declaring the revision validated. Review concurrent events, power integrity and network failures explicitly. Retain the installed firmware/source for recovery.
