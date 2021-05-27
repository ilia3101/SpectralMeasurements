#ifndef _Monochromator_h_
#define _Monochromator_h_

#include <stdint.h>

void MonochromatorInit();

/* Configuration */
void MonochromatorConfigure(uint32_t CurrentWavelength, uint32_t MinWavelength, uint32_t MaxWavelength, uint16_t StepsPerNm, uint8_t StepDenom);

/* Wavalength is in nm */
int MonochromatorSetWavelength(uint32_t Wavelength);

#endif
