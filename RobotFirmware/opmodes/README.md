# DaVinci 027 — first TeleOp

Team 027 code for the official NRL v1.3.8 student kit. The local generated
project is at `C:/Users/msi/NRL/DaVinci027` (ESP-NOW channel 11). This fork
contains the team files for review; generate your own team-stamped project
with HexaSDK, then copy these files into its `RobotFirmware/opmodes/` folder.
The generator intentionally creates an empty `opmodes/` directory.

**Connection status:** A controller and robot flashed from the same v1.3.8 kit
still showed repeated `LINK LOST` / `LINK RESTORED` messages at the robot while
no OpMode was running. The link fault is unresolved. The link engine is shipped
as a prebuilt library, so this student TeleOp does not alter its behavior.
See [the public report](https://github.com/NationalRoboticsLeague-NRL/NRL-NationalRoboticsLeague/issues/1)
for another team's similar symptoms.

## Current status

Hardware outputs are disabled. This is intentional: actual wiring, motor
directions, servo limits and loaded movement times have not been supplied.
Only team code in this folder has been edited after project generation.

The live Programming Curriculum Module 3.2 documents TankDrive, left Y forward,
right X steering, and LB precision mode. Module 3.3 documents servo ports and
limits. Its advice to call servo `begin()` in INIT conflicts with the requested
motion-free INIT: our program defers all hardware initialization to START.
Competition-rule compliance and hardware behavior still require verification.

## Proposed controls

| Control | Behavior after configuration |
|---|---|
| Left Y / right X | Forward / turn on tank drivetrain |
| Hold LB | 35% precision speed; normal speed is 75% |
| D-pad down / up | Pickup / high arm position |
| D-pad left / right | Stow / travel arm position |
| X | Toggle grabber |
| A | Close grabber, wait measured time, lift to travel |
| RB | Release, wait measured time, return arm to travel |
| Hold Y | Cancel macro and stop drive; preserve arm pose |
| LT | Reserved for official controller exit/STOP UI |

Presets and speed are starting software choices, not tested robot calibration.
Driver aligns before A or RB; neither macro detects a CORE or alignment.
No STAR/APEX presets are invented before verifying mechanical reach.

## Hardware setup

1. Identify the physical M_L/M_R connections and verify direction with wheels
   raised. The left-flipped setting follows the curriculum example, not a
   measurement of this robot. Enable `driveVerified` only after checking it.
2. Identify arm/grabber servo ports 1–4. Support the arm before energizing or
   stopping; STOP detaches servos and removes holding torque.
3. Measure safe angle limits, stow, pickup, travel, high, grab-open and
   grab-closed positions. Enter them in `DaVinciConfig.h`.
4. Measure close/release times under load and add a suitable settling margin.
   These are open-loop delays, not feedback confirming physical completion.
5. Set `mechanismsVerified` only when every value is checked. Invalid settings
   keep mechanisms disabled. START commands stow and grab-closed.
6. Bench-test INIT (no movement), START, neutral-stick gating, each direction,
   precision, each preset, macro cancel, official STOP and link loss.
   Link loss latches this OpMode off until an explicit restart.

No firmware has been uploaded to hardware by this task. Flash the robot only
after confirming the board and safe bench setup. Use the generated project's
official controller flasher and select channel 11 on the controller.
