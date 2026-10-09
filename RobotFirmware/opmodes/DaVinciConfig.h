#pragma once

// Team-owned configuration. Unknown measurements are deliberately invalid.
namespace davinci {
constexpr bool driveVerified = true; // Enable board M_L/M_R for a raised-wheel direction test.
constexpr bool mechanismsVerified = false;
constexpr bool leftFlipped = true; // Curriculum example; verify wheels raised.
constexpr bool rightFlipped = false;
constexpr int armPort = -1; // Physical servo port 1..4
constexpr int grabberPort = -1;
constexpr float armMin = -1, armMax = -1;
constexpr float stow = -1, pickup = -1, travel = -1, high = -1;
constexpr float grabMin = -1, grabMax = -1, grabOpen = -1, grabClosed = -1;
constexpr unsigned closeWaitMs = 0, releaseWaitMs = 0; // Measure loaded travel times.
constexpr float normalSpeed = 0.25f, precisionSpeed = 0.15f;
constexpr float deadzone = 0.08f;
}
