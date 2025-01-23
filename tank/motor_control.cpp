#include "motor_control.h"

const int motorAGPIO2 = 2;
const int motorAGPIO3 = 3;
const int motorBGPIO4 = 4;
const int motorBGPIO5 = 5;

const int RSpeed = 73;
const int LSpeed = 100;

ESP32MotorControl MotorControl = ESP32MotorControl();

int motorABackward = 0;
int motorAForward = 0;
int motorBBackward = 0;
int motorBForward = 0;

char msg_buf[20];

void setupMotors() 
{
    MotorControl.attachMotors(motorAGPIO2, motorAGPIO3, motorBGPIO4, motorBGPIO5);
}
void handleMotorCommands(uint8_t *payload, uint8_t client_num, WebSocketsServer& webSocket)
{
    char msg_buf[50]; // buffer for sending motor states

    if (strcmp((char*)payload, "moveForwardMotorA") == 0) 
    {
        motorAForward = motorAForward ? 0 : 1;
        if (motorAForward == 1) {
            motorABackward = 0;
            MotorControl.motorForward(0, LSpeed);
        }
        else {
            MotorControl.motorStop(0);
        }
    }
    else if (strcmp((char*)payload, "stateForwardMotorA") == 0) 
    {
        sprintf(msg_buf, "motorAforward:%d", motorAForward);
        //webSocket.sendTXT(client_num, msg_buf);
        webSocket.broadcastTXT(msg_buf);
    }
    else if (strcmp((char*)payload, "moveBackwardMotorA") == 0) 
    {
        motorABackward = motorABackward ? 0 : 1;
        if (motorABackward == 1) {
            motorAForward = 0;
            MotorControl.motorReverse(0, LSpeed);
        }
        else {
            MotorControl.motorStop(0);
        }
    }
    else if (strcmp((char*)payload, "stateBackwardMotorA") == 0) 
    {
        sprintf(msg_buf, "motorAbackward:%d", motorABackward);
        //webSocket.sendTXT(client_num, msg_buf);
        webSocket.broadcastTXT(msg_buf);
    }
    else if (strcmp((char*)payload, "moveForwardMotorB") == 0) 
    {
        motorBForward = motorBForward ? 0 : 1;
        if (motorBForward == 1) {
            motorBBackward = 0;
            MotorControl.motorForward(1, RSpeed);
        }
        else {
            MotorControl.motorStop(1);
        }
    }
    else if (strcmp((char*)payload, "stateForwardMotorB") == 0) 
    {
        sprintf(msg_buf, "motorBforward:%d", motorBForward);
        //webSocket.sendTXT(client_num, msg_buf);
        webSocket.broadcastTXT(msg_buf);
    }
    else if (strcmp((char*)payload, "moveBackwardMotorB") == 0) 
    {
        motorBBackward = motorBBackward ? 0 : 1;
        if (motorBBackward == 1) {
            motorBForward = 0;
            MotorControl.motorReverse(1, RSpeed);
        }
        else {
            MotorControl.motorStop(1);
        }
    }
    else if (strcmp((char*)payload, "stateBackwardMotorB") == 0)
    {
        sprintf(msg_buf, "motorBbackward:%d", motorBBackward);
        //webSocket.sendTXT(client_num, msg_buf);
        webSocket.broadcastTXT(msg_buf);
    }
}
