#include <stdio.h>
#include <stdlib.h>

#include "Control/Control.h"


int main(int argc, char ** argv)
{
    if (argc != 5)
    {
        puts(
            "usage: ./CalibrateMonochromator CurrentWavelength MaxWavelength MinWavelength StepsPerNm\n"
            "Any argument can be set to zero if it does not need to be updated"
        );
    }
    else
    {
        InitMeasurementSystem();

        int current_wavelength = atoi(argv[1]);
        int max_wavelength = atoi(argv[2]);
        int min_wavelength = atoi(argv[3]);
        int wavelength_step = atoi(argv[4]);
        printf(
            "Calibrating monochromator:\n"
            "Current wavelength: %i\n"
            "Max wavelength: %i\n"
            "Max wavelength: %i\n"
            "Wavelength step: %i\n",
            current_wavelength,
            max_wavelength,
            min_wavelength,
            wavelength_step
        );
        MonochromatorCalibrate(
            current_wavelength,
            max_wavelength,
            min_wavelength,
            wavelength_step
        );
    }

    return 0;
}