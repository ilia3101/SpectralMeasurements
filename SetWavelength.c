#include <stdio.h>
#include <stdlib.h>

#include "Control/Control.h"

int main(int argc, char ** argv)
{
    if (argc == 2)
    {
        InitMeasurementSystem();
        SetMonochromatorWavelength(atoi(argv[1]));
    }
    else
    {
        puts("Enter wavelength as argument");
    }

    FinishMeasurement();
    return 0;
}