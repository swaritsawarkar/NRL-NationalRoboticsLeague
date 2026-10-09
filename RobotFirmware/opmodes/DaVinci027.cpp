#include "NRL.h"
#include "DaVinciConfig.h"
#include <cmath>

namespace {
bool inRange(float x, float lo, float hi) {
    return std::isfinite(x) && x >= lo && x <= hi;
}
bool mechanismsValid() {
    using namespace davinci;
    return mechanismsVerified && armPort >= 1 && armPort <= 4 &&
        grabberPort >= 1 && grabberPort <= 4 && armPort != grabberPort &&
        inRange(armMin, 0, 180) && inRange(armMax, armMin, 180) &&
        armMin < armMax && inRange(grabMin, 0, 180) &&
        inRange(grabMax, grabMin, 180) && grabMin < grabMax &&
        inRange(stow, armMin, armMax) && inRange(pickup, armMin, armMax) &&
        inRange(travel, armMin, armMax) && inRange(high, armMin, armMax) &&
        inRange(grabOpen, grabMin, grabMax) && inRange(grabClosed, grabMin, grabMax) &&
        closeWaitMs > 0 && closeWaitMs <= 5000 && releaseWaitMs > 0 && releaseWaitMs <= 5000;
}
uint8_t servoPin(int port) {
    const uint8_t pins[] = {SERVO_1, SERVO_2, SERVO_3, SERVO_4};
    return port >= 1 && port <= 4 ? pins[port - 1] : SERVO_1;
    // Invalid port never reaches begin(); the fallback is storage only.
}
HexaServoConfig servoConfig(bool arm) {
    HexaServoConfig c{};
    c.signalPin = servoPin(arm ? davinci::armPort : davinci::grabberPort);
    c.minAngle = arm ? davinci::armMin : davinci::grabMin;
    c.maxAngle = arm ? davinci::armMax : davinci::grabMax;
    c.startAngle = arm ? davinci::stow : davinci::grabClosed;
    return c;
}
float shape(float x) {
    if (!std::isfinite(x)) return 0;
    float a = std::fabs(x);
    if (a <= davinci::deadzone) return 0;
    a = (std::fmin(a, 1.0f) - davinci::deadzone) / (1 - davinci::deadzone);
    return std::copysign(a * a, x);
}
}

class DaVinci027 : public NRLOpMode {
    HexaDCMotor left{{MOTOR_L_DIR, MOTOR_L_PWM, davinci::leftFlipped}};
    HexaDCMotor right{{MOTOR_R_DIR, MOTOR_R_PWM, davinci::rightFlipped}};
    TankDrive drive{left, right};
    HexaServo arm{servoConfig(true)}, grabber{servoConfig(false)};
    enum class Macro { Idle, Closing, Releasing };
    Macro macro = Macro::Idle;
    uint32_t since = 0;
    bool running = false, driveReady = false, mechanismsReady = false;
    bool neutralSeen = false, open = false, fault = false;

    void cancelMacro() { macro = Macro::Idle; cancelActions(); }
    void halt() {
        cancelMacro();
        if (left.isReady()) left.stop();
        if (right.isReady()) right.stop();
        if (driveReady) drive.stop();
        if (arm.isReady()) arm.detach();
        if (grabber.isReady()) grabber.detach();
        running = false;
        telemetry.addData("state", fault ? "FAULT: RESTART" : "STOPPED");
    }
public:
    void init() override {
        // Pure state only: no begin(), attach(), PWM or servo commands in INIT.
        running = driveReady = mechanismsReady = neutralSeen = open = fault = false;
        macro = Macro::Idle;
        telemetry.addData("setup", davinci::driveVerified ? "CHECKED" : "CONFIG REQUIRED");
    }
    void start() override {
        running = true;
        if (davinci::driveVerified) {
            bool l = left.begin(), r = right.begin();
            driveReady = l && r;
            if (!driveReady) { fault = true; halt(); return; }
            drive.stop();
            drive.setRampRate(0.1f);
        }
        if (mechanismsValid()) {
            bool a = arm.begin(), g = grabber.begin();
            mechanismsReady = a && g;
            if (!mechanismsReady) { fault = true; halt(); }
        }
    }
    void loop() override {
        if (!running) return;
        // Latch a link fault: no resumed motion or macro after reconnection.
        if (NRLComms::isInputStale()) { fault = true; halt(); return; }
        if (!std::isfinite(gamepad1.leftY()) || !std::isfinite(gamepad1.rightX())) {
            fault = true; halt(); return;
        }
        const float forward = shape(gamepad1.leftY()), turn = shape(gamepad1.rightX());
        if (!neutralSeen) {
            neutralSeen = forward == 0 && turn == 0;
            return; // One neutral tick required after START before controls act.
        }
        // Y cancels a macro and stops drive while held. Official LT exit stays reserved.
        if (gamepad1.pressed(BTN_Y)) {
            cancelMacro();
            if (driveReady) drive.stop();
            return;
        }
        if (driveReady) {
            // Hold RT to permit motion. Releasing it stops both motors immediately.
            if (gamepad1.pressed(BTN_RT)) {
                drive.setScale(gamepad1.pressed(BTN_LB) ? davinci::precisionSpeed : davinci::normalSpeed);
                drive.drive(forward, turn);
            } else {
                drive.stop();
            }
        }
        if (mechanismsReady) {
            // Direct commands take precedence and cancel any delayed return.
            if (gamepad1.justPressed(BTN_DPAD_DOWN)) { cancelMacro(); arm.setPosition(davinci::pickup); }
            else if (gamepad1.justPressed(BTN_DPAD_UP)) { cancelMacro(); arm.setPosition(davinci::high); }
            else if (gamepad1.justPressed(BTN_DPAD_LEFT)) { cancelMacro(); arm.setPosition(davinci::stow); }
            else if (gamepad1.justPressed(BTN_DPAD_RIGHT)) { cancelMacro(); arm.setPosition(davinci::travel); }
            else if (gamepad1.justPressed(BTN_X)) {
                cancelMacro(); open = !open;
                grabber.setPosition(open ? davinci::grabOpen : davinci::grabClosed);
            } else if (macro == Macro::Idle && gamepad1.justPressed(BTN_A)) {
                open = false; grabber.setPosition(davinci::grabClosed);
                macro = Macro::Closing; since = millis();
            } else if (macro == Macro::Idle && gamepad1.justPressed(BTN_RB)) {
                open = true; grabber.setPosition(davinci::grabOpen);
                macro = Macro::Releasing; since = millis();
            }
            const uint32_t elapsed = uint32_t(millis() - since);
            if ((macro == Macro::Closing && elapsed >= davinci::closeWaitMs) ||
                (macro == Macro::Releasing && elapsed >= davinci::releaseWaitMs)) {
                arm.setPosition(davinci::travel); macro = Macro::Idle;
            }
        }
        telemetry.addData("drive", driveReady ? "READY" : "LOCKED");
        telemetry.addData("arm", mechanismsReady ? "READY" : "UNCALIBRATED");
        telemetry.addData("macro", macro == Macro::Idle ? "IDLE" : "WAITING");
    }
    void stop() override { halt(); }
};
REGISTER_OPMODE(DaVinci027, "DaVinci 027", TELEOP);
