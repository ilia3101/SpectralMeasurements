#ifndef _Diode_h_
#define _Diode_h_

#include <stdint.h>

/* Call in setup */
void DiodeInit();

/* Returns measured value. IntegrationTime in milliseconds */
uint32_t DiodeMeasure(uint32_t IntegrationTime);

#endif
