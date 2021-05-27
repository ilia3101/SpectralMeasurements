#include "Servo.h"

int servo = 5;
void ServoInit()
{
    pinMode(servo, OUTPUT);
}

void ServoPressShutter()
{
    int angle = 0;
    for (angle = 0; angle <= 16; angle += 1)
    {
        servoPulse(servo, angle);
        delay(5);
    }
    delay(100);
    for (angle = angle; angle >= 0; angle -= 1)
    {
        servoPulse(servo, angle);
        delay(5);
    }
}

void servoPulse (int servo, int angle)
{
    int pwm = angle * 9 + 500;
    digitalWrite(servo, HIGH);
    delayMicroseconds(pwm); //First delay
    digitalWrite(servo, LOW);
}
