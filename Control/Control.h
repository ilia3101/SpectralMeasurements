#ifndef _Control_h_
#define _Control_h_

#include <stdint.h>

/* Call this at the start */
int InitMeasurementSystem();
void FinishMeasurement();

/* Sets monochromator wavelength */
uint32_t SetMonochromatorWavelength(int Wavelength);

/* Set calibration, pass zero to any value to not update it. */
uint32_t MonochromatorCalibrate(int CurrentWavelength, int MaxWavelength, int MinWavelength, int StepsPerNm);

/* Set integration time in milliseconds, should match the shutter speed set on camera */
uint32_t SetIngtegrationTime(uint32_t IntegrationTime);

/* Takes a reading which presses the camera shutter and takes a diode reading.
 * Return value is diode reading */
uint32_t TakeReading();

/* Just take diode reading, no camera activation */
uint32_t TakeReadingOnlyDiode();

#endif