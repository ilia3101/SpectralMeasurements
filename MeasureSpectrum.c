#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "Control/Control.h"

int main(int argc, char ** argv)
{
    if (argc != 3)
    {
        puts("usage: ./MeasureSpectrum OUTPUT_FILENAME INTEGRATION_TIME");
        return 1;
    }

    FILE * out = fopen(argv[1], "w");

    InitMeasurementSystem("");

    /* Set integration time */
    SetIngtegrationTime(atoi(argv[2]));

    for (int w = 300; w <= 800; ++w)
    {
        SetMonochromatorWavelength(w);
        fprintf(out, "%i %i\n", w, TakeReadingOnlyDiode());
        if (w % 20) fflush(out); /* So data is in the file if user quits it with control C */
    }

    fclose(out);

    FinishMeasurement();
    return 0;
}