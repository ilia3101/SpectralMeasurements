#ifndef _Control_h_
#define _Control_h_

#include <stdint.h>

/* Call this at the start */
int InitMeasurementSystem(char * DeviceFile);
void FinishMeasurement();

/* Sets monochromator wavelength */
int SetMonochromatorWavelength(int Wavelength);

/* Set calibration, pass zero to any value to not update it. */
int MonochromatorCalibrate(int CurrentWavelength, int MaxWavelength, int MinWavelength, int StepsPerNm);

/* Set integration time in milliseconds, should match the shutter speed set on camera */
int SetIngtegrationTime(uint32_t IntegrationTime);

/* Takes a reading which presses the camera shutter and takes a diode reading.
 * Return value is diode reading */
uint32_t TakeReading();

#endif