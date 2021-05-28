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
        MonochromatorCalibrate(
            atoi(argv[1]),
            atoi(argv[2]),
            atoi(argv[3]),
            atoi(argv[4])
        );
    }
}