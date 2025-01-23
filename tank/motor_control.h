#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <ESP32MotorControl.h>
#include <WebSocketsServer.h>

// Define motor GPIO pins
extern const int motorAGPIO2;
extern const int motorAGPIO3;
extern const int motorBGPIO4;
extern const int motorBGPIO5;

extern const int RSpeed;
extern const int LSpeed;

// MotorControl instance
extern ESP32MotorControl MotorControl;

// Motor states
extern int motorABackward;
extern int motorAForward;
extern int motorBBackward;
extern int motorBForward;

void setupMotors();
void handleMotorCommands(uint8_t *payload, uint8_t client_num, WebSocketsServer& webSocket);

#endif
