#include <stdio.h>

#include "Control/Control.h"

/* Description of a section */
typedef struct {
    int start_wavelength;
    int max_wavelength;
    int step_nm;
    char * comment;
} section_t;

#define MAX_SECTIONS 20

/* Define sections */
int num_sections = 3;
section_t sections[MAX_SECTIONS] =
{
    {
        .start_wavelength = 380,
        .max_wavelength = 424,
        .step_nm = 2,
        .comment = "Use the 375-425nm bandpass filter for this section"
    },
    {
        .start_wavelength = 410,
        .max_wavelength = 690,
        .step_nm = 2,
        .comment = "Remove filter, no filter for this section"
    },
    {
        .start_wavelength = 680,
        .max_wavelength = 780,
        .step_nm = 2,
        .comment = "Use the 695nm longpass now"
    }
};

/* Pause between sections? */
int do_pause = 1;

int main()
{
    InitMeasurementSystem("/dev/ttyUSB1");

    FILE * file = fopen("diode.log", "w");

    fprintf(file, "--- START");


    for (int s = 0; s < num_sections; ++s)
    {
        /* Indicates start of new section */
        fprintf(file, "--- SECTION");

        section_t section = sections[s];

        for (int w = section.start_wavelength; w <= section.max_wavelength; w += section.step_nm)
        {
            SetMonochromatorWavelength(w);

            /* Now call TakeReading function which will activate the camera shutter and read
             * out the diode, giving the diode's reading as its return value */
            uint32_t diode_reading = TakeReading();

            /* Write wavelength and diode value */
            fprintf(file, "%i %i", w, diode_reading);
        }
    }
    
}