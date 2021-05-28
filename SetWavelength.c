#include <stdio.h>
#include <stdlib.h>

#include "Control/Control.h"

int main(int argc, char ** argv)
{
    InitMeasurementSystem("/dev/ttyUSB1");

    if (argc == 2)
    {
        SetMonochromatorWavelength(atoi(argv[1]));
    }
    else
    {
        puts("Enter wavelength as argument");
    }

    return 0;
}