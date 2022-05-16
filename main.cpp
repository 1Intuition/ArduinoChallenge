/*
 * This software is authored by Teodor Oprea
 * in collaboration with Nathan Wong and Nirav Patel.
 */

#include <stdint.h>

#include <Servo.h>
#include <IRremote.h>


// Color sensor pins
constexpr uint8_t CS_S0 = 2;
constexpr uint8_t CS_S1 = 3;
constexpr uint8_t CS_S2 = 4;
constexpr uint8_t CS_S3 = 5;
constexpr uint8_t CS_OUT = 6;
constexpr uint8_t CS_LED = 7;

// ultrasonic sensor pins
constexpr uint8_t US_TRIG_PIN = 8;
constexpr uint8_t US_ECHO_PIN = 9;

// servo pins
constexpr uint8_t LEFT_SERVO_PIN = 10;
constexpr uint8_t RIGHT_SERVO_PIN = 11;

// infrared sensor pins
constexpr uint8_t IR_RECEIVE_PIN = 12;

// experimental values of the color of tapes
constexpr float RIGHT_TAPE_R = 1.27;
constexpr float RIGHT_TAPE_G = 4.45;
constexpr float RIGHT_TAPE_B = 3.36;
constexpr float LEFT_TAPE_R = 2.38;
constexpr float LEFT_TAPE_G = 2.31;
constexpr float LEFT_TAPE_B = 2.92;
constexpr float MID_TAPE_R = 3.14;
constexpr float MID_TAPE_G = 3.00;
constexpr float MID_TAPE_B = 1.64;

// distances in cm for ultrasonic algorithm
constexpr uint16_t SMALL_DIST = 18;
constexpr uint16_t BIG_DIST = 24;
constexpr uint16_t NO_WALL_DIST = 70;
constexpr uint8_t NO_WALL_MAX_COUNT = 10;

// delays in-between servo movements
constexpr uint32_t MOVEMENT_SMALL_DELAY = 100;
constexpr uint32_t MOVEMENT_BIG_DELAY = 200;

// delay in ms to wait IR signal before stopping cart
constexpr uint32_t WAIT_IR_SIGNAL_BEFORE_STOP = 130;

// delay between checking frequencies in color sensor
constexpr uint32_t CS_INTERNAL_DELAY = 20;

// servo turn write angle change
constexpr uint32_t WALL_TURN_DEG = 10;
constexpr uint32_t TAPE_TURN_DEG = 15;

// defining modes, remote buttons, and different tapes
enum class SysMode : uint8_t {RemoteControl, WallFollow, TapeFollow};
enum class RemoteButtons : uint8_t {Up = 0x18, Left = 0x8, Right = 0x5A, 
  Down = 0x46, Mode1 = 0x45, Mode2 = 0x46, Mode3 = 0x47};
enum class TapeColor : uint8_t {MidTape, LeftTape, RightTape};

// initializing servo objects
Servo leftServo, rightServo;

// initializing state variables
SysMode currMode = SysMode::RemoteControl;
uint8_t waitIR = 0, noWallCount = 0;

// functions related to servo movement for different tasks
namespace Movement {

  void stopMoving(void) {
    leftServo.write(90);
    rightServo.write(90);
  }

  namespace RemoteControl {

    void turnLeft(void) {
      leftServo.write(0);
      rightServo.write(0);
    }

    void turnRight(void) {
      leftServo.write(180);
      rightServo.write(180);
    }

    void moveForward(void) { 
      leftServo.write(180);
      rightServo.write(0);
    }

    void moveBackward(void) {
      leftServo.write(0);
      rightServo.write(180);
    }
  }

  namespace WallFollow {

    void turnLeft(void) {
      leftServo.write(90 + WALL_TURN_DEG);
      rightServo.write(0);
    }

    void turnRight(void) {
      leftServo.write(180);
      rightServo.write(90 - WALL_TURN_DEG);
    }

    void moveStraight(void) {
      leftServo.write(180);
      rightServo.write(0);
    }
  }

  namespace TapeFollow {

    void turnLeft(void) {
      leftServo.write(90 - TAPE_TURN_DEG);
      rightServo.write(90 - TAPE_TURN_DEG);
    }

    void turnRight(void) {
      leftServo.write(90 + TAPE_TURN_DEG);
      rightServo.write(90 + TAPE_TURN_DEG);
    }

    void moveStraight(void) {
      leftServo.write(90 + TAPE_TURN_DEG);
      rightServo.write(90 - TAPE_TURN_DEG);
    }
  }
}

// getting information from ultrasonic and color sensors
namespace Sensors {

  uint16_t getDistance(void) {  // ultrasound sensor
    
    // send ultrasonic signal
    digitalWrite(US_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(US_TRIG_PIN, LOW);
    
    // get the pulse duration in ms
    const uint32_t duration = pulseIn(US_ECHO_PIN, HIGH);
    
    // convert to distance by using 0.034 as the speed of sound (cm/ms)
    return duration * 0.034 / 2;
  }

  TapeColor readColor(void) {  // color sensor
    
    // detecting red color frequency
    digitalWrite(CS_S2, LOW);
    digitalWrite(CS_S3, LOW);
    const uint32_t redFrequency = pulseIn(CS_OUT, LOW);
    delay(CS_INTERNAL_DELAY);
    
    // detecting green color frequency
    digitalWrite(CS_S2, HIGH);
    digitalWrite(CS_S3, HIGH);
    const uint32_t greenFrequency = pulseIn(CS_OUT, LOW);
    delay(CS_INTERNAL_DELAY);
    
    // detecting blue color frequency
    digitalWrite(CS_S2, LOW);
    digitalWrite(CS_S3, HIGH);
    const uint32_t blueFrequency = pulseIn(CS_OUT, LOW);
    delay(CS_INTERNAL_DELAY);
    
    // detecting clear (no filter) frequency
    digitalWrite(CS_S2, HIGH);
    digitalWrite(CS_S3, LOW);
    const uint32_t clearFrequency = pulseIn(CS_OUT, LOW);
    delay(CS_INTERNAL_DELAY);
    
    // calculate color ratios
    const float redRatio = (float) redFrequency / clearFrequency;
    const float greenRatio = (float) greenFrequency / clearFrequency;
    const float blueRatio = (float) blueFrequency / clearFrequency;
    
    // calculate differences for each tape color
    const uint32_t midTapeDiff = abs(MID_TAPE_R - redRatio) +
      abs(MID_TAPE_G - greenRatio) + abs(MID_TAPE_B - blueRatio);
    const uint32_t leftTapeDiff = abs(LEFT_TAPE_R - redRatio) +
      abs(LEFT_TAPE_G - greenRatio) + abs(LEFT_TAPE_B - blueRatio);
    const uint32_t rightTapeDiff = abs(RIGHT_TAPE_R - redRatio) +
      abs(RIGHT_TAPE_G - greenRatio) + abs(RIGHT_TAPE_B - blueRatio);
    
    // return value corresponding with minimum difference
    return ((midTapeDiff < leftTapeDiff) ?
      ((rightTapeDiff < midTapeDiff) ? 
        TapeColor::RightTape : TapeColor::MidTape) :
      ((leftTapeDiff < rightTapeDiff) ? 
        TapeColor::LeftTape : TapeColor::RightTape));
  }
}

void setup() {

  Serial.begin(9600);

  // attach servos
  leftServo.attach(LEFT_SERVO_PIN);
  rightServo.attach(RIGHT_SERVO_PIN);
  Movement::stopMoving();

  // ultrasonic IO
  pinMode(US_TRIG_PIN, OUTPUT);
  pinMode(US_ECHO_PIN, INPUT);
  digitalWrite(US_TRIG_PIN, LOW);

  // color sensor IO
  pinMode(CS_S0, OUTPUT);
  pinMode(CS_S1, OUTPUT);
  pinMode(CS_S2, OUTPUT);
  pinMode(CS_S3, OUTPUT);
  pinMode(CS_OUT, INPUT);
  pinMode(CS_LED, OUTPUT);

  // enable lights on color sensor
  digitalWrite(CS_LED, HIGH);

  // set up color sensor frequency to 20%
  digitalWrite(CS_S0, HIGH);
  digitalWrite(CS_S1, LOW);
  digitalWrite(CS_S2, LOW);
  digitalWrite(CS_S3, LOW);

  // IR sensor
  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);
}


void loop() {

  // checks the current mode
  if (currMode == SysMode::RemoteControl) {

    if (IrReceiver.decode()) {  // receive infrared signal with sensor
      const bool isRepeating = (IrReceiver.decodedIRData.flags ==
                                IRDATA_FLAGS_IS_REPEAT);
      const uint8_t command = IrReceiver.decodedIRData.command;

      // after pressing movement command, move and wait before stopping
      if (command == static_cast<uint8_t>(RemoteButtons::Up)) {
        waitIR = WAIT_IR_SIGNAL_BEFORE_STOP;
        Movement::RemoteControl::moveForward();
      } else if (command == static_cast<uint8_t>(RemoteButtons::Left)) {
        waitIR = WAIT_IR_SIGNAL_BEFORE_STOP;
        Movement::RemoteControl::turnLeft();
      } else if (command == static_cast<uint8_t>(RemoteButtons::Right)) {
        waitIR = WAIT_IR_SIGNAL_BEFORE_STOP;
        Movement::RemoteControl::turnRight();
      } else if (command == static_cast<uint8_t>(RemoteButtons::Down)) {
        waitIR = WAIT_IR_SIGNAL_BEFORE_STOP;
        Movement::RemoteControl::moveBackward();
      }

      // when changing modes, stop the cart and change current mode
      else if (command == static_cast<uint8_t>(RemoteButtons::Mode2)) {
        if (!isRepeating) {
          noWallCount = 0;
          currMode = SysMode::WallFollow;
          Movement::stopMoving();
          delay(MOVEMENT_SMALL_DELAY);
        }
      } else if (command == static_cast<uint8_t>(RemoteButtons::Mode3)) {
        if (!isRepeating) {
          currMode = SysMode::TapeFollow;
          Movement::stopMoving();
          delay(MOVEMENT_SMALL_DELAY);
        }
      }

      IrReceiver.resume();  // re-enables input reading from sensor
    }

    // if no valid command, stop the cart after some time
    else {
      // decrement waitIR until 1, then stop the cart and set waitIR to 0
      if (waitIR) {
        if (waitIR > 1) {
          --waitIR;
          delay(1);
        } else {
          Movement::stopMoving();
          waitIR = 0;
        }
      }
    }

  } else if (currMode == SysMode::WallFollow) {
    if (IrReceiver.decode()) {  // receive infrared signal with sensor
      // change to remote control if button 1 is pressed
      if (IrReceiver.decodedIRData.command == 
          static_cast<uint8_t>(RemoteButtons::Mode1)) {
        currMode = SysMode::RemoteControl;
        Movement::stopMoving();
      }
      IrReceiver.resume();

    } else {  // start follow the wall task

      // get distance from ultrasonic sensor
      const uint16_t distance = Sensors::getDistance();

      if (distance > NO_WALL_DIST) {  // no wall detected

        // the cart stops if there is no wall for some time
        if (noWallCount >= NO_WALL_MAX_COUNT) {
          Movement::stopMoving();
          currMode = SysMode::RemoteControl;
        } else {
          Movement::WallFollow::moveStraight();
          noWallCount++;
        }

        return;  // skips rest of code for performance
      }

      noWallCount = 0;  // set count to 0 if it detected a wall
      
      if (distance < SMALL_DIST) {  // turns right if too close
        Movement::WallFollow::turnRight();
        delay(MOVEMENT_BIG_DELAY);
        Movement::WallFollow::moveStraight();
        delay(MOVEMENT_SMALL_DELAY);
      } else if (distance < BIG_DIST) {  // goes straight if good distance
        Movement::WallFollow::moveStraight();
        delay(MOVEMENT_BIG_DELAY);
      } else {  // turns left if too far
        Movement::WallFollow::turnLeft();
        delay(MOVEMENT_BIG_DELAY);
        Movement::WallFollow::moveStraight();
        delay(MOVEMENT_SMALL_DELAY);
      }
    }

  } else if (currMode == SysMode::TapeFollow) {

    if (IrReceiver.decode()) {  // receive infrared signal with sensor
      // change to remote control if button 1 is pressed
      if (IrReceiver.decodedIRData.command == 
          static_cast<uint8_t>(RemoteButtons::Mode1)) {
        currMode = SysMode::RemoteControl;
        Movement::stopMoving();
      }
      IrReceiver.resume();
    } else {  // start follow the tape task
      
      // detect tape color with color sensor
      const TapeColor c = Sensors::readColor();

      // move cart according to tape color
      if (c == TapeColor::MidTape) {
        Movement::TapeFollow::moveStraight();
        delay(MOVEMENT_BIG_DELAY);
      } else if (c == TapeColor::LeftTape) {
        Movement::TapeFollow::turnRight();
        delay(MOVEMENT_SMALL_DELAY);
      } else if (c == TapeColor::RightTape) {
        Movement::TapeFollow::turnLeft();
        delay(MOVEMENT_SMALL_DELAY);
      }
      Movement::stopMoving();
      delay(MOVEMENT_SMALL_DELAY);
    }
  }
}
