//rm a.out; gcc -c Control/MessageSender.c; gcc -c Control/Control.c; gcc -c main.c; gcc *.o; rm *.o; ./a.out
#include <stdio.h>

#include "Control/Control.h"

int main(int argc, char *argv[])
{
    InitMeasurementSystem("/dev/ttyUSB1");

    // MonochromatorCalibrate(880, 880, 100, 8);

    int wavelength = 300;

    SetMonochromatorWavelength(wavelength);

    SetIngtegrationTime(2000);

    while (1)
    {
        SetMonochromatorWavelength(wavelength);

        int reading = TakeReading();
        printf("Diode reading = %i\n", reading);

        wavelength = wavelength + 1;
    }

    return 0;
}