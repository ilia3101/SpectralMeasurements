#ifndef _ArduinoMessage_h_
#define _ArduinoMessage_h_

#include <stdint.h>

/* Initialise wavelength
 * a: current wavelength position
 * b: min wavelength
 * c: max wavelength
 * d: motor steps per nanometer
 * e: denominator for motor steps, if required
 */
#define Arduino_INITIALISE_MONOCHROMATOR 1
/* Sets wavelength.
   a: wavelength, b: denominator for doing fractional wavelengths.
   Returns 0 on success. */
#define Arduino_SET_WAVELENGHTH 2
/* Sets integration time, should match camera shutter speed.
   a: time in milliseconds
   Returns 0 on success */
#define Arduino_SET_INTEGRATION_TIME 3
/* Takes reading, actuates camera shutter and sends back diode value
   a: Set to 1 to not activate camera */
#define Arduino_TAKE_READING 4

typedef struct {
    // Which option
    uint8_t  type;
    
    // Additional values, smaller
    uint8_t  value_e;
    uint16_t value_d;
    
    // Main values
    uint32_t value_a;
    uint32_t value_b;
    uint32_t value_c;
} ArduinoMsg_t;

typedef struct {
  uint32_t value;
} ArduinoResponse_t;

#endif
