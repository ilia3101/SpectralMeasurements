#include <stdio.h>
#include <stdlib.h>

#include "Control/Control.h"

int main(int argc, char ** argv)
{
    if (argc == 2)
    {
        InitMeasurementSystem("/dev/ttyUSB1");
        SetMonochromatorWavelength(atoi(argv[1]));
    }
    else
    {
        puts("Enter wavelength as argument");
    }

    return 0;
}